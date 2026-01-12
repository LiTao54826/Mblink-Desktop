"""
LightUI 高级 API 封装

提供简化的 API，整合 Window + Document + Runtime + EventLoop + HostBridge。

使用示例：
    from lightui import LightUIApp
    
    with LightUIApp("My App", 800, 600) as app:
        # 创建状态
        counter = app.state("counter", 0)
        
        # 绑定 Python 函数供 JS 调用
        @app.bind("increment")
        def increment(args):
            counter.increment()
            return counter.get()
        
        # 加载 UI
        app.load_html('''
            <html>
            <body>
                <button onclick="host.call('increment')">Click me</button>
            </body>
            </html>
        ''')
        
        # 运行
        app.run()
"""

import sys
import os
from typing import Any, Callable, Optional, Dict, Union, TypeVar

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


class LightUIApp:
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
        >>> app = LightUIApp("My App", 800, 600)
        >>> counter = app.state("counter", 0)
        >>> @app.bind("increment")
        ... def increment(args):
        ...     counter.increment()
        ...     return counter.get()
        >>> app.load_html("<button onclick='host.call(\"increment\")'>Click</button>")
        >>> app.run()
    """
    
    def __init__(
        self,
        title: str = "LightUI App",
        width: int = 800,
        height: int = 600,
        headless: bool = False
    ):
        """
        创建 LightUI 应用
        
        Args:
            title: 窗口标题
            width: 窗口宽度
            height: 窗口高度
            headless: 是否无头模式（不显示窗口，用于测试）
        """
        self._title = title
        self._width = width
        self._height = height
        self._headless = headless
        
        # 创建核心组件
        self._app = _core.App(title, width, height)
        self._window = _core.Window(title, width, height, headless)
        self._runtime = _core.Runtime()
        
        # 将 JS 运行时设置到窗口的 Document，这样内联脚本才能执行
        # 同时会初始化 WindowBindings（setTimeout, setInterval, requestAnimationFrame）
        self._window.set_js_runtime(self._runtime)
        
        # 使用窗口的 TaskScheduler 创建事件循环（支持 JS 定时器）
        task_scheduler = self._window.get_task_scheduler()
        self._event_loop = _core.EventLoop(task_scheduler)
        self._event_loop.set_quickjs_runtime(self._runtime)
        
        # 设置 StateManager，使 EventLoop 每帧自动处理状态队列
        # 这样后台线程的状态更新会在主线程统一处理（线程安全）
        self._event_loop.set_state_manager(self._app)
        
        # 创建 HostBridge
        self._bridge = _core.HostBridge(self._runtime, self._app)
        
        # 状态缓存
        self._states: Dict[str, Any] = {}
        
        # 绑定函数缓存
        self._bound_functions: Dict[str, Callable] = {}
    
    def __enter__(self) -> "LightUIApp":
        """上下文管理器入口"""
        return self
    
    def __exit__(self, exc_type, exc_val, exc_tb) -> None:
        """上下文管理器退出"""
        self.close()
        # 强制退出以避免资源清理时卡住
        import os
        os._exit(0)
    
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
        - int -> IntState
        - str -> StringState
        - list -> ListState
        - dict -> DictState
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
        self._states[name] = state
        return state
    
    # ========== 函数绑定 ==========
    
    def bind(self, name: str) -> Callable[[HostCallback], HostCallback]:
        """
        绑定 Python 函数供 JS 调用（装饰器）
        
        绑定后，JS 可以通过 host.call(name, args) 调用此函数。
        
        Args:
            name: 函数名
            
        Returns:
            装饰器函数
        
        Example:
            >>> @app.bind("getData")
            ... def get_data(args):
            ...     return {"items": [1, 2, 3]}
            # JS: const result = host.call("getData", {})
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
    
    # ========== UI 加载 ==========
    
    def load_html(self, html: str) -> bool:
        """
        加载 HTML 字符串
        
        Args:
            html: HTML 内容
            
        Returns:
            是否成功
        """
        doc = self.document
        if doc:
            return doc.load_html(html)
        return False
    
    def load_html_file(self, path: str) -> bool:
        """
        加载 HTML 文件
        
        Args:
            path: 文件路径
            
        Returns:
            是否成功
        """
        doc = self.document
        if doc:
            return doc.load_html_file(path)
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
        执行 ES6 模块代码
        
        Args:
            code: JavaScript 模块代码
            filename: 文件名（用于错误报告）
            
        Returns:
            执行结果
        """
        return self._runtime.eval_module(code, filename)
    
    # ========== 事件循环 ==========
    
    def run(self) -> None:
        """
        运行事件循环（阻塞）
        
        这会阻塞当前线程直到窗口关闭或调用 stop()。
        """
        self._event_loop.run()
    
    def run_once(self) -> None:
        """
        运行一次事件循环迭代（非阻塞）
        
        用于需要手动控制事件循环的场景。
        """
        self._event_loop.run_once()
    
    def stop(self) -> None:
        """停止事件循环"""
        self._event_loop.stop()
    
    def is_running(self) -> bool:
        """检查事件循环是否正在运行"""
        return self._event_loop.is_running()
    
    # ========== 事件回调 ==========
    
    def on_resize(self, callback: Callable[[int, int], None]) -> None:
        """
        设置窗口大小改变回调
        
        Args:
            callback: 回调函数，接收 (width, height)
        """
        self._window.set_on_resize(callback)
    
    def on_close(self, callback: Callable[[], None]) -> None:
        """
        设置窗口关闭回调
        
        Args:
            callback: 回调函数
        """
        self._window.set_on_close(callback)
    
    def on_focus(self, callback: Callable[[], None]) -> None:
        """
        设置窗口获得焦点回调
        
        Args:
            callback: 回调函数
        """
        self._window.set_on_focus(callback)
    
    def on_blur(self, callback: Callable[[], None]) -> None:
        """
        设置窗口失去焦点回调
        
        Args:
            callback: 回调函数
        """
        self._window.set_on_blur(callback)
    
    def on_update(self, callback: Callable[[float], None]) -> None:
        """
        设置更新回调（每帧调用）
        
        Args:
            callback: 回调函数，接收 delta_time
        """
        self._event_loop.set_update_callback(callback)
    
    def on_render(self, callback: Callable[[], None]) -> None:
        """
        设置渲染回调（每帧调用）
        
        Args:
            callback: 回调函数
        """
        self._event_loop.set_render_callback(callback)
    
    def on_idle(self, callback: Callable[[], None]) -> None:
        """
        设置空闲回调
        
        Args:
            callback: 回调函数
        """
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
            ...     counter.increment()
            # 监听器只会收到一次通知
        """
        return _core.BatchContext(self._app)


# 导出便捷别名
App = LightUIApp
