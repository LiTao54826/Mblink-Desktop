"""
LightUI 高级 API 封装

提供简化的 API，整合 Window + Document + Runtime + EventLoop + HostBridge。

特性：
- 自动增量渲染（只在需要时重绘）
- 状态管理（线程安全）
- Python ↔ JS 双向通信（py.funcName() 调用方式）
- @app.bindable 装饰器自动注册函数
- ES 模块支持
- Fetch API 支持
- DevTools 支持

使用示例：
    import lightui as ui

    app = ui.App("My App", 800, 600)
    counter = app.state("counter", 0)

    @app.bindable
    def increment():
        counter.increment()
        return counter.get()

    app.load_html('''
        <button onclick="document.getElementById('v').textContent = py.increment()">+</button>
        <span id="v">0</span>
    ''')
    app.run()
"""

import sys
import os
import inspect
from typing import Any, Callable, Optional, Dict, Union, TypeVar
from pathlib import Path

# 尝试导入 C++ 扩展模块（从 bin 子目录）
try:
    _bin_path = os.path.join(os.path.dirname(__file__), 'bin')
    if _bin_path not in sys.path:
        sys.path.insert(0, _bin_path)
    import lightui_core as _core
except ImportError as e:
    raise ImportError(
        f"Failed to import lightui_core. Make sure the module is built. Error: {e}"
    )

# 类型别名
JsonValue = Union[None, bool, int, float, str, list, dict]
HostCallback = Callable[[JsonValue], JsonValue]
T = TypeVar('T')


class _IntStateWrapper:
    """IntState 的 Pythonic 包装器
    
    支持像操作普通变量一样操作状态：
        counter = app.state("counter", 0)
        counter += 1        # 等价于 counter.increment(1)
        counter -= 3        # 等价于 counter.decrement(3)
        counter *= 2        # 等价于 counter.multiply(2)
        counter.value = 10  # 等价于 counter.set(10)
        print(counter.value)  # 等价于 counter.get()
        print(int(counter))   # 隐式转换
    """

    def __init__(self, state):
        self._state = state

    @property
    def value(self):
        """获取当前值"""
        return self._state.get()

    @value.setter
    def value(self, v):
        """设置值"""
        self._state.set(v)

    def get(self):
        """获取当前值（兼容旧 API）"""
        return self._state.get()

    def set(self, value):
        """设置值（兼容旧 API）"""
        self._state.set(value)

    def increment(self, delta=1):
        self._state.increment(delta)

    def decrement(self, delta=1):
        """将值减少指定量（默认为 1）"""
        self._state.increment(-delta)

    def multiply(self, factor):
        self._state.multiply(factor)

    def watch(self, callback):
        return self._state.watch(callback)

    def unwatch(self, watch_id):
        self._state.unwatch(watch_id)

    @property
    def name(self):
        return self._state.name

    # ========== Pythonic 运算符 ==========

    def __iadd__(self, delta):
        """counter += n"""
        self._state.increment(delta)
        return self

    def __isub__(self, delta):
        """counter -= n"""
        self._state.increment(-delta)
        return self

    def __imul__(self, factor):
        """counter *= n"""
        self._state.multiply(factor)
        return self

    def __int__(self):
        return self._state.get()

    def __float__(self):
        return float(self._state.get())

    def __eq__(self, other):
        if isinstance(other, _IntStateWrapper):
            return self._state.get() == other._state.get()
        return self._state.get() == other

    def __lt__(self, other):
        v = other._state.get() if isinstance(other, _IntStateWrapper) else other
        return self._state.get() < v

    def __le__(self, other):
        v = other._state.get() if isinstance(other, _IntStateWrapper) else other
        return self._state.get() <= v

    def __gt__(self, other):
        v = other._state.get() if isinstance(other, _IntStateWrapper) else other
        return self._state.get() > v

    def __ge__(self, other):
        v = other._state.get() if isinstance(other, _IntStateWrapper) else other
        return self._state.get() >= v

    def __repr__(self):
        return f"IntState({self.name}={self.get()})"

    def __str__(self):
        return str(self._state.get())


def _make_wrapper(func: Callable, params: list) -> HostCallback:
    """为绑定函数创建参数解包包装器

    根据函数签名自动解包 JS 传入的参数：
    - 无参函数：忽略 JS 传入值
    - 单参函数：直接传递
    - 多参函数 + dict：尝试 **kwargs 解包
    - 多参函数 + list：尝试 *args 解包
    - 解包失败：回退为单参传递

    异常会被捕获并返回 {"error": "..."} 给 JS 端。
    """
    def wrapper(args: JsonValue) -> JsonValue:
        try:
            # 无参函数
            if not params:
                return func()
            # 单参函数
            elif len(params) == 1:
                return func(args)
            # 多参函数
            else:
                if isinstance(args, dict):
                    try:
                        return func(**args)
                    except TypeError:
                        return func(args)
                elif isinstance(args, list):
                    try:
                        return func(*args)
                    except TypeError:
                        return func(args)
                else:
                    return func(args)
        except Exception as e:
            return {"error": str(e)}
    return wrapper


class App:
    """
    LightUI 高级应用类

    整合 Window + Document + Runtime + EventLoop + HostBridge，
    提供简化的 API 来构建桌面应用。

    Attributes:
        window: 原生窗口对象
        document: DOM 文档对象
        runtime: JavaScript 运行时
        event_loop: 事件循环
        bridge: Python ↔ JS 通信桥接

    Example:
        >>> import lightui as ui
        >>> app = ui.App("My App", 800, 600)
        >>> counter = app.state("counter", 0)
        >>> @app.bindable
        ... def increment():
        ...     counter.increment()
        ...     return counter.get()
        >>> app.load_html('<button onclick="py.increment()">Click</button>')
        >>> app.run()
    """

    def __init__(
        self,
        title: str = "LightUI App",
        width: int = 800,
        height: int = 600,
        headless: bool = False,
        enable_devtools: bool = True,
        borderless: bool = False,
        transparent: bool = False,
        always_on_top: bool = False,
        resizable: bool = True,
        resize_border_width: int = 8
    ):
        """
        创建 LightUI 应用

        Args:
            title: 窗口标题
            width: 窗口宽度
            height: 窗口高度
            headless: 是否无头模式（不显示窗口，用于测试）
            enable_devtools: 是否启用开发者工具（F12 打开）
            borderless: 是否无边框窗口
            transparent: 是否透明窗口（不规则窗体，自动启用 borderless）
            always_on_top: 是否窗口置顶
            resizable: 是否可调整大小
            resize_border_width: 无边框窗口的调整大小边缘宽度（像素）
        """
        self._title = title
        self._width = width
        self._height = height
        self._headless = headless
        self._enable_devtools = enable_devtools

        # 创建核心组件
        self._app = _core.App(title, width, height)
        self._window = _core.Window(
            title, width, height,
            headless=headless,
            borderless=borderless,
            transparent=transparent,
            always_on_top=always_on_top,
            resizable=resizable,
            resize_border_width=resize_border_width
        )
        self._runtime = _core.Runtime()

        # 将 JS 运行时设置到窗口的 Document，这样内联脚本才能执行
        # 同时会初始化 WindowBindings（setTimeout, setInterval, requestAnimationFrame）
        self._window.set_js_runtime(self._runtime)

        # 使用窗口的 TaskScheduler 创建事件循环（支持 JS 定时器）
        task_scheduler = self._window.get_task_scheduler()
        self._event_loop = _core.EventLoop(task_scheduler)

        # 设置 QuickJS 运行时（会自动设置全局 EventLoop）
        self._event_loop.set_quickjs_runtime(self._runtime)

        # 设置 Window（用于自动增量渲染）
        self._event_loop.set_window(self._window)

        # 设置 StateManager，使 EventLoop 每帧自动处理状态队列
        self._event_loop.set_state_manager(self._app)

        # 初始化 FetchBindings（网络请求 API）
        self._runtime.init_fetch_bindings(task_scheduler)

        # 创建 HostBridge
        self._bridge = _core.HostBridge(self._runtime, self._app)

        # 将 HostBridge 传递给 EventLoop，使其在每帧自动刷新事件队列
        self._event_loop.set_host_bridge(self._bridge)

        # 初始化 DevTools
        if enable_devtools:
            self._devtools = _core.DevToolsManager.get_instance()
            self._devtools.initialize(self._window.document, self._window)
        else:
            self._devtools = None

        # 绑定函数缓存
        self._states: Dict[str, Any] = {}
        self._bound_functions: Dict[str, Callable] = {}

        # 防止重复清理
        self._cleaned_up = False

    def __enter__(self) -> "App":
        """上下文管理器入口"""
        return self

    def __exit__(self, exc_type, exc_val, exc_tb) -> None:
        """上下文管理器退出，自动清理资源"""
        self.cleanup()

    def cleanup(self) -> None:
        """
        清理资源（正确的清理顺序）

        按照 esm_loader 的清理顺序：
        1. 停止事件循环
        2. 关闭 DevTools
        3. 清理字体缓存
        4. 清理 DOM 绑定
        5. 关闭窗口

        多次调用是安全的（幂等操作）。
        """
        if self._cleaned_up:
            return

        try:
            if self._event_loop.is_running():
                self._event_loop.stop()
        except Exception:
            pass

        try:
            if self._devtools:
                self._devtools.shutdown()
        except Exception:
            pass

        try:
            _core.clear_font_cache()
        except Exception:
            pass

        try:
            _core.cleanup_dom_bindings(self._runtime)
        except Exception:
            pass

        try:
            self._window.close()
        except Exception:
            pass

        self._cleaned_up = True

    # ========== 窗口操作 ==========

    @property
    def window(self) -> "_core.Window":
        """获取原生窗口对象"""
        return self._window

    @property
    def document(self) -> Optional["_core.Document"]:
        """获取 DOM 文档对象"""
        return self._window.document

    @property
    def runtime(self) -> "_core.Runtime":
        """获取 JavaScript 运行时"""
        return self._runtime

    @property
    def event_loop(self) -> "_core.EventLoop":
        """获取事件循环"""
        return self._event_loop

    @property
    def bridge(self) -> "_core.HostBridge":
        """获取 Python ↔ JS 通信桥接"""
        return self._bridge

    def show(self) -> None:
        """显示窗口"""
        self._window.show()

    def hide(self) -> None:
        """隐藏窗口"""
        self._window.hide()

    def close(self) -> None:
        """关闭窗口"""
        self._window.close()

    def set_title(self, title: str) -> None:
        """设置窗口标题"""
        self._window.title = title
        self._title = title

    def set_size(self, width: int, height: int) -> None:
        """设置窗口大小"""
        self._window.set_size(width, height)
        self._width = width
        self._height = height

    # ========== 状态管理 ==========

    def state(self, name: str, initial: JsonValue = None):
        """
        创建或获取状态

        根据 initial 的类型自动返回对应的 State 子类：
        - int -> IntState（支持 increment/decrement/multiply）
        - str -> StringState（支持 append/prepend）
        - list -> ListState（支持 append/pop/shift/unshift）
        - dict -> DictState（支持 set_key/remove_key/keys）
        - 其他 -> State

        Args:
            name: 状态名称
            initial: 初始值

        Returns:
            对应类型的 State 对象

        Example:
            >>> counter = app.state("counter", 0)  # IntState
            >>> counter.increment()
            >>> print(counter.get())  # 1
        """
        if name in self._states:
            return self._states[name]

        state = self._app.state(name, initial)

        # 为 IntState 包装，添加 decrement 便捷方法
        if type(state).__name__ == "IntState":
            state = _IntStateWrapper(state)

        self._states[name] = state
        return state

    # ========== 函数绑定 ==========

    def bindable(self, func: Callable) -> Callable:
        """
        将 Python 函数注册为 JS 可调用函数（推荐方式）

        自动使用函数名作为 JS 调用名称，支持多参数自动解包。
        JS 端通过 py.funcName(args) 调用。

        参数解包规则：
        - def func()        → py.func()       无参调用
        - def func(x)       → py.func(42)     单参传递
        - def func(a, b)    → py.func({a:1, b:2})  dict → **kwargs
        - def func(a, b)    → py.func([1, 2])      list → *args
        - 解包失败时回退为单参传递

        Args:
            func: 要绑定的 Python 函数

        Returns:
            原始函数（不改变函数行为）

        Example:
            >>> @app.bindable
            ... def increment():
            ...     counter.increment()
            ...     return counter.get()
            # JS: py.increment()

            >>> @app.bindable
            ... def add(a, b):
            ...     return a + b
            # JS: py.add({a: 1, b: 2}) 或 py.add([1, 2])
        """
        name = func.__name__
        sig = inspect.signature(func)
        params = [
            p for p in sig.parameters.values()
            if p.kind in (
                inspect.Parameter.POSITIONAL_ONLY,
                inspect.Parameter.POSITIONAL_OR_KEYWORD,
                inspect.Parameter.KEYWORD_ONLY,
            )
        ]

        raw_wrapper = _make_wrapper(func, params)
        self._bridge.bind(name, raw_wrapper)
        self._bound_functions[name] = func
        return func

    def bind(self, name: str) -> Callable[[HostCallback], HostCallback]:
        """
        绑定 Python 函数供 JS 调用（显式命名方式）

        推荐使用 @app.bindable 代替此方法。
        绑定后，JS 可通过 py.name(args) 调用。

        Args:
            name: JS 端调用名称

        Returns:
            装饰器函数

        Example:
            >>> @app.bind("getData")
            ... def get_data(args):
            ...     return {"items": [1, 2, 3]}
            # JS: py.getData({})
        """
        def decorator(func: HostCallback) -> HostCallback:
            self._bridge.bind(name, func)
            self._bound_functions[name] = func
            return func
        return decorator

    def unbind(self, name: str) -> None:
        """
        解绑 Python 函数

        Args:
            name: 函数名
        """
        self._bridge.unbind(name)
        self._bound_functions.pop(name, None)

    def emit(self, event: str, data: JsonValue = None) -> None:
        """
        向 JS 端发送自定义事件

        JS 端通过 host.on(event, callback) 接收。
        事件会进入队列，在下一帧 EventLoop Update 时分发。

        Args:
            event: 事件名称
            data: 事件数据（None, bool, int, float, str, list, dict）

        Raises:
            ValueError: 事件名为空
            TypeError: data 不可序列化为 JSON

        Example:
            >>> app.emit("update", {"count": 42})
            # JS: host.on("update", function(data) { console.log(data.count); })
        """
        if not event:
            raise ValueError("event name must not be empty")

        # 类型检查
        allowed = (type(None), bool, int, float, str, list, dict)
        if data is not None and not isinstance(data, allowed):
            raise TypeError(
                f"data must be a JSON-serializable type (None, bool, int, float, str, list, dict), "
                f"got {type(data).__name__}"
            )

        self._bridge.emit(event, data)

    # ========== UI 加载 ==========

    def load_html(self, html: str) -> bool:
        """
        加载 HTML 字符串

        自动加载外部样式表和执行脚本。

        Args:
            html: HTML 内容

        Returns:
            是否成功
        """
        doc = self.document
        if doc:
            success = doc.load_html(html)
            if success:
                doc.load_external_stylesheets()
            return success
        return False

    def load_html_file(self, path: str) -> bool:
        """
        加载 HTML 文件

        自动设置基础路径、加载外部样式表和执行脚本。

        Args:
            path: 文件路径

        Returns:
            是否成功
        """
        doc = self.document
        if doc:
            abs_path = Path(path).resolve()
            base_path = str(abs_path.parent)
            doc.set_base_path(base_path)
            _core.set_image_base_path(base_path)

            success = doc.load_html_file(path)
            if success:
                doc.load_external_stylesheets()
            return success
        return False

    def load_js(self, code: str, filename: str = "<script>") -> JsonValue:
        """
        执行 JavaScript 代码

        Args:
            code: JavaScript 代码
            filename: 文件名（用于错误报告）

        Returns:
            执行结果
        """
        return self._runtime.eval(code, filename)

    def load_js_file(self, path: str) -> JsonValue:
        """
        执行 JavaScript 文件

        Args:
            path: 文件路径

        Returns:
            执行结果
        """
        return self._runtime.eval_file(path)

    def load_module(self, code: str, filename: str = "<module>") -> JsonValue:
        """
        执行 ES6 模块代码（支持 import/export）

        Args:
            code: JavaScript 模块代码
            filename: 文件名（用于错误报告）

        Returns:
            执行结果
        """
        return self._runtime.eval_module(code, filename)

    def register_module(self, name: str, code: str) -> None:
        """
        注册虚拟 ES 模块

        Args:
            name: 模块名（如 "preact", "preact/hooks"）
            code: 模块代码（必须包含 export 语句）
        """
        self._runtime.register_module(name, code)

    def set_module_base_path(self, path: str) -> None:
        """
        设置模块基础路径（用于解析相对路径的模块导入）

        Args:
            path: 基础路径
        """
        self._runtime.set_base_module_path(path)

    def load_preact(self, as_module: bool = True) -> bool:
        """
        加载 Preact 库（如果已嵌入）

        Args:
            as_module: 是否注册为 ES 模块（推荐）

        Returns:
            是否成功
        """
        # TODO: 实现嵌入库加载
        return False

    # ========== DevTools ==========

    def open_devtools(self) -> None:
        """打开开发者工具（F12）"""
        if self._devtools:
            self._devtools.open()

    def close_devtools(self) -> None:
        """关闭开发者工具"""
        if self._devtools:
            self._devtools.close()

    def toggle_devtools(self) -> None:
        """切换开发者工具显示状态"""
        if self._devtools:
            self._devtools.toggle()

    def is_devtools_open(self) -> bool:
        """检查开发者工具是否打开"""
        return self._devtools.is_open() if self._devtools else False

    # ========== 事件循环 ==========

    def run(self) -> None:
        """
        运行事件循环（阻塞）

        自动启用增量渲染：只在窗口需要重绘时才渲染。
        这会阻塞当前线程直到窗口关闭或调用 stop()。
        """
        self._event_loop.run()

    def run_once(self) -> None:
        """
        运行一次事件循环迭代（非阻塞）

        用于需要手动控制事件循环的场景。
        """
        self._event_loop.run_once()

    def run_for_seconds(self, seconds: float) -> None:
        """
        运行指定秒数后自动退出（用于测试）

        Args:
            seconds: 运行时长（秒）
        """
        elapsed = [0.0]
        def update_callback(delta_time: float):
            elapsed[0] += delta_time
            if elapsed[0] >= seconds:
                self.stop()
        self.on_update(update_callback)
        self.run()

    def stop(self) -> None:
        """停止事件循环"""
        self._event_loop.stop()

    def is_running(self) -> bool:
        """检查事件循环是否正在运行"""
        return self._event_loop.is_running()

    # ========== 事件回调 ==========

    def on_resize(self, callback: Callable[[int, int], None]) -> None:
        """设置窗口大小改变回调"""
        self._window.set_on_resize(callback)

    def on_close(self, callback: Callable[[], None]) -> None:
        """设置窗口关闭回调"""
        self._window.set_on_close(callback)

    def on_focus(self, callback: Callable[[], None]) -> None:
        """设置窗口获得焦点回调"""
        self._window.set_on_focus(callback)

    def on_blur(self, callback: Callable[[], None]) -> None:
        """设置窗口失去焦点回调"""
        self._window.set_on_blur(callback)

    def on_update(self, callback: Callable[[float], None]) -> None:
        """设置更新回调（每帧调用）"""
        self._event_loop.set_update_callback(callback)

    def on_render(self, callback: Callable[[], None]) -> None:
        """设置渲染回调（每帧调用）"""
        self._event_loop.set_render_callback(callback)

    def on_idle(self, callback: Callable[[], None]) -> None:
        """设置空闲回调"""
        self._event_loop.set_idle_callback(callback)

    # ========== 批量操作 ==========

    def batch(self):
        """
        返回批量操作上下文管理器

        在批量模式下，状态变化不会立即通知监听器，
        而是在退出批量模式时统一通知。

        Example:
            >>> with app.batch():
            ...     counter.increment()
            ...     counter.increment()
            # 监听器只会收到一次通知
        """
        return _core.BatchContext(self._app)


# 向后兼容别名
LightUIApp = App
