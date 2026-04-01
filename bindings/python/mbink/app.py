"""
MBink Python App 类
基于 ctypes 直接调用 C ABI，零编译、零依赖
"""

import json
import ctypes
import atexit
import os
import inspect
import warnings
from ._ffi import (
    load_dll, MBinkConfig, MBinkCallback, MBinkResizeCallback,
    MBinkVoidCallback, MBinkUpdateCallback, c_int, c_char_p, c_void_p,
    POINTER,
)


class App:
    """MBink 应用程序主类"""

    def __init__(self, title="MBink", width=800, height=600, *,
                 dll_path=None, headless=False, borderless=False,
                 transparent=False, always_on_top=False, resizable=True,
                 gpu=True, fullscreen=False, min_size=None, max_size=None):
        self._lib = load_dll(dll_path)
        self._lib.mbink_init()
        self._destroyed = False

        # 使用 create_ex 支持全量配置
        cfg = self._lib.mbink_default_config()
        cfg.title = title.encode("utf-8")
        cfg.width = width
        cfg.height = height
        cfg.headless = headless
        cfg.borderless = borderless
        cfg.transparent = transparent
        cfg.always_on_top = always_on_top
        cfg.resizable = resizable
        cfg.gpu = gpu
        cfg.fullscreen = fullscreen
        if min_size:
            cfg.min_width, cfg.min_height = min_size
        if max_size:
            cfg.max_width, cfg.max_height = max_size

        self._handle = self._lib.mbink_create_ex(ctypes.byref(cfg))
        if not self._handle:
            raise RuntimeError("mbink_create_ex 返回 NULL，创建窗口失败")

        self._callbacks = []  # prevent GC
        self._shared_objects = {}   # name -> SharedState proxy
        self._shared_handles = {}   # name -> c_void_p handle
        self._title_bytes = cfg.title  # keep alive

        atexit.register(self._cleanup)

    # ========== 生命周期 ==========

    def _ensure_alive(self):
        if self._destroyed or not self._handle:
            raise RuntimeError("MBink App 已销毁，不能继续调用此操作")

    def _resolve_user_path(self, path: str):
        if os.path.isabs(path):
            return path
        caller_dir = os.path.dirname(os.path.abspath(inspect.stack()[2].filename))
        return os.path.join(caller_dir, path)

    def run(self):
        """启动事件循环（阻塞）"""
        self._ensure_alive()
        with warnings.catch_warnings():
            warnings.filterwarnings(
                "ignore", message="memory leak in callback function",
                category=RuntimeWarning,
            )
            self._lib.mbink_run(self._handle)
        # 事件循环结束后立即清理，避免 SDL 后台线程阻止进程退出
        self._cleanup()

    def stop(self):
        """停止事件循环"""
        self._ensure_alive()
        self._lib.mbink_stop(self._handle)

    def poll(self) -> bool:
        """手动轮询一次事件，返回 True 表示窗口仍存活"""
        self._ensure_alive()
        return bool(self._lib.mbink_poll_events(self._handle))

    def _cleanup(self):
        if self._destroyed:
            return
        if self._handle:
            # destroy shared objects first (releases JS refs)
            for _name, sh in list(self._shared_handles.items()):
                self._lib.mbink_shared_destroy(sh)
            self._shared_handles.clear()
            self._shared_objects.clear()
            self._lib.mbink_destroy(self._handle)
            self._handle = None
        self._destroyed = True

    # ========== UI 加载 ==========

    def load_html(self, html: str):
        self._ensure_alive()
        ret = self._lib.mbink_load_html(self._handle, html.encode("utf-8"))
        if ret != 0:
            err = self._lib.mbink_last_error()
            msg = err.decode("utf-8") if err else "unknown JS error"
            print(f"[JS Error] {msg}", flush=True)
        return self

    def load_html_file(self, filepath: str):
        self._ensure_alive()
        filepath = self._resolve_user_path(filepath)
        ret = self._lib.mbink_load_html_file(self._handle, filepath.encode("utf-8"))
        if ret != 0:
            err = self._lib.mbink_last_error()
            msg = err.decode("utf-8") if err else "unknown JS error"
            print(f"[JS Error] {msg}", flush=True)
        return self

    def eval_js(self, code: str):
        self._ensure_alive()
        ret = self._lib.mbink_eval_js(self._handle, code.encode("utf-8"))
        if ret != 0:
            err = self._lib.mbink_last_error()
            msg = err.decode("utf-8") if err else "unknown JS error"
            print(f"[JS Error] {msg}", flush=True)
        return self

    def eval_module(self, code: str, filename: str = "<module>"):
        self._ensure_alive()
        ret = self._lib.mbink_eval_module(
            self._handle, code.encode("utf-8"), filename.encode("utf-8")
        )
        if ret != 0:
            err = self._lib.mbink_last_error()
            msg = err.decode("utf-8") if err else "unknown JS error"
            print(f"[JS Error] {msg}", flush=True)
        return self

    def load_js_file(self, filepath: str):
        self._ensure_alive()
        filepath = self._resolve_user_path(filepath)
        ret = self._lib.mbink_load_js_file(self._handle, filepath.encode("utf-8"))
        if ret != 0:
            err = self._lib.mbink_last_error()
            msg = err.decode("utf-8") if err else "unknown JS error"
            print(f"[JS Error] {msg}", flush=True)
        return self

    # ========== 共享 C 对象 ==========

    def shared(self, name: str = "data"):
        """创建/获取共享 C 对象，注册为 JS globalThis.<name>

        用法：
            data = app.shared("data")
            data.count = 0          # → JS: data.count = 0
            data.count += 1         # → 自动触发 Preact 重渲染

        返回 SharedState 代理对象。
        """
        self._ensure_alive()
        if name in self._shared_objects:
            return self._shared_objects[name]

        from .shared import SharedState
        sh = self._lib.mbink_shared_create(
            self._handle, name.encode("utf-8")
        )
        if not sh:
            raise RuntimeError(f"mbink_shared_create('{name}') 返回 NULL")

        proxy = SharedState(self._lib, self._handle, sh, name)
        self._shared_objects[name] = proxy
        self._shared_handles[name] = sh
        return proxy

    def load_preact(self, js_file: str):
        """加载 Preact 应用（.js 入口文件）

        自动完成：
        1. 加载 Preact + Hooks 到 globalThis
        2. 以 module 模式加载入口 JS 文件

        用法：
            app.load_preact("ui/app.js")
        """
        # ① 自动查找并加载 Preact 库（设置 globalThis.Preact / globalThis.PreactHooks）
        if not getattr(self, '_preact_loaded', False):
            self._load_preact_libs()

        # ② 解析用户 JS 文件路径
        js_file = self._resolve_user_path(js_file)

        # ③ 走原生 ES module 文件加载链路，与 esm_loader 保持一致
        self.load_js_file(js_file)
        return self

    def _load_preact_libs(self):
        """运行时已内嵌加载 preact / hooks，这里只做一次标记。"""
        self._preact_loaded = True

    # ========== 函数绑定 ==========

    def bind(self, name_or_func=None):
        """
        注册 Python 函数供 JS 调用。可作为装饰器使用：

            @app.bind("greet")
            def greet(args):
                return {"msg": f"Hello {args}"}

        也可直接调用：
            app.bind("greet", greet_func)
        """
        self._ensure_alive()
        def _decorator(func, fname=None):
            fn_name = fname or func.__name__

            @MBinkCallback
            def _callback(args_json, _user_data):
                try:
                    args_str = args_json.decode("utf-8") if args_json else "null"
                    args = json.loads(args_str)
                    result = func(args)
                    ret = json.dumps(result, ensure_ascii=False)
                    return self._lib.mbink_copy_string(ret.encode("utf-8"))
                except Exception as e:
                    err = json.dumps({"error": str(e)})
                    return self._lib.mbink_copy_string(err.encode("utf-8"))

            self._callbacks.append(_callback)  # prevent GC
            self._lib.mbink_bind(
                self._handle, fn_name.encode("utf-8"), _callback, None
            )
            return func

        # @app.bind 或 @app.bind("name")
        if callable(name_or_func):
            return _decorator(name_or_func)
        else:
            def _wrapper(func):
                return _decorator(func, name_or_func)
            return _wrapper

    def unbind(self, name: str):
        self._ensure_alive()
        self._lib.mbink_unbind(self._handle, name.encode("utf-8"))

    def emit(self, event: str, data=None):
        self._ensure_alive()
        data_json = json.dumps(data, ensure_ascii=False) if data is not None else "null"
        self._lib.mbink_emit(
            self._handle, event.encode("utf-8"), data_json.encode("utf-8")
        )

    # ========== 窗口属性 ==========

    @property
    def title(self):
        return self._title_bytes.decode("utf-8") if self._title_bytes else ""

    @title.setter
    def title(self, value: str):
        self._ensure_alive()
        b = value.encode("utf-8")
        self._title_bytes = b
        self._lib.mbink_set_title(self._handle, b)

    @property
    def size(self):
        self._ensure_alive()
        w, h = c_int(0), c_int(0)
        self._lib.mbink_get_size(self._handle, ctypes.byref(w), ctypes.byref(h))
        return (w.value, h.value)

    @size.setter
    def size(self, wh):
        self._ensure_alive()
        self._lib.mbink_set_size(self._handle, wh[0], wh[1])

    @property
    def position(self):
        self._ensure_alive()
        x, y = c_int(0), c_int(0)
        self._lib.mbink_get_position(self._handle, ctypes.byref(x), ctypes.byref(y))
        return (x.value, y.value)

    @position.setter
    def position(self, xy):
        self._ensure_alive()
        self._lib.mbink_set_position(self._handle, xy[0], xy[1])

    def set_min_size(self, width: int, height: int):
        self._ensure_alive()
        self._lib.mbink_set_min_size(self._handle, width, height)

    def set_max_size(self, width: int, height: int):
        self._ensure_alive()
        self._lib.mbink_set_max_size(self._handle, width, height)

    def minimize(self):
        self._ensure_alive()
        self._lib.mbink_minimize(self._handle)

    def maximize(self):
        self._ensure_alive()
        self._lib.mbink_maximize(self._handle)

    def restore(self):
        self._ensure_alive()
        self._lib.mbink_restore(self._handle)

    def show(self):
        self._ensure_alive()
        self._lib.mbink_show(self._handle)

    def hide(self):
        self._ensure_alive()
        self._lib.mbink_hide(self._handle)

    def set_fullscreen(self, fullscreen: bool):
        self._ensure_alive()
        self._lib.mbink_set_fullscreen(self._handle, fullscreen)

    def set_resizable(self, resizable: bool):
        self._ensure_alive()
        self._lib.mbink_set_resizable(self._handle, resizable)

    def set_borderless(self, borderless: bool):
        self._ensure_alive()
        self._lib.mbink_set_borderless(self._handle, borderless)

    def set_always_on_top(self, on_top: bool):
        self._ensure_alive()
        self._lib.mbink_set_always_on_top(self._handle, on_top)

    # ========== 事件回调 ==========

    def on_resize(self, callback):
        """callback(width, height)"""
        self._ensure_alive()

        @MBinkResizeCallback
        def _cb(w, h, _ud):
            try:
                callback(w, h)
            except Exception:
                import traceback; traceback.print_exc()
        self._callbacks.append(_cb)
        self._lib.mbink_on_resize(self._handle, _cb, None)
        return callback

    def on_close(self, callback):
        """callback()"""
        self._ensure_alive()

        @MBinkVoidCallback
        def _cb(_ud):
            try:
                callback()
            except Exception:
                import traceback; traceback.print_exc()
        self._callbacks.append(_cb)
        self._lib.mbink_on_close(self._handle, _cb, None)
        return callback

    def on_focus(self, callback):
        """callback()"""
        self._ensure_alive()

        @MBinkVoidCallback
        def _cb(_ud):
            try:
                callback()
            except Exception:
                import traceback; traceback.print_exc()
        self._callbacks.append(_cb)
        self._lib.mbink_on_focus(self._handle, _cb, None)
        return callback

    def on_blur(self, callback):
        """callback()"""
        self._ensure_alive()

        @MBinkVoidCallback
        def _cb(_ud):
            try:
                callback()
            except Exception:
                import traceback; traceback.print_exc()
        self._callbacks.append(_cb)
        self._lib.mbink_on_blur(self._handle, _cb, None)
        return callback

    def on_update(self, callback):
        """callback(delta_time)"""
        self._ensure_alive()

        @MBinkUpdateCallback
        def _cb(dt, _ud):
            try:
                callback(dt)
            except Exception:
                import traceback; traceback.print_exc()
        self._callbacks.append(_cb)
        self._lib.mbink_on_update(self._handle, _cb, None)
        return callback

    # ========== 批量操作 ==========

    def batch(self):
        """上下文管理器：批量状态更新"""
        self._ensure_alive()
        return _BatchContext(self._lib, self._handle)

    # ========== DevTools ==========

    def devtools_open(self):
        self._ensure_alive()
        self._lib.mbink_devtools_open(self._handle)

    def devtools_close(self):
        self._ensure_alive()
        self._lib.mbink_devtools_close(self._handle)


class _BatchContext:
    """with app.batch(): 批量更新状态"""
    def __init__(self, lib, handle):
        self._lib = lib
        self._handle = handle

    def __enter__(self):
        if not self._handle:
            raise RuntimeError("MBink App 已销毁，不能进入 batch 上下文")
        self._lib.mbink_state_batch_begin(self._handle)
        return self

    def __exit__(self, *args):
        if self._handle:
            self._lib.mbink_state_batch_end(self._handle)
