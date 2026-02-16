"""
LightUI Python App 类
基于 ctypes 直接调用 C ABI，零编译、零依赖
"""

import json
import ctypes
import atexit
from ._ffi import (
    load_dll, LightUIConfig, LightUICallback, LightUIResizeCallback,
    LightUIVoidCallback, LightUIUpdateCallback, c_int, c_char_p, c_void_p,
    POINTER,
)
from ._state import State


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
        self._states = {}     # name -> State
        self._title_bytes = cfg.title  # keep alive

        atexit.register(self._cleanup)

    # ========== 生命周期 ==========

    def run(self):
        """启动事件循环（阻塞）"""
        self._lib.lightui_run(self._handle)

    def stop(self):
        """停止事件循环"""
        self._lib.lightui_stop(self._handle)

    def poll(self) -> bool:
        """手动轮询一次事件，返回 True 表示窗口仍存活"""
        return bool(self._lib.lightui_poll_events(self._handle))

    def _cleanup(self):
        if self._handle:
            self._lib.lightui_destroy(self._handle)
            self._handle = None
            self._lib.lightui_cleanup()

    # ========== UI 加载 ==========

    def load_html(self, html: str):
        self._lib.lightui_load_html(self._handle, html.encode("utf-8"))
        return self

    def load_html_file(self, filepath: str):
        self._lib.lightui_load_html_file(self._handle, filepath.encode("utf-8"))
        return self

    def eval_js(self, code: str):
        self._lib.lightui_eval_js(self._handle, code.encode("utf-8"))
        return self

    def eval_module(self, code: str, filename: str = "<module>"):
        self._lib.lightui_eval_module(
            self._handle, code.encode("utf-8"), filename.encode("utf-8")
        )
        return self

    def load_js_file(self, filepath: str):
        self._lib.lightui_load_js_file(self._handle, filepath.encode("utf-8"))
        return self

    # ========== 声明式 UI ==========

    def ui(self, root_component):
        """设置声明式 UI，接受组件树，自动生成 HTML 并加载

        用法:
            from lightui.ui import Column, Text, Button
            app.ui(Column(Text("Hello"), Button("Click", on_click="handler")))
        """
        self._auto_bind_counter = 0
        self._process_component(root_component)

        from .ui import build_html
        html = build_html(root_component)
        self.load_html(html)
        return self

    def _process_component(self, comp):
        """遍历组件树，将 callable 的 on_click 等自动注册为 bind"""
        if hasattr(comp, '_on_click') and callable(comp._on_click):
            name = f"_auto_click_{self._auto_bind_counter}"
            self._auto_bind_counter += 1
            fn = comp._on_click
            @self.bind(name)
            def _handler(args, _fn=fn):
                _fn()
            comp._on_click = name

        for child in getattr(comp, '_children', []):
            self._process_component(child)

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

    # ========== 状态管理 ==========

    def state(self, name: str, initial=None):
        """创建或获取一个 State 对象

        用法:
            count = app.state("count", 0)
            count.value += 1
        """
        if name in self._states:
            return self._states[name]
        s = State(self._lib, self._handle, name, initial)
        self._states[name] = s
        return s


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
            callback(w, h)
        self._callbacks.append(_cb)
        self._lib.lightui_on_resize(self._handle, _cb, None)
        return callback

    def on_close(self, callback):
        """callback()"""
        @LightUIVoidCallback
        def _cb(_ud):
            callback()
        self._callbacks.append(_cb)
        self._lib.lightui_on_close(self._handle, _cb, None)
        return callback

    def on_focus(self, callback):
        """callback()"""
        @LightUIVoidCallback
        def _cb(_ud):
            callback()
        self._callbacks.append(_cb)
        self._lib.lightui_on_focus(self._handle, _cb, None)
        return callback

    def on_blur(self, callback):
        """callback()"""
        @LightUIVoidCallback
        def _cb(_ud):
            callback()
        self._callbacks.append(_cb)
        self._lib.lightui_on_blur(self._handle, _cb, None)
        return callback

    def on_update(self, callback):
        """callback(delta_time)"""
        @LightUIUpdateCallback
        def _cb(dt, _ud):
            callback(dt)
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
