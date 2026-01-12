"""
LightUI Python Binding Type Stubs

提供 IDE 自动补全和类型检查支持。
"""

from typing import Any, Callable, Dict, List, Optional, Union

# 类型别名
JsonValue = Union[None, bool, int, float, str, List[Any], Dict[str, Any]]
WatchCallback = Callable[[JsonValue], None]

def version() -> str:
    """获取 LightUI 版本号"""
    ...

class State:
    """状态基类"""
    
    @property
    def name(self) -> str:
        """状态名称"""
        ...
    
    def get(self) -> JsonValue:
        """获取状态值"""
        ...
    
    def set(self, value: JsonValue) -> None:
        """设置状态值"""
        ...
    
    def watch(self, callback: WatchCallback) -> int:
        """
        监听状态变化
        
        Args:
            callback: 状态变化时调用的回调函数
            
        Returns:
            watch_id: 用于取消监听的 ID
        """
        ...
    
    def unwatch(self, watch_id: int) -> None:
        """
        取消监听
        
        Args:
            watch_id: watch() 返回的 ID
        """
        ...

class IntState(State):
    """整数状态，支持原子操作"""
    
    def get(self) -> int:
        """获取整数值"""
        ...
    
    def set(self, value: int) -> None:
        """设置整数值"""
        ...
    
    def increment(self, delta: int = 1) -> None:
        """
        原子增加
        
        Args:
            delta: 增量，默认为 1
        """
        ...
    
    def multiply(self, factor: float) -> None:
        """
        原子乘法
        
        Args:
            factor: 乘数
        """
        ...

class StringState(State):
    """字符串状态，支持字符串操作"""
    
    def get(self) -> str:
        """获取字符串值"""
        ...
    
    def set(self, value: str) -> None:
        """设置字符串值"""
        ...
    
    def append(self, suffix: str) -> None:
        """追加文本"""
        ...
    
    def prepend(self, prefix: str) -> None:
        """前置文本"""
        ...
    
    def __len__(self) -> int:
        """获取字符串长度"""
        ...

class ListState(State):
    """列表状态，支持数组操作"""
    
    def get(self) -> List[Any]:
        """获取列表值"""
        ...
    
    def append(self, item: JsonValue) -> None:
        """添加元素到末尾"""
        ...
    
    def pop(self) -> None:
        """移除最后一个元素"""
        ...
    
    def remove(self, index: int) -> None:
        """移除指定位置的元素"""
        ...
    
    def clear(self) -> None:
        """清空列表"""
        ...
    
    def __len__(self) -> int:
        """获取列表长度"""
        ...
    
    def __getitem__(self, index: int) -> JsonValue:
        """获取指定位置的元素"""
        ...
    
    def __setitem__(self, index: int, value: JsonValue) -> None:
        """设置指定位置的元素"""
        ...

class DictState(State):
    """字典状态，支持对象操作"""
    
    def get(self) -> Dict[str, Any]:
        """获取字典值"""
        ...
    
    def set_key(self, key: str, value: JsonValue) -> None:
        """设置键值"""
        ...
    
    def remove_key(self, key: str) -> None:
        """移除键"""
        ...
    
    def clear(self) -> None:
        """清空字典"""
        ...
    
    def keys(self) -> List[str]:
        """获取所有键"""
        ...
    
    def __getitem__(self, key: str) -> JsonValue:
        """获取指定键的值"""
        ...
    
    def __setitem__(self, key: str, value: JsonValue) -> None:
        """设置指定键的值"""
        ...
    
    def __contains__(self, key: str) -> bool:
        """检查键是否存在"""
        ...

class App:
    """LightUI 应用主类"""
    
    def __init__(
        self,
        title: str = "LightUI App",
        width: int = 800,
        height: int = 600
    ) -> None:
        """
        创建 LightUI 应用
        
        Args:
            title: 窗口标题
            width: 窗口宽度
            height: 窗口高度
        """
        ...
    
    def state(
        self,
        name: str,
        initial: JsonValue = None
    ) -> Union[State, IntState, StringState, ListState, DictState]:
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
        """
        ...
    
    def bind(self, name: str, func: Callable[..., Any]) -> None:
        """
        绑定 Python 函数供 JS 调用
        
        Args:
            name: 函数名称
            func: Python 函数
        """
        ...
    
    def unbind(self, name: str) -> None:
        """
        解绑函数
        
        Args:
            name: 函数名称
        """
        ...
    
    def load_file(self, path: str) -> None:
        """
        加载 JS 文件
        
        Args:
            path: 文件路径
        """
        ...
    
    def load_js(self, code: str) -> None:
        """
        加载 JS 代码
        
        Args:
            code: JS 代码字符串
        """
        ...
    
    def run(self) -> None:
        """运行事件循环"""
        ...
    
    def stop(self) -> None:
        """停止事件循环"""
        ...
    
    def batch_begin(self) -> None:
        """进入批量模式"""
        ...
    
    def batch_end(self) -> None:
        """退出批量模式"""
        ...
    
    def process_queue(self) -> None:
        """处理队列（测试用）"""
        ...
    
    def __enter__(self) -> "App":
        ...
    
    def __exit__(self, exc_type: Any, exc_val: Any, exc_tb: Any) -> None:
        ...

class BatchContext:
    """批量操作上下文管理器"""
    
    def __init__(self, app: App) -> None:
        """
        创建批量操作上下文
        
        Args:
            app: App 实例
        """
        ...
    
    def __enter__(self) -> "BatchContext":
        ...
    
    def __exit__(self, exc_type: Any, exc_val: Any, exc_tb: Any) -> None:
        ...


# ========== Phase 2: Window 绑定 ==========

from typing import Tuple

ResizeCallback = Callable[[int, int], None]
CloseCallback = Callable[[], None]
FocusCallback = Callable[[], None]
UpdateCallback = Callable[[float], None]
RenderCallback = Callable[[], None]
IdleCallback = Callable[[], None]

class Element:
    """DOM 元素"""
    
    @property
    def tag_name(self) -> str:
        """标签名"""
        ...
    
    @property
    def id(self) -> str:
        """元素 ID"""
        ...
    
    @id.setter
    def id(self, value: str) -> None:
        ...
    
    @property
    def class_name(self) -> str:
        """CSS 类名"""
        ...
    
    @class_name.setter
    def class_name(self, value: str) -> None:
        ...
    
    @property
    def inner_html(self) -> str:
        """内部 HTML"""
        ...
    
    @inner_html.setter
    def inner_html(self, value: str) -> None:
        ...
    
    @property
    def text_content(self) -> str:
        """文本内容"""
        ...
    
    @text_content.setter
    def text_content(self, value: str) -> None:
        ...
    
    def get_attribute(self, name: str) -> str:
        """获取属性值"""
        ...
    
    def set_attribute(self, name: str, value: str) -> None:
        """设置属性值"""
        ...
    
    def remove_attribute(self, name: str) -> None:
        """移除属性"""
        ...
    
    def query_selector(self, selector: str) -> Optional["Element"]:
        """查询第一个匹配的子元素"""
        ...
    
    def query_selector_all(self, selector: str) -> List["Element"]:
        """查询所有匹配的子元素"""
        ...
    
    def is_valid(self) -> bool:
        """检查元素是否有效"""
        ...

class Document:
    """DOM 文档"""
    
    @property
    def body(self) -> Optional[Element]:
        """body 元素"""
        ...
    
    def create_element(self, tag_name: str) -> Optional[Element]:
        """创建元素"""
        ...
    
    def query_selector(self, selector: str) -> Optional[Element]:
        """查询第一个匹配的元素"""
        ...
    
    def query_selector_all(self, selector: str) -> List[Element]:
        """查询所有匹配的元素"""
        ...
    
    def get_element_by_id(self, id: str) -> Optional[Element]:
        """根据 ID 获取元素"""
        ...
    
    def load_html(self, html: str) -> bool:
        """加载 HTML 字符串"""
        ...
    
    def load_html_file(self, path: str) -> bool:
        """加载 HTML 文件"""
        ...
    
    def save_html(self) -> str:
        """保存为 HTML 字符串"""
        ...
    
    def is_valid(self) -> bool:
        """检查文档是否有效"""
        ...

class Window:
    """原生窗口"""
    
    def __init__(
        self,
        title: str = "LightUI Window",
        width: int = 800,
        height: int = 600,
        headless: bool = False
    ) -> None:
        """
        创建窗口
        
        Args:
            title: 窗口标题
            width: 窗口宽度
            height: 窗口高度
            headless: 是否无头模式（不显示窗口）
        """
        ...
    
    @property
    def title(self) -> str:
        """窗口标题"""
        ...
    
    @title.setter
    def title(self, value: str) -> None:
        ...
    
    @property
    def size(self) -> Tuple[int, int]:
        """窗口大小 (width, height)"""
        ...
    
    @property
    def position(self) -> Tuple[int, int]:
        """窗口位置 (x, y)"""
        ...
    
    @property
    def document(self) -> Optional[Document]:
        """关联的文档"""
        ...
    
    def show(self) -> None:
        """显示窗口"""
        ...
    
    def hide(self) -> None:
        """隐藏窗口"""
        ...
    
    def close(self) -> None:
        """关闭窗口"""
        ...
    
    def set_size(self, width: int, height: int) -> None:
        """设置窗口大小"""
        ...
    
    def set_position(self, x: int, y: int) -> None:
        """设置窗口位置"""
        ...
    
    def set_fullscreen(self, fullscreen: bool) -> None:
        """设置全屏模式"""
        ...
    
    def set_resizable(self, resizable: bool) -> None:
        """设置是否可调整大小"""
        ...
    
    def set_borderless(self, borderless: bool) -> None:
        """设置无边框模式"""
        ...
    
    def set_always_on_top(self, on_top: bool) -> None:
        """设置窗口置顶"""
        ...
    
    def minimize(self) -> None:
        """最小化窗口"""
        ...
    
    def maximize(self) -> None:
        """最大化窗口"""
        ...
    
    def restore(self) -> None:
        """恢复窗口"""
        ...
    
    def should_close(self) -> bool:
        """检查窗口是否应该关闭"""
        ...
    
    def render(self) -> None:
        """渲染窗口内容"""
        ...
    
    def swap_buffers(self) -> None:
        """交换缓冲区"""
        ...
    
    def set_on_resize(self, callback: ResizeCallback) -> None:
        """设置窗口大小改变回调"""
        ...
    
    def set_on_close(self, callback: CloseCallback) -> None:
        """设置窗口关闭回调"""
        ...
    
    def set_on_focus(self, callback: FocusCallback) -> None:
        """设置窗口获得焦点回调"""
        ...
    
    def set_on_blur(self, callback: FocusCallback) -> None:
        """设置窗口失去焦点回调"""
        ...
    
    def is_valid(self) -> bool:
        """检查窗口是否有效"""
        ...

class EventLoop:
    """事件循环"""
    
    def __init__(self) -> None:
        """创建事件循环"""
        ...
    
    def run(self) -> None:
        """运行事件循环（阻塞）"""
        ...
    
    def stop(self) -> None:
        """停止事件循环"""
        ...
    
    def run_once(self) -> None:
        """运行一次事件循环迭代"""
        ...
    
    def is_running(self) -> bool:
        """检查是否正在运行"""
        ...
    
    def should_quit(self) -> bool:
        """检查是否应该退出"""
        ...
    
    def set_update_callback(self, callback: UpdateCallback) -> None:
        """设置更新回调（每帧调用）"""
        ...
    
    def set_render_callback(self, callback: RenderCallback) -> None:
        """设置渲染回调（每帧调用）"""
        ...
    
    def set_idle_callback(self, callback: IdleCallback) -> None:
        """设置空闲回调"""
        ...

class Runtime:
    """QuickJS JavaScript 运行时"""
    
    def __init__(self) -> None:
        """创建 JavaScript 运行时"""
        ...
    
    def eval(self, code: str, filename: str = "<eval>") -> JsonValue:
        """
        执行 JavaScript 代码
        
        Args:
            code: JavaScript 代码
            filename: 文件名（用于错误报告）
            
        Returns:
            执行结果
        """
        ...
    
    def eval_module(self, code: str, filename: str = "<module>") -> JsonValue:
        """
        执行 ES6 模块代码
        
        Args:
            code: JavaScript 模块代码
            filename: 文件名（用于错误报告）
            
        Returns:
            执行结果
        """
        ...
    
    def eval_file(self, path: str) -> JsonValue:
        """
        执行 JavaScript 文件
        
        Args:
            path: 文件路径
            
        Returns:
            执行结果
        """
        ...


HostCallback = Callable[[JsonValue], JsonValue]

class HostBridge:
    """Python ↔ JS 通信桥接类
    
    允许 Python 函数被 JS 调用，实现双向通信。
    JS 端通过 host.call(name, args) 调用绑定的 Python 函数。
    """
    
    def __init__(self, runtime: Runtime, app: App) -> None:
        """
        创建 HostBridge
        
        Args:
            runtime: QuickJS 运行时
            app: LightUI App 实例（提供 StateManager）
        """
        ...
    
    def bind(self, name: str, func: HostCallback) -> None:
        """
        绑定 Python 函数供 JS 调用
        
        绑定后，JS 可以通过 host.call(name, args) 调用此函数。
        
        Args:
            name: 函数名
            func: Python 函数，接收 JSON 兼容参数，返回 JSON 兼容值
        
        Example:
            >>> def get_data(args):
            ...     return {"items": [1, 2, 3]}
            >>> bridge.bind("getData", get_data)
            # JS: const result = host.call("getData", {})
        """
        ...
    
    def unbind(self, name: str) -> None:
        """
        解绑 Python 函数
        
        Args:
            name: 函数名
        """
        ...
    
    def call(self, name: str, args: JsonValue = None) -> JsonValue:
        """
        调用已注册的函数
        
        Args:
            name: 函数名
            args: 参数
            
        Returns:
            函数返回值
        """
        ...
    
    def is_valid(self) -> bool:
        """检查桥接是否已正确初始化"""
        ...
