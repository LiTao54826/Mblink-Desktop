"""
MBlink Python App 类
基于 ctypes 直接调用 C ABI，零编译、零依赖
"""

import json
import ctypes
import atexit
import os
import inspect
import warnings
import asyncio
import threading
import time
import urllib.request
from .controls import LogView, Terminal
from .resources import RESOURCE_FLAG_BYTECODE, load_resource_file
from ._ffi import (
    load_dll, load_devtools_dll, MBlinkConfig, MBlinkCallback, MBlinkAsyncCallback, MBlinkResizeCallback,
    MBlinkVoidCallback, MBlinkBoolCallback, MBlinkUpdateCallback, c_int, c_char_p, c_void_p,
    MBlinkDevToolsHttpInfo, POINTER,
)


class App:
    """MBlink 应用程序主类"""

    def __init__(self, title="MBlink", width=800, height=600, *,
                 dll_path=None, headless=False, borderless=False,
                 transparent=False, always_on_top=False, resizable=True,
                 gpu=True, fullscreen=False, min_size=None, max_size=None):
        self._lib = load_dll(dll_path)
        self._devtools_lib = None
        self._lib.mblink_init()
        self._destroyed = False

        # 使用 create_ex 支持全量配置
        cfg = self._lib.mblink_default_config()
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

        self._handle = self._lib.mblink_create_ex(ctypes.byref(cfg))
        if not self._handle:
            err = self._lib.mblink_last_error()
            msg = err.decode("utf-8", "ignore") if err else "unknown create error"
            raise RuntimeError(f"mblink_create_ex 返回 NULL，创建窗口失败: {msg}")

        runtime_options = self._lib.mblink_default_runtime_options()
        ret = self._lib.mblink_configure_runtime(self._handle, ctypes.byref(runtime_options))
        if ret != 0:
            err = self._lib.mblink_last_error()
            msg = err.decode("utf-8", "ignore") if err else "unknown runtime error"
            self._lib.mblink_destroy(self._handle)
            self._handle = None
            raise RuntimeError(f"mblink_configure_runtime failed: {msg}")

        self._callbacks = []  # prevent GC
        self._tray_callbacks = []
        self._tray_action_handlers = {}
        self._tray_action_dispatch_enabled = False
        self._shared_objects = {}   # name -> SharedState proxy
        self._shared_handles = {}   # name -> c_void_p handle
        self._control_handles = []  # native control handles
        self._title_bytes = cfg.title  # keep alive

        atexit.register(self._cleanup)

    # ========== 生命周期 ==========

    def _ensure_alive(self):
        if self._destroyed or not self._handle:
            raise RuntimeError("MBlink App 已销毁，不能继续调用此操作")

    def _devtools(self):
        if self._devtools_lib is None:
            self._devtools_lib = load_devtools_dll(self._lib)
        return self._devtools_lib

    def _resolve_user_path(self, path: str):
        if os.path.isabs(path):
            return path

        current_file = os.path.normcase(os.path.abspath(__file__))
        frame = inspect.currentframe()
        try:
            frame = frame.f_back if frame else None
            while frame:
                filename = frame.f_code.co_filename
                if filename and os.path.normcase(os.path.abspath(filename)) != current_file:
                    caller_dir = os.path.dirname(os.path.abspath(filename))
                    return os.path.normpath(os.path.join(caller_dir, path))
                frame = frame.f_back
        finally:
            del frame

        return os.path.normpath(os.path.abspath(path))

    def _call_file_loader(self, loader, filepath: str):
        raw_path = filepath
        ret = loader(self._handle, raw_path.encode("utf-8"))
        if ret == 0:
            return 0

        resolved_path = self._resolve_user_path(filepath)
        if resolved_path != raw_path:
            ret = loader(self._handle, resolved_path.encode("utf-8"))
        return ret

    def _owned_json(self, producer):
        self._ensure_alive()
        out = c_void_p()
        ret = producer(self._handle, ctypes.byref(out))
        if ret != 0:
            err = self._lib.mblink_last_error()
            msg = err.decode("utf-8") if err else "unknown json error"
            raise RuntimeError(msg)
        try:
            raw = ctypes.cast(out, c_char_p).value.decode("utf-8") if out.value else "{}"
            return json.loads(raw)
        finally:
            if out.value:
                self._lib.mblink_free(out)

    def run(self):
        """启动事件循环（阻塞）"""
        self._ensure_alive()
        with warnings.catch_warnings():
            warnings.filterwarnings(
                "ignore", message="memory leak in callback function",
                category=RuntimeWarning,
            )
            self._lib.mblink_run(self._handle)
        # 事件循环结束后立即清理，避免 SDL 后台线程阻止进程退出
        self._cleanup()

    def stop(self):
        """停止事件循环"""
        self._ensure_alive()
        self._lib.mblink_stop(self._handle)

    def poll(self) -> bool:
        """手动轮询一次事件，返回 True 表示窗口仍存活"""
        self._ensure_alive()
        return bool(self._lib.mblink_poll_events(self._handle))

    def wait(self) -> bool:
        """Block until one event-loop step completes; returns True while alive."""
        self._ensure_alive()
        return bool(self._lib.mblink_wait_events(self._handle))

    def _cleanup(self):
        if self._destroyed:
            return
        if self._handle:
            for kind, control_handle in reversed(self._control_handles):
                if kind == "logview":
                    self._lib.mblink_logview_destroy(control_handle)
                elif kind == "terminal":
                    self._lib.mblink_terminal_destroy(control_handle)
            self._control_handles.clear()
            # destroy shared objects first (releases JS refs)
            for _name, sh in list(self._shared_handles.items()):
                self._lib.mblink_shared_destroy(sh)
            self._shared_handles.clear()
            self._shared_objects.clear()
            self._lib.mblink_destroy(self._handle)
            self._handle = None
        self._destroyed = True

    # ========== UI 加载 ==========

    def load_html(self, html: str):
        self._ensure_alive()
        ret = self._lib.mblink_load_html(self._handle, html.encode("utf-8"))
        if ret != 0:
            err = self._lib.mblink_last_error()
            msg = err.decode("utf-8") if err else "unknown JS error"
            print(f"[JS Error] {msg}", flush=True)
        return self

    def load_html_file(self, filepath: str):
        self._ensure_alive()
        ret = self._call_file_loader(self._lib.mblink_load_html_file, filepath)
        if ret != 0:
            err = self._lib.mblink_last_error()
            msg = err.decode("utf-8") if err else "unknown JS error"
            print(f"[JS Error] {msg}", flush=True)
        return self

    def load_entry_file(self, filepath: str, execute_html_scripts: bool = True):
        self._ensure_alive()

        def _loader(handle, encoded_path):
            return self._lib.mblink_load_entry_file(handle, encoded_path, execute_html_scripts)

        ret = self._call_file_loader(_loader, filepath)
        if ret != 0:
            err = self._lib.mblink_last_error()
            msg = err.decode("utf-8") if err else "unknown load error"
            raise RuntimeError(msg)
        return self

    def eval_js(self, code: str):
        self._ensure_alive()
        ret = self._lib.mblink_eval_js(self._handle, code.encode("utf-8"))
        if ret != 0:
            err = self._lib.mblink_last_error()
            msg = err.decode("utf-8") if err else "unknown JS error"
            print(f"[JS Error] {msg}", flush=True)
        return self

    def eval_module(self, code: str, filename: str = "<module>"):
        self._ensure_alive()
        ret = self._lib.mblink_eval_module(
            self._handle, code.encode("utf-8"), filename.encode("utf-8")
        )
        if ret != 0:
            err = self._lib.mblink_last_error()
            msg = err.decode("utf-8") if err else "unknown JS error"
            print(f"[JS Error] {msg}", flush=True)
        return self

    def load_js_file(self, filepath: str):
        self._ensure_alive()
        ret = self._call_file_loader(self._lib.mblink_load_js_file, filepath)
        if ret != 0:
            err = self._lib.mblink_last_error()
            msg = err.decode("utf-8") if err else "unknown JS error"
            print(f"[JS Error] {msg}", flush=True)
        return self

    def load_module_file(self, filepath: str):
        self._ensure_alive()
        ret = self._call_file_loader(self._lib.mblink_load_module_file, filepath)
        if ret != 0:
            err = self._lib.mblink_last_error()
            msg = err.decode("utf-8") if err else "unknown module load error"
            raise RuntimeError(msg)
        return self

    def load_bytecode(self, data: bytes):
        self._ensure_alive()
        buf = ctypes.create_string_buffer(data)
        ret = self._lib.mblink_load_bytecode(self._handle, buf, len(data))
        if ret != 0:
            err = self._lib.mblink_last_error()
            msg = err.decode("utf-8") if err else "unknown JS error"
            raise RuntimeError(msg)
        return self

    def load_resource_bytecode(self, package_file: str, resource_path: str, encryption_key: str = ""):
        package_file = self._resolve_user_path(package_file)
        data, flags = load_resource_file(package_file, resource_path, encryption_key)
        if not (flags & RESOURCE_FLAG_BYTECODE):
            raise ValueError(f"resource '{resource_path}' 不是 QuickJS bytecode")
        return self.load_bytecode(data)

    def load_resource_text(self, package_file: str, resource_path: str, encryption_key: str = ""):
        package_file = self._resolve_user_path(package_file)
        data, _ = load_resource_file(package_file, resource_path, encryption_key)
        return data.decode("utf-8")

    def mount_resource_package(self, package_file: str, encryption_key: str = "", mount_point: str = "/"):
        self._ensure_alive()
        package_file = self._resolve_user_path(package_file)
        ret = self._lib.mblink_mount_resource_package(
            self._handle,
            package_file.encode("utf-8"),
            encryption_key.encode("utf-8"),
            mount_point.encode("utf-8"),
        )
        if ret != 0:
            err = self._lib.mblink_last_error()
            msg = err.decode("utf-8") if err else "unknown resource package error"
            raise RuntimeError(msg)
        return self


    # ========== 共享 C 对象 ==========

    def runtime_epoch(self) -> str:
        self._ensure_alive()
        out = c_void_p()
        ret = self._lib.mblink_runtime_epoch(self._handle, ctypes.byref(out))
        if ret != 0:
            err = self._lib.mblink_last_error()
            msg = err.decode("utf-8") if err else "unknown runtime error"
            raise RuntimeError(msg)
        try:
            return ctypes.cast(out, c_char_p).value.decode("utf-8") if out.value else ""
        finally:
            if out.value:
                self._lib.mblink_free(out)

    def lifecycle_state(self) -> int:
        self._ensure_alive()
        return int(self._lib.mblink_lifecycle_state(self._handle))

    def lifecycle_reason(self) -> str:
        self._ensure_alive()
        out = c_void_p()
        ret = self._lib.mblink_lifecycle_reason(self._handle, ctypes.byref(out))
        if ret != 0:
            err = self._lib.mblink_last_error()
            msg = err.decode("utf-8") if err else "unknown lifecycle error"
            raise RuntimeError(msg)
        try:
            return ctypes.cast(out, c_char_p).value.decode("utf-8") if out.value else ""
        finally:
            if out.value:
                self._lib.mblink_free(out)

    def observe_console(self):
        return self._owned_json(self._lib.mblink_observe_console_json)

    def observe_errors(self):
        return self._owned_json(self._lib.mblink_observe_errors_json)

    def observe_lifecycle(self):
        return self._owned_json(self._lib.mblink_observe_lifecycle_json)

    def clear_observation(self, kind: int):
        self._ensure_alive()
        ret = self._lib.mblink_observe_clear(self._handle, kind)
        if ret != 0:
            err = self._lib.mblink_last_error()
            msg = err.decode("utf-8") if err else "unknown observation error"
            raise RuntimeError(msg)
        return self

    def ui_dev_snapshot(self, **options):
        self._ensure_alive()
        devtools = self._devtools()
        snapshot_options = devtools.mblink_ui_dev_default_snapshot_options()
        keepalive = []
        for key, value in options.items():
            if value is None:
                continue
            if key in {"runtime_epoch", "root_selector", "screenshot_file"}:
                encoded = str(value).encode("utf-8")
                keepalive.append(encoded)
                setattr(snapshot_options, key, encoded)
            elif hasattr(snapshot_options, key):
                setattr(snapshot_options, key, value)
            else:
                raise TypeError(f"unknown snapshot option: {key}")
        out = c_void_p()
        ret = devtools.mblink_ui_dev_snapshot_json(
            self._handle, ctypes.byref(snapshot_options), ctypes.byref(out)
        )
        if ret != 0:
            err = self._lib.mblink_last_error()
            msg = err.decode("utf-8") if err else "unknown snapshot error"
            raise RuntimeError(msg)
        try:
            raw = ctypes.cast(out, c_char_p).value.decode("utf-8") if out.value else "{}"
            return json.loads(raw)
        finally:
            if out.value:
                self._lib.mblink_free(out)
            keepalive.clear()

    def ui_dev_snapshot_file(self, output_path: str, **options):
        self._ensure_alive()
        devtools = self._devtools()
        snapshot_options = devtools.mblink_ui_dev_default_snapshot_options()
        keepalive = []
        for key, value in options.items():
            if value is None:
                continue
            if key in {"runtime_epoch", "root_selector", "screenshot_file"}:
                encoded = str(value).encode("utf-8")
                keepalive.append(encoded)
                setattr(snapshot_options, key, encoded)
            elif hasattr(snapshot_options, key):
                setattr(snapshot_options, key, value)
            else:
                raise TypeError(f"unknown snapshot option: {key}")
        ret = devtools.mblink_ui_dev_snapshot_file(
            self._handle, output_path.encode("utf-8"), ctypes.byref(snapshot_options)
        )
        keepalive.clear()
        if ret != 0:
            err = self._lib.mblink_last_error()
            msg = err.decode("utf-8") if err else "unknown snapshot error"
            raise RuntimeError(msg)
        return self

    def ui_dev_command(self, command):
        self._ensure_alive()
        devtools = self._devtools()
        payload = command if isinstance(command, str) else json.dumps(command, ensure_ascii=False)
        out = c_void_p()
        ret = devtools.mblink_ui_dev_command_json(
            self._handle, payload.encode("utf-8"), ctypes.byref(out)
        )
        if ret != 0:
            err = self._lib.mblink_last_error()
            msg = err.decode("utf-8") if err else "unknown command error"
            raise RuntimeError(msg)
        try:
            raw = ctypes.cast(out, c_char_p).value.decode("utf-8") if out.value else "{}"
            return json.loads(raw)
        finally:
            if out.value:
                self._lib.mblink_free(out)

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
        sh = self._lib.mblink_shared_create(
            self._handle, name.encode("utf-8")
        )
        if not sh:
            raise RuntimeError(f"mblink_shared_create('{name}') 返回 NULL")

        proxy = SharedState(self._lib, self._handle, sh, name)
        self._shared_objects[name] = proxy
        self._shared_handles[name] = sh
        return proxy

    def logview(self, element_id: str):
        self._ensure_alive()
        handle = self._lib.mblink_logview_get(self._handle, element_id.encode("utf-8"))
        if not handle:
            err = self._lib.mblink_last_error()
            msg = err.decode("utf-8") if err else f"logview '{element_id}' not found"
            raise RuntimeError(msg)
        self._control_handles.append(("logview", handle))
        return LogView(self._lib, handle, element_id)

    def terminal(self, element_id: str):
        self._ensure_alive()
        handle = self._lib.mblink_terminal_get(self._handle, element_id.encode("utf-8"))
        if not handle:
            err = self._lib.mblink_last_error()
            msg = err.decode("utf-8") if err else f"terminal '{element_id}' not found"
            raise RuntimeError(msg)
        self._control_handles.append(("terminal", handle))
        return Terminal(self._lib, handle, element_id)

    # ========== 函数绑定 ==========

    def _wrap_result_json(self, result):
        return self._lib.mblink_copy_string(json.dumps(result, ensure_ascii=False).encode("utf-8"))

    def _wrap_error_json(self, exc: Exception):
        return self._lib.mblink_copy_string(json.dumps({"error": str(exc)}, ensure_ascii=False).encode("utf-8"))

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

            @MBlinkCallback
            def _callback(args_json, _user_data):
                try:
                    args_str = args_json.decode("utf-8") if args_json else "null"
                    args = json.loads(args_str)
                    result = func(args)
                    return self._wrap_result_json(result)
                except Exception as e:
                    return self._wrap_error_json(e)

            self._callbacks.append(_callback)  # prevent GC
            self._lib.mblink_bind(
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

    def bind_async(self, name_or_func=None):
        self._ensure_alive()

        def _decorator(func, fname=None):
            fn_name = fname or func.__name__

            @MBlinkAsyncCallback
            def _callback(args_json, _user_data):
                try:
                    args_str = args_json.decode("utf-8") if args_json else "null"
                    args = json.loads(args_str)
                    result = func(args)
                    if inspect.isawaitable(result):
                        result = asyncio.run(result)
                    return self._wrap_result_json(result)
                except Exception as e:
                    return self._wrap_error_json(e)

            self._callbacks.append(_callback)
            self._lib.mblink_bind_async(
                self._handle, fn_name.encode("utf-8"), _callback, None
            )
            return func

        if callable(name_or_func):
            return _decorator(name_or_func)
        else:
            def _wrapper(func):
                return _decorator(func, name_or_func)
            return _wrapper

    def unbind(self, name: str):
        self._ensure_alive()
        self._lib.mblink_unbind(self._handle, name.encode("utf-8"))

    def emit(self, event: str, data=None):
        self._ensure_alive()
        data_json = json.dumps(data, ensure_ascii=False) if data is not None else "null"
        self._lib.mblink_emit(
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
        self._lib.mblink_set_title(self._handle, b)

    @property
    def size(self):
        self._ensure_alive()
        w, h = c_int(0), c_int(0)
        self._lib.mblink_get_size(self._handle, ctypes.byref(w), ctypes.byref(h))
        return (w.value, h.value)

    @size.setter
    def size(self, wh):
        self._ensure_alive()
        self._lib.mblink_set_size(self._handle, wh[0], wh[1])

    @property
    def position(self):
        self._ensure_alive()
        x, y = c_int(0), c_int(0)
        self._lib.mblink_get_position(self._handle, ctypes.byref(x), ctypes.byref(y))
        return (x.value, y.value)

    @position.setter
    def position(self, xy):
        self._ensure_alive()
        self._lib.mblink_set_position(self._handle, xy[0], xy[1])

    def set_min_size(self, width: int, height: int):
        self._ensure_alive()
        self._lib.mblink_set_min_size(self._handle, width, height)

    def set_max_size(self, width: int, height: int):
        self._ensure_alive()
        self._lib.mblink_set_max_size(self._handle, width, height)

    def minimize(self):
        self._ensure_alive()
        self._lib.mblink_minimize(self._handle)

    def maximize(self):
        self._ensure_alive()
        self._lib.mblink_maximize(self._handle)

    def restore(self):
        self._ensure_alive()
        self._lib.mblink_restore(self._handle)

    def show(self):
        self._ensure_alive()
        self._lib.mblink_show(self._handle)

    def hide(self):
        self._ensure_alive()
        self._lib.mblink_hide(self._handle)

    def show_main_window(self):
        """显示并激活主窗口；适用于托盘恢复场景。"""
        self.show()
        self.restore()

    def hide_to_tray(self):
        """隐藏主窗口；通常与用户态创建的系统托盘配合使用。"""
        self.hide()

    def create_tray(self, tooltip=None, menu=None):
        self._ensure_alive()
        tip = (tooltip or self.title).encode("utf-8")
        rc = self._lib.mblink_tray_create(self._handle, tip)
        if rc != 0:
            raise RuntimeError(f"创建 tray 失败: {rc}")
        if menu is not None:
            self.set_tray_menu(menu)

    def destroy_tray(self):
        self._ensure_alive()
        self._lib.mblink_tray_destroy(self._handle)

    def set_tray_tooltip(self, tooltip: str):
        self._ensure_alive()
        self._lib.mblink_tray_set_tooltip(self._handle, tooltip.encode("utf-8"))

    def set_tray_menu(self, items):
        self._ensure_alive()
        payload = json.dumps(items, ensure_ascii=False).encode("utf-8")
        rc = self._lib.mblink_tray_set_menu(self._handle, payload)
        if rc != 0:
            raise RuntimeError(f"设置 tray menu 失败: {rc}")

    def on_tray_click(self, callback):
        self._ensure_alive()

        @MBlinkVoidCallback
        def _cb(_ud):
            try:
                callback()
            except Exception:
                import traceback; traceback.print_exc()

        self._tray_callbacks.append(_cb)
        self._lib.mblink_tray_set_left_click_callback(self._handle, _cb, None)
        return callback

    def on_tray_menu(self, callback):
        self._ensure_alive()

        @MBlinkCallback
        def _cb(args_json, _ud):
            try:
                args_str = args_json.decode("utf-8") if args_json else "{}"
                args = json.loads(args_str)
                result = callback(args.get("id"))
                return self._wrap_result_json(result)
            except Exception as e:
                return self._wrap_error_json(e)

        self._tray_callbacks.append(_cb)
        self._lib.mblink_tray_set_menu_callback(self._handle, _cb, None)
        return callback

    def tray_action(self, item_id=None, *, background=False):
        """按 tray 菜单 id 注册处理函数。

        用法：
            @app.tray_action("show")
            def handle_show():
                ...

            @app.tray_action("work", background=True)
            def handle_work():
                ...
        """
        self._ensure_alive()

        if not self._tray_action_dispatch_enabled:
            self.enable_tray_action_dispatch()

        def _decorator(func):
            action_id = item_id or func.__name__
            self._tray_action_handlers[action_id] = {
                "func": func,
                "background": background,
            }
            return func

        return _decorator

    def enable_tray_action_dispatch(self):
        """启用基于 tray_action 注册表的统一分发。"""
        self._ensure_alive()

        if self._tray_action_dispatch_enabled:
            return None

        self._tray_action_dispatch_enabled = True

        @self.on_tray_menu
        def _dispatch(item_id):
            handler = self._tray_action_handlers.get(item_id)
            if not handler:
                return {"ok": False, "error": f"unhandled tray action: {item_id}", "id": item_id}

            func = handler["func"]
            if handler["background"]:
                def _runner():
                    try:
                        func()
                    except Exception:
                        import traceback; traceback.print_exc()

                thread = threading.Thread(target=_runner, daemon=True, name=f"mblink-tray-{item_id}")
                thread.start()
                return {"ok": True, "id": item_id, "background": True}

            result = func()
            if inspect.isawaitable(result):
                result = asyncio.run(result)
            return {"ok": True, "id": item_id, "result": result}

        return _dispatch


    def set_fullscreen(self, fullscreen: bool):
        self._ensure_alive()
        self._lib.mblink_set_fullscreen(self._handle, fullscreen)

    def set_resizable(self, resizable: bool):
        self._ensure_alive()
        self._lib.mblink_set_resizable(self._handle, resizable)

    def set_borderless(self, borderless: bool):
        self._ensure_alive()
        self._lib.mblink_set_borderless(self._handle, borderless)

    def set_always_on_top(self, on_top: bool):
        self._ensure_alive()
        self._lib.mblink_set_always_on_top(self._handle, on_top)

    # ========== 事件回调 ==========

    def on_resize(self, callback):
        """callback(width, height)"""
        self._ensure_alive()

        @MBlinkResizeCallback
        def _cb(w, h, _ud):
            try:
                callback(w, h)
            except Exception:
                import traceback; traceback.print_exc()
        self._callbacks.append(_cb)
        self._lib.mblink_on_resize(self._handle, _cb, None)
        return callback

    def on_close(self, callback):
        """callback()"""
        self._ensure_alive()

        @MBlinkVoidCallback
        def _cb(_ud):
            try:
                callback()
            except Exception:
                import traceback; traceback.print_exc()
        self._callbacks.append(_cb)
        self._lib.mblink_on_close(self._handle, _cb, None)
        return callback

    def on_close_request(self, callback):
        """callback() -> bool。返回 True 表示拦截关闭。"""
        self._ensure_alive()

        @MBlinkBoolCallback
        def _cb(_ud):
            try:
                return bool(callback())
            except Exception:
                import traceback; traceback.print_exc()
                return False

        self._callbacks.append(_cb)
        self._lib.mblink_on_close_request(self._handle, _cb, None)
        return callback

    def on_focus(self, callback):
        """callback()"""
        self._ensure_alive()

        @MBlinkVoidCallback
        def _cb(_ud):
            try:
                callback()
            except Exception:
                import traceback; traceback.print_exc()
        self._callbacks.append(_cb)
        self._lib.mblink_on_focus(self._handle, _cb, None)
        return callback

    def on_blur(self, callback):
        """callback()"""
        self._ensure_alive()

        @MBlinkVoidCallback
        def _cb(_ud):
            try:
                callback()
            except Exception:
                import traceback; traceback.print_exc()
        self._callbacks.append(_cb)
        self._lib.mblink_on_blur(self._handle, _cb, None)
        return callback

    def on_update(self, callback):
        """callback(delta_time)"""
        self._ensure_alive()

        @MBlinkUpdateCallback
        def _cb(dt, _ud):
            try:
                callback(dt)
            except Exception:
                import traceback; traceback.print_exc()
        self._callbacks.append(_cb)
        self._lib.mblink_on_update(self._handle, _cb, None)
        return callback

    # ========== 批量操作 ==========

    def batch(self):
        """上下文管理器：批量状态更新"""
        self._ensure_alive()
        return _BatchContext(self._lib, self._handle)

    # ========== DevTools ==========

    def devtools_open(self):
        self._ensure_alive()
        ret = self._devtools().mblink_devtools_open(self._handle)
        if ret != 0:
            err = self._lib.mblink_last_error()
            msg = err.decode("utf-8") if err else "unknown devtools open error"
            raise RuntimeError(msg)

    def devtools_close(self):
        self._ensure_alive()
        self._devtools().mblink_devtools_close(self._handle)

    def enable_devtools(self, *, http_mcp=False, port=0, auth_token=None, require_auth=True, bind_host=None):
        self._ensure_alive()
        if http_mcp:
            return self.devtools_http_session(
                port=port,
                auth_token=auth_token,
                require_auth=require_auth,
                bind_host=bind_host,
            )
        self.devtools_open()
        return None

    def devtools_http_session(self, *, port=0, auth_token=None, require_auth=True, bind_host=None):
        self._ensure_alive()
        devtools = self._devtools()
        options = devtools.mblink_devtools_default_http_options()
        keepalive = []
        if bind_host is not None:
            encoded = str(bind_host).encode("utf-8")
            keepalive.append(encoded)
            options.bind_host = encoded
        if auth_token is not None:
            encoded = str(auth_token).encode("utf-8")
            keepalive.append(encoded)
            options.auth_token = encoded
        options.port = int(port)
        options.require_auth = bool(require_auth)
        raw_info = MBlinkDevToolsHttpInfo()
        ret = devtools.mblink_devtools_http_start(
            self._handle, ctypes.byref(options), ctypes.byref(raw_info)
        )
        keepalive.clear()
        if ret != 0:
            err = self._lib.mblink_last_error()
            msg = err.decode("utf-8") if err else "unknown devtools http error"
            raise RuntimeError(msg)
        try:
            url = raw_info.url.decode("utf-8") if raw_info.url else ""
            token = raw_info.auth_token.decode("utf-8") if raw_info.auth_token else ""
            return DevToolsHttpSession(self, url, int(raw_info.port), token, bool(require_auth))
        finally:
            devtools.mblink_devtools_http_info_free(ctypes.byref(raw_info))

    def devtools_http_stop(self):
        self._ensure_alive()
        ret = self._devtools().mblink_devtools_http_stop(self._handle)
        if ret != 0:
            err = self._lib.mblink_last_error()
            msg = err.decode("utf-8") if err else "unknown devtools http stop error"
            raise RuntimeError(msg)
        return self


class _BatchContext:
    """with app.batch(): 批量更新状态"""
    def __init__(self, lib, handle):
        self._lib = lib
        self._handle = handle

    def __enter__(self):
        if not self._handle:
            raise RuntimeError("MBlink App 已销毁，不能进入 batch 上下文")
        self._lib.mblink_state_batch_begin(self._handle)
        return self

    def __exit__(self, *args):
        if self._handle:
            self._lib.mblink_state_batch_end(self._handle)


class DevToolsHttpSession:
    def __init__(self, app, url: str, port: int, auth_token: str, require_auth: bool):
        self._app = app
        self.url = url
        self.port = port
        self.auth_token = auth_token
        self.require_auth = require_auth
        self._next_id = 1

    def request(self, method: str, params=None):
        request_id = self._next_id
        self._next_id += 1
        payload = {"jsonrpc": "2.0", "id": request_id, "method": method}
        if params is not None:
            payload["params"] = params
        headers = {"Content-Type": "application/json"}
        if self.auth_token:
            headers["X-MBLINK-DevTools-Token"] = self.auth_token
        data = json.dumps(payload, ensure_ascii=False).encode("utf-8")
        req = urllib.request.Request(self.url, data=data, headers=headers, method="POST")
        state = {"done": False, "result": None, "error": None}

        def _worker():
            try:
                with urllib.request.urlopen(req) as response:
                    state["result"] = json.loads(response.read().decode("utf-8"))
            except BaseException as exc:
                state["error"] = exc
            finally:
                state["done"] = True

        worker = threading.Thread(
            target=_worker,
            daemon=True,
            name="mblink-devtools-http-request",
        )
        worker.start()
        while not state["done"]:
            if self._app is not None and not self._app._destroyed:
                self._app.poll()
            time.sleep(0.002)
        worker.join()
        if state["error"] is not None:
            raise state["error"]
        return state["result"]

    def stop(self):
        self._app.devtools_http_stop()
