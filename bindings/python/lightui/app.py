"""
LightUI Python App 类
基于 ctypes 直接调用 C ABI，零编译、零依赖
"""

import json
import ctypes
import atexit
import warnings
from ._ffi import (
    load_dll, LightUIConfig, LightUICallback, LightUIResizeCallback,
    LightUIVoidCallback, LightUIUpdateCallback, c_int, c_char_p, c_void_p,
    POINTER,
)


class App:
    """LightUI 应用程序主类"""

    def __init__(self, title="LightUI", width=800, height=600, *,
                 dll_path=None, headless=False, borderless=False,
                 transparent=False, always_on_top=False, resizable=True,
                 gpu=True, fullscreen=False, min_size=None, max_size=None):
        self._lib = load_dll(dll_path)
        self._lib.lightui_init()

        # 使用 create_ex 支持全量配置
        cfg = self._lib.lightui_default_config()
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

        self._handle = self._lib.lightui_create_ex(ctypes.byref(cfg))
        if not self._handle:
            raise RuntimeError("lightui_create_ex 返回 NULL，创建窗口失败")

        self._callbacks = []  # prevent GC
        self._shared_objects = {}   # name -> SharedState proxy
        self._shared_handles = {}   # name -> c_void_p handle
        self._title_bytes = cfg.title  # keep alive

        atexit.register(self._cleanup)

    # ========== 生命周期 ==========

    def run(self):
        """启动事件循环（阻塞）"""
        with warnings.catch_warnings():
            warnings.filterwarnings(
                "ignore", message="memory leak in callback function",
                category=RuntimeWarning,
            )
            self._lib.lightui_run(self._handle)
        # 事件循环结束后立即清理，避免 SDL 后台线程阻止进程退出
        self._cleanup()

    def stop(self):
        """停止事件循环"""
        self._lib.lightui_stop(self._handle)

    def poll(self) -> bool:
        """手动轮询一次事件，返回 True 表示窗口仍存活"""
        return bool(self._lib.lightui_poll_events(self._handle))

    def _cleanup(self):
        if self._handle:
            # destroy shared objects first (releases JS refs)
            for name, sh in self._shared_handles.items():
                self._lib.lightui_shared_destroy(sh)
            self._shared_handles.clear()
            self._shared_objects.clear()
            # lightui_destroy 内部会调用 TerminateProcess/quick_exit，不会返回
            self._lib.lightui_destroy(self._handle)

    # ========== UI 加载 ==========

    def load_html(self, html: str):
        self._lib.lightui_load_html(self._handle, html.encode("utf-8"))
        return self

    def load_html_file(self, filepath: str):
        self._lib.lightui_load_html_file(self._handle, filepath.encode("utf-8"))
        return self

    def eval_js(self, code: str):
        ret = self._lib.lightui_eval_js(self._handle, code.encode("utf-8"))
        if ret != 0:
            err = self._lib.lightui_last_error()
            msg = err.decode("utf-8") if err else "unknown JS error"
            print(f"[JS Error] {msg}", flush=True)
        return self

    def eval_module(self, code: str, filename: str = "<module>"):
        self._lib.lightui_eval_module(
            self._handle, code.encode("utf-8"), filename.encode("utf-8")
        )
        return self

    def load_js_file(self, filepath: str):
        self._lib.lightui_load_js_file(self._handle, filepath.encode("utf-8"))
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
        if name in self._shared_objects:
            return self._shared_objects[name]

        from .shared import SharedState
        sh = self._lib.lightui_shared_create(
            self._handle, name.encode("utf-8")
        )
        if not sh:
            raise RuntimeError(f"lightui_shared_create('{name}') 返回 NULL")

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
        import os

        # ① 自动查找并加载 Preact 库（设置 globalThis.Preact / globalThis.PreactHooks）
        if not getattr(self, '_preact_loaded', False):
            self._load_preact_libs()

        # ② 解析用户 JS 文件路径
        if not os.path.isabs(js_file):
            import inspect
            caller_dir = os.path.dirname(
                os.path.abspath(inspect.stack()[1].filename)
            )
            js_file = os.path.join(caller_dir, js_file)

        with open(js_file, 'r', encoding='utf-8') as f:
            code = f.read()

        # ③ 以普通脚本模式 eval（同步执行，避免 module 异步问题）
        # 如果用户需要 import/export，可直接调用 app.eval_module()
        self.eval_js(code)
        return self

    def _load_preact_libs(self):
        """从项目目录加载 preact.js 和 hooks.js"""
        import os
        pkg_dir = os.path.dirname(os.path.abspath(__file__))
        proj_root = os.path.normpath(os.path.join(pkg_dir, "..", "..", ".."))

        preact_js = os.path.join(proj_root, "js", "preact", "preact.js")
        hooks_js = os.path.join(proj_root, "js", "preact", "hooks.js")

        # 尝试从环境变量获取路径
        env_root = os.environ.get("LIGHTUI_ROOT", "")
        if env_root:
            alt_preact = os.path.join(env_root, "js", "preact", "preact.js")
            alt_hooks = os.path.join(env_root, "js", "preact", "hooks.js")
            if os.path.exists(alt_preact):
                preact_js = alt_preact
                hooks_js = alt_hooks

        if os.path.exists(preact_js):
            with open(preact_js, 'r', encoding='utf-8') as f:
                self.eval_js(f.read())
        else:
            raise FileNotFoundError(
                f"找不到 preact.js: {preact_js}\n"
                f"请设置 LIGHTUI_ROOT 环境变量指向项目根目录"
            )

        if os.path.exists(hooks_js):
            with open(hooks_js, 'r', encoding='utf-8') as f:
                self.eval_js(f.read())

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
        def _decorator(func, fname=None):
            fn_name = fname or func.__name__

            @LightUICallback
            def _callback(args_json, _user_data):
                try:
                    args_str = args_json.decode("utf-8") if args_json else "null"
                    args = json.loads(args_str)
                    result = func(args)
                    ret = json.dumps(result, ensure_ascii=False)
                    return ret.encode("utf-8")
                except Exception as e:
                    err = json.dumps({"error": str(e)})
                    return err.encode("utf-8")

            self._callbacks.append(_callback)  # prevent GC
            self._lib.lightui_bind(
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
        self._lib.lightui_unbind(self._handle, name.encode("utf-8"))

    def emit(self, event: str, data=None):
        data_json = json.dumps(data, ensure_ascii=False) if data is not None else "null"
        self._lib.lightui_emit(
            self._handle, event.encode("utf-8"), data_json.encode("utf-8")
        )

    # ========== 窗口属性 ==========

    @property
    def title(self):
        return self._title_bytes

    @title.setter
    def title(self, value: str):
        b = value.encode("utf-8")
        self._title_bytes = b
        self._lib.lightui_set_title(self._handle, b)

    @property
    def size(self):
        w, h = c_int(0), c_int(0)
        self._lib.lightui_get_size(self._handle, ctypes.byref(w), ctypes.byref(h))
        return (w.value, h.value)

    @size.setter
    def size(self, wh):
        self._lib.lightui_set_size(self._handle, wh[0], wh[1])

    @property
    def position(self):
        x, y = c_int(0), c_int(0)
        self._lib.lightui_get_position(self._handle, ctypes.byref(x), ctypes.byref(y))
        return (x.value, y.value)

    @position.setter
    def position(self, xy):
        self._lib.lightui_set_position(self._handle, xy[0], xy[1])

    def set_min_size(self, width: int, height: int):
        self._lib.lightui_set_min_size(self._handle, width, height)

    def set_max_size(self, width: int, height: int):
        self._lib.lightui_set_max_size(self._handle, width, height)

    def minimize(self):
        self._lib.lightui_minimize(self._handle)

    def maximize(self):
        self._lib.lightui_maximize(self._handle)

    def restore(self):
        self._lib.lightui_restore(self._handle)

    def show(self):
        self._lib.lightui_show(self._handle)

    def hide(self):
        self._lib.lightui_hide(self._handle)

    def set_fullscreen(self, fullscreen: bool):
        self._lib.lightui_set_fullscreen(self._handle, fullscreen)

    def set_resizable(self, resizable: bool):
        self._lib.lightui_set_resizable(self._handle, resizable)

    def set_borderless(self, borderless: bool):
        self._lib.lightui_set_borderless(self._handle, borderless)

    def set_always_on_top(self, on_top: bool):
        self._lib.lightui_set_always_on_top(self._handle, on_top)

    # ========== 事件回调 ==========

    def on_resize(self, callback):
        """callback(width, height)"""
        @LightUIResizeCallback
        def _cb(w, h, _ud):
            try:
                callback(w, h)
            except Exception as e:
                import traceback; traceback.print_exc()
        self._callbacks.append(_cb)
        self._lib.lightui_on_resize(self._handle, _cb, None)
        return callback

    def on_close(self, callback):
        """callback()"""
        @LightUIVoidCallback
        def _cb(_ud):
            try:
                callback()
            except Exception as e:
                import traceback; traceback.print_exc()
        self._callbacks.append(_cb)
        self._lib.lightui_on_close(self._handle, _cb, None)
        return callback

    def on_focus(self, callback):
        """callback()"""
        @LightUIVoidCallback
        def _cb(_ud):
            try:
                callback()
            except Exception as e:
                import traceback; traceback.print_exc()
        self._callbacks.append(_cb)
        self._lib.lightui_on_focus(self._handle, _cb, None)
        return callback

    def on_blur(self, callback):
        """callback()"""
        @LightUIVoidCallback
        def _cb(_ud):
            try:
                callback()
            except Exception as e:
                import traceback; traceback.print_exc()
        self._callbacks.append(_cb)
        self._lib.lightui_on_blur(self._handle, _cb, None)
        return callback

    def on_update(self, callback):
        """callback(delta_time)"""
        @LightUIUpdateCallback
        def _cb(dt, _ud):
            try:
                callback(dt)
            except Exception as e:
                import traceback; traceback.print_exc()
        self._callbacks.append(_cb)
        self._lib.lightui_on_update(self._handle, _cb, None)
        return callback

    # ========== 批量操作 ==========

    def batch(self):
        """上下文管理器：批量状态更新"""
        return _BatchContext(self._lib, self._handle)

    # ========== DevTools ==========

    def devtools_open(self):
        self._lib.lightui_devtools_open(self._handle)

    def devtools_close(self):
        self._lib.lightui_devtools_close(self._handle)


class _BatchContext:
    """with app.batch(): 批量更新状态"""
    def __init__(self, lib, handle):
        self._lib = lib
        self._handle = handle

    def __enter__(self):
        self._lib.lightui_state_batch_begin(self._handle)
        return self

    def __exit__(self, *args):
        self._lib.lightui_state_batch_end(self._handle)
