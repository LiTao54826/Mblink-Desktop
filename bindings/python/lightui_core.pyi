"""
lightui_core — LightUI 低级 Python 绑定（pybind11）

本模块通过 pybind11 直接暴露 C++ 对象，适合需要精细控制的高级用法。
普通开发者请使用高级封装：``from lightui import App``

模块结构
--------
- **状态管理**：App / State / IntState / StringState / ListState / DictState
- **窗口**：Window / Document / Element
- **JS 运行时**：Runtime / TaskScheduler
- **通信桥接**：HostBridge
- **事件循环**：EventLoop
- **开发工具**：DevToolsManager
- **全局函数**：version / set_image_base_path / clear_font_cache / cleanup_dom_bindings

典型用法（低级 API）
--------------------
::

    import lightui_core as lui

    app     = lui.App("title", 800, 600)       # 仅状态管理器，无窗口
    runtime = lui.Runtime()
    window  = lui.Window("title", 800, 600)
    bridge  = lui.HostBridge(runtime, app)

    window.set_js_runtime(runtime)
    scheduler = window.get_task_scheduler()

    loop = lui.EventLoop(scheduler)
    loop.set_quickjs_runtime(runtime)
    loop.set_window(window)
    loop.set_state_manager(app)
    loop.set_host_bridge(bridge)

    runtime.eval('console.log("hello")')
    loop.run()                                 # 阻塞，结束后进程退出
"""

from typing import Any, Callable, Dict, List, Optional, Tuple, Union, overload

# ---------------------------------------------------------------------------
# 类型别名
# ---------------------------------------------------------------------------
JsonValue = Union[None, bool, int, float, str, List[Any], Dict[str, Any]]
"""可在 Python ↔ JS 之间传递的 JSON 兼容类型。"""

WatchCallback = Callable[[JsonValue], None]
"""状态变化监听回调：``callback(new_value) -> None``"""

HostCallback = Callable[[JsonValue], JsonValue]
"""JS 调用 Python 函数的签名：``callback(args) -> result``"""

ResizeCallback = Callable[[int, int], None]
"""窗口大小变化回调：``callback(width, height) -> None``"""

CloseCallback = Callable[[], None]
"""窗口关闭回调：``callback() -> None``"""

FocusCallback = Callable[[], None]
"""窗口焦点回调：``callback() -> None``"""

UpdateCallback = Callable[[float], None]
"""每帧更新回调：``callback(delta_time_seconds) -> None``"""

RenderCallback = Callable[[], None]
"""渲染回调：``callback() -> None``"""

IdleCallback = Callable[[], None]
"""空闲回调：``callback() -> None``"""

# ---------------------------------------------------------------------------
# 全局函数
# ---------------------------------------------------------------------------

def version() -> str:
    """返回 LightUI 版本字符串，例如 ``"0.5.0"``。"""
    ...

def cleanup_dom_bindings(runtime: "Runtime") -> None:
    """清理 DOM 绑定和全局 Node*→JSValue 映射表。

    必须在销毁 ``Runtime`` 之前调用，否则会有悬空指针风险。
    """
    ...

def clear_font_cache() -> None:
    """清空字体管理器缓存，释放已加载字体的内存。"""
    ...

def set_image_base_path(path: str) -> None:
    """设置图片加载的基础路径，相对路径的 ``<img>`` 会基于此解析。"""
    ...


# ---------------------------------------------------------------------------
# State 类层次
# ---------------------------------------------------------------------------

class State:
    """通用状态基类，对应 C++ ``PyState``。

    通过 ``App.state(name, initial)`` 获取，根据 ``initial`` 类型自动返回具体子类：

    - ``int``      → ``IntState``
    - ``str``      → ``StringState``
    - ``list``     → ``ListState``
    - ``dict``     → ``DictState``
    - 其他 / None  → ``State``

    状态变化会进入队列，由 ``EventLoop`` 在每帧调用 ``App.process_queue()``
    统一推送到 JS 端，保证线程安全。
    """

    @property
    def name(self) -> str:
        """状态名称（创建时传入的 ``name`` 字符串）。"""
        ...

    def get(self) -> JsonValue:
        """读取当前值，返回 Python 原生类型（int / str / list / dict / bool / None）。"""
        ...

    def set(self, value: JsonValue) -> None:
        """写入新值。写入后进入队列，等待主线程 ``process_queue()`` 推送到 JS。"""
        ...

    def watch(self, callback: WatchCallback) -> int:
        """注册变化监听器，状态被修改时异步调用 ``callback(new_value)``。

        Returns
        -------
        int
            watch_id，传给 ``unwatch()`` 可取消。
        """
        ...

    def unwatch(self, watch_id: int) -> None:
        """取消指定 watch_id 的监听。"""
        ...


class IntState(State):
    """整数状态（``initial`` 为 ``int`` 时由 ``App.state()`` 返回）。

    提供原子算术操作，避免 read-modify-write 竞态。
    """

    def get(self) -> int:
        """读取整数值。"""
        ...

    def set(self, value: int) -> None:
        """写入整数值。"""
        ...

    def increment(self, delta: int = 1) -> None:
        """原子加法：``value += delta``（delta 可为负数实现减法）。"""
        ...

    def multiply(self, factor: float) -> None:
        """原子乘法：``value *= factor``。"""
        ...


class StringState(State):
    """字符串状态（``initial`` 为 ``str`` 时由 ``App.state()`` 返回）。"""

    def get(self) -> str:
        """读取字符串值。"""
        ...

    def set(self, value: str) -> None:
        """写入字符串值。"""
        ...

    def append(self, suffix: str) -> None:
        """在末尾追加文本：``value += suffix``。"""
        ...

    def prepend(self, prefix: str) -> None:
        """在开头前置文本：``value = prefix + value``。"""
        ...

    def __len__(self) -> int:
        """返回字符串字节长度。"""
        ...



class ListState(State):
    """列表状态（``initial`` 为 ``list`` 时由 ``App.state()`` 返回）。

    提供类数组操作接口，支持 ``__getitem__`` / ``__setitem__`` / ``__len__``。
    """

    def get(self) -> List[Any]:
        """以 Python list 返回完整列表快照。"""
        ...

    def append(self, item: JsonValue) -> None:
        """向末尾追加元素（等价于 JS ``Array.push``）。"""
        ...

    def pop(self) -> None:
        """移除末尾元素（等价于 JS ``Array.pop``）。"""
        ...

    def shift(self) -> None:
        """移除并丢弃第一个元素（等价于 JS ``Array.shift``）。"""
        ...

    def unshift(self, item: JsonValue) -> None:
        """在开头插入元素（等价于 JS ``Array.unshift``）。"""
        ...

    def remove(self, index: int) -> None:
        """移除指定索引处的元素。"""
        ...

    def clear(self) -> None:
        """清空列表。"""
        ...

    def __len__(self) -> int: ...
    def __getitem__(self, index: int) -> JsonValue: ...
    def __setitem__(self, index: int, value: JsonValue) -> None: ...


class DictState(State):
    """字典状态（``initial`` 为 ``dict`` 时由 ``App.state()`` 返回）。

    提供键值操作接口，支持 ``__getitem__`` / ``__setitem__`` / ``__contains__``。
    """

    def get(self) -> Dict[str, Any]:
        """以 Python dict 返回完整字典快照。"""
        ...

    def set_key(self, key: str, value: JsonValue) -> None:
        """设置或更新指定键的值，等价于 ``obj[key] = value``。"""
        ...

    def remove_key(self, key: str) -> None:
        """删除指定键。"""
        ...

    def clear(self) -> None:
        """清空字典中所有键值对。"""
        ...

    def keys(self) -> List[str]:
        """返回所有键的列表。"""
        ...

    def __getitem__(self, key: str) -> JsonValue: ...
    def __setitem__(self, key: str, value: JsonValue) -> None: ...
    def __contains__(self, key: str) -> bool: ...


# ---------------------------------------------------------------------------
# App — 纯状态管理器（无窗口，仅 pybind11 低级 API 使用）
# ---------------------------------------------------------------------------

class App:
    """LightUI 低级状态管理器。

    .. note::
        这里的 ``App`` 仅负责状态（StateManager）和函数绑定注册表。
        **不**创建窗口、**不**运行事件循环。
        ``load_file``、``load_js``、``run`` 会直接抛出 ``RuntimeError``。

        需要完整窗口应用请使用高级封装：``from lightui import App``

    典型用法
    --------
    ::

        app     = lui.App("title", 800, 600)
        counter = app.state("counter", 0)   # IntState
        app.process_queue()                 # 单元测试时手动刷新队列
    """

    def __init__(
        self,
        title: str = "LightUI App",
        width: int = 800,
        height: int = 600,
    ) -> None: ...

    def state(
        self,
        name: str,
        initial: JsonValue = None,
    ) -> Union[State, IntState, StringState, ListState, DictState]:
        """创建或获取状态对象。

        根据 ``initial`` 的 Python 类型自动选择子类：

        ========  ===========
        Python    返回类型
        ========  ===========
        ``int``   IntState
        ``str``   StringState
        ``list``  ListState
        ``dict``  DictState
        其他      State
        ========  ===========

        若同名状态已存在，直接返回已有实例。
        """
        ...

    def bind(self, name: str, func: Callable[[JsonValue], JsonValue]) -> None:
        """向内部注册表添加函数（不直接注册到 JS）。

        通常由 ``HostBridge.bind()`` 间接调用，不需要手动使用。
        """
        ...

    def unbind(self, name: str) -> None:
        """从内部注册表移除函数。"""
        ...

    def batch_begin(self) -> None:
        """进入批量模式：暂停 JS 状态通知，积累变更。"""
        ...

    def batch_end(self) -> None:
        """离开批量模式：一次性推送所有积累的变更到 JS。"""
        ...

    def process_queue(self) -> None:
        """手动处理状态更新队列（主线程调用）。

        正常运行时由 ``EventLoop`` 每帧自动调用；
        单元测试或无事件循环场景下需手动调用。
        """
        ...

    def __enter__(self) -> "App": ...
    def __exit__(self, exc_type: Any, exc_val: Any, exc_tb: Any) -> None: ...


class BatchContext:
    """批量状态更新的上下文管理器（with 语句支持）。

    ::

        ctx = lui.BatchContext(app)
        with ctx:
            counter.set(1)
            name.set("hello")
        # 此处一次性推送所有变更
    """

    def __init__(self, app: App) -> None: ...
    def __enter__(self) -> "BatchContext": ...
    def __exit__(self, exc_type: Any, exc_val: Any, exc_tb: Any) -> None: ...


# ---------------------------------------------------------------------------
# TaskScheduler
# ---------------------------------------------------------------------------

class TaskScheduler:
    """JS 任务调度器，驱动 ``setTimeout`` / ``setInterval`` / ``requestAnimationFrame``。

    由 ``Window`` 内部创建，通过 ``window.get_task_scheduler()`` 获取，
    再传给 ``EventLoop(task_scheduler)`` 和 ``Runtime.init_fetch_bindings()``。
    """

    def __init__(self) -> None: ...


# ---------------------------------------------------------------------------
# Element / Document
# ---------------------------------------------------------------------------

class Element:
    """DOM 元素包装，对应 C++ ``lightui::Element``。

    通过 ``Document.query_selector()`` / ``get_element_by_id()`` 等方法获取。
    """

    @property
    def tag_name(self) -> str:
        """标签名（大写），如 ``"DIV"``、``"BUTTON"``。"""
        ...

    @property
    def id(self) -> str: ...
    @id.setter
    def id(self, value: str) -> None: ...

    @property
    def class_name(self) -> str:
        """``class`` 属性字符串，多个类名以空格分隔。"""
        ...
    @class_name.setter
    def class_name(self, value: str) -> None: ...

    @property
    def inner_html(self) -> str:
        """元素的 innerHTML（包含子标签的 HTML 字符串）。"""
        ...
    @inner_html.setter
    def inner_html(self, value: str) -> None: ...

    @property
    def text_content(self) -> str:
        """元素的纯文本内容（不含子标签）。"""
        ...
    @text_content.setter
    def text_content(self, value: str) -> None: ...

    def get_attribute(self, name: str) -> str:
        """获取属性值，属性不存在时返回空字符串。"""
        ...

    def set_attribute(self, name: str, value: str) -> None:
        """设置属性值。"""
        ...

    def remove_attribute(self, name: str) -> None:
        """移除属性。"""
        ...

    def query_selector(self, selector: str) -> Optional["Element"]:
        """在子树中查找第一个匹配选择器的元素。"""
        ...

    def query_selector_all(self, selector: str) -> List["Element"]:
        """在子树中查找所有匹配选择器的元素。"""
        ...

    def is_valid(self) -> bool:
        """元素是否仍然有效（未被销毁）。"""
        ...


class Document:
    """DOM 文档包装，对应 C++ ``lightui::Document``。

    通过 ``Window.document`` 属性获取。加载 HTML 后会自动执行 ``<script>`` 标签。
    """

    @property
    def body(self) -> Optional[Element]:
        """文档的 ``<body>`` 元素。"""
        ...

    def create_element(self, tag_name: str) -> Optional[Element]:
        """创建新元素，``tag_name`` 不区分大小写。"""
        ...

    def query_selector(self, selector: str) -> Optional[Element]:
        """在 body 子树中查找第一个匹配选择器的元素。"""
        ...

    def query_selector_all(self, selector: str) -> List[Element]:
        """在 body 子树中查找所有匹配选择器的元素。"""
        ...

    def get_element_by_id(self, id: str) -> Optional[Element]:
        """通过 id 属性查找元素。"""
        ...

    def load_html(self, html: str) -> bool:
        """加载 HTML 字符串，重建 DOM 树，并自动执行 ``<script>`` 标签。"""
        ...

    def load_html_file(self, path: str) -> bool:
        """从文件加载 HTML，重建 DOM 树，并自动执行 ``<script>`` 标签。"""
        ...

    def save_html(self) -> str:
        """将当前 DOM 序列化为 HTML 字符串。"""
        ...

    def set_base_path(self, path: str) -> None:
        """设置 URL 基础路径，影响 ``<img>``、``<link>`` 等相对路径解析。"""
        ...

    def load_external_stylesheets(self) -> None:
        """主动加载文档中 ``<link rel="stylesheet">`` 引用的外部样式表。"""
        ...

    def execute_scripts(self) -> None:
        """手动执行文档中所有 ``<script>`` 标签。（``load_html`` 已自动调用）"""
        ...

    def is_valid(self) -> bool:
        """文档对象是否有效。"""
        ...


# ---------------------------------------------------------------------------
# Window
# ---------------------------------------------------------------------------

class Window:
    """原生 SDL 窗口包装，对应 C++ ``PyWindow``。

    创建时自动注册到 ``WindowManager``，销毁时自动注销。
    内部持有 ``Document``、``TaskScheduler`` 和（设置后的）``WindowBindings``。

    参数
    ----
    title
        窗口标题。
    width / height
        初始宽高（像素）。
    headless
        无头模式（不显示窗口，用于测试）。
    borderless
        无边框模式，配合 ``-webkit-app-region: drag`` CSS 实现自定义标题栏。
    transparent
        透明背景（需要合成器支持）。
    always_on_top
        窗口始终置顶。
    resizable
        允许用户拖拽调整大小（默认 True）。
    resize_border_width
        无边框模式下可拖拽调整大小的边缘宽度（像素，默认 8）。
    gpu
        启用 GPU 加速渲染（默认 True）。
    """

    def __init__(
        self,
        title: str = "LightUI Window",
        width: int = 800,
        height: int = 600,
        headless: bool = False,
        borderless: bool = False,
        transparent: bool = False,
        always_on_top: bool = False,
        resizable: bool = True,
        resize_border_width: int = 8,
        gpu: bool = True,
    ) -> None: ...

    @property
    def title(self) -> str:
        """窗口标题（可读写）。"""
        ...
    @title.setter
    def title(self, value: str) -> None: ...

    @property
    def size(self) -> Tuple[int, int]:
        """当前窗口尺寸 ``(width, height)``（只读，用 ``set_size()`` 修改）。"""
        ...

    @property
    def position(self) -> Tuple[int, int]:
        """当前窗口位置 ``(x, y)``（只读，用 ``set_position()`` 修改）。"""
        ...

    @property
    def document(self) -> Optional[Document]:
        """关联的 DOM 文档（只读）。"""
        ...

    @property
    def is_borderless(self) -> bool:
        """窗口是否为无边框模式。"""
        ...

    @property
    def is_transparent(self) -> bool:
        """窗口是否启用透明背景。"""
        ...

    @property
    def resize_border_width(self) -> int:
        """无边框窗口可拖拽边缘宽度（像素）。"""
        ...

    @property
    def min_size(self) -> Tuple[int, int]:
        """最小尺寸 ``(width, height)``，``0`` 表示无限制。"""
        ...

    @property
    def max_size(self) -> Tuple[int, int]:
        """最大尺寸 ``(width, height)``，``0`` 表示无限制。"""
        ...

    def show(self) -> None:
        """显示窗口（从隐藏状态恢复）。"""
        ...

    def hide(self) -> None:
        """隐藏窗口（不销毁）。"""
        ...

    def close(self) -> None:
        """请求关闭窗口（设置 ``should_close`` 标志）。"""
        ...

    def set_size(self, width: int, height: int) -> None:
        """设置窗口尺寸。"""
        ...

    def set_position(self, x: int, y: int) -> None:
        """设置窗口在屏幕上的位置（左上角坐标）。"""
        ...

    def set_fullscreen(self, fullscreen: bool) -> None:
        """切换全屏模式。"""
        ...

    def set_resizable(self, resizable: bool) -> None:
        """动态开启/关闭用户可调整窗口大小。"""
        ...

    def set_borderless(self, borderless: bool) -> None:
        """动态切换无边框模式。"""
        ...

    def set_always_on_top(self, on_top: bool) -> None:
        """动态切换窗口置顶。"""
        ...

    def set_min_size(self, width: int, height: int) -> None:
        """设置最小窗口尺寸（``0`` 表示不限制）。"""
        ...

    def set_max_size(self, width: int, height: int) -> None:
        """设置最大窗口尺寸（``0`` 表示不限制）。"""
        ...

    def minimize(self) -> None:
        """最小化窗口。"""
        ...

    def maximize(self) -> None:
        """最大化窗口。"""
        ...

    def restore(self) -> None:
        """从最小化/最大化恢复到普通尺寸。"""
        ...

    def should_close(self) -> bool:
        """是否已请求关闭（``EventLoop`` 据此退出循环）。"""
        ...

    def render(self) -> None:
        """触发一次渲染（增量渲染：仅当 ``needs_repaint()`` 为 True 时实际绘制）。"""
        ...

    def swap_buffers(self) -> None:
        """交换前后缓冲区（``render()`` 后调用以呈现画面）。"""
        ...

    def needs_repaint(self) -> bool:
        """检查窗口是否有脏区域需要重绘。"""
        ...

    def mark_dirty(self) -> None:
        """手动标记窗口为需要重绘。"""
        ...

    def set_js_runtime(self, runtime: "Runtime") -> None:
        """将 ``Runtime`` 绑定到此窗口的 ``Document``，初始化 DOM 绑定和 WindowBindings。

        必须在 ``load_html`` / ``eval`` 之前调用。
        """
        ...

    def get_task_scheduler(self) -> TaskScheduler:
        """获取此窗口的 ``TaskScheduler``，用于构造 ``EventLoop``。"""
        ...

    def set_on_resize(self, callback: ResizeCallback) -> None:
        """注册窗口大小变化回调：``callback(width, height)``。"""
        ...

    def set_on_close(self, callback: CloseCallback) -> None:
        """注册窗口关闭回调：``callback()``。"""
        ...

    def set_on_focus(self, callback: FocusCallback) -> None:
        """注册窗口获得焦点回调：``callback()``。"""
        ...

    def set_on_blur(self, callback: FocusCallback) -> None:
        """注册窗口失去焦点回调：``callback()``。"""
        ...

    def is_valid(self) -> bool:
        """窗口对象是否有效。"""
        ...


# ---------------------------------------------------------------------------
# EventLoop
# ---------------------------------------------------------------------------

class EventLoop:
    """SDL 事件循环，对应 C++ ``PyEventLoop``。

    负责驱动：

    - SDL 窗口事件（输入、resize、close）
    - JS 定时器（``setTimeout`` / ``setInterval`` / ``requestAnimationFrame``）
    - 状态队列（每帧调用 ``StateManager::processQueue()``）
    - 增量渲染（仅在 ``Window.needs_repaint()`` 时绘制）
    - 事件队列刷新（``HostBridge.flush_events()``）

    .. warning::
        ``run()`` 结束后会调用 ``std::quick_exit(0)``，进程直接退出。

    典型搭建顺序
    ------------
    ::

        scheduler = window.get_task_scheduler()
        loop = lui.EventLoop(scheduler)
        loop.set_quickjs_runtime(runtime)
        loop.set_window(window)
        loop.set_state_manager(app)
        loop.set_host_bridge(bridge)
        loop.run()
    """

    @overload
    def __init__(self) -> None:
        """创建不带 TaskScheduler 的事件循环（不支持 JS 定时器）。"""
        ...

    @overload
    def __init__(self, task_scheduler: TaskScheduler) -> None:
        """创建带 TaskScheduler 的事件循环（支持 setTimeout 等）。"""
        ...

    def run(self) -> None:
        """启动事件循环（阻塞）。循环结束后调用 ``std::quick_exit(0)``。"""
        ...

    def stop(self) -> None:
        """请求停止事件循环（异步，下一帧生效）。"""
        ...

    def run_once(self) -> None:
        """处理一帧事件并返回（非阻塞，用于手动控制循环）。"""
        ...

    def is_running(self) -> bool:
        """事件循环是否正在运行。"""
        ...

    def should_quit(self) -> bool:
        """事件循环是否已被请求退出。"""
        ...

    def set_quickjs_runtime(self, runtime: "Runtime") -> None:
        """绑定 QuickJS 运行时，用于执行 JS 定时器回调。

        同时设置全局 ``EventLoop`` 供 DOM 的 ``execCommand`` 等 API 使用。
        """
        ...

    def set_window(self, window: Window) -> None:
        """绑定窗口，用于自动增量渲染（每帧检查 ``needs_repaint()``）。"""
        ...

    def set_state_manager(self, app: App) -> None:
        """绑定状态管理器，每帧自动调用 ``process_queue()`` 刷新状态。"""
        ...

    def set_host_bridge(self, bridge: "HostBridge") -> None:
        """绑定 HostBridge，每帧 ``process_queue()`` 之后自动调用 ``flush_events()``。"""
        ...

    def set_update_callback(self, callback: UpdateCallback) -> None:
        """注册每帧更新回调：``callback(delta_time)``。"""
        ...

    def set_render_callback(self, callback: RenderCallback) -> None:
        """注册渲染回调：``callback()``。若未设置，EventLoop 自动执行增量渲染。"""
        ...

    def set_idle_callback(self, callback: IdleCallback) -> None:
        """注册空闲回调：无事件时调用 ``callback()``。"""
        ...


# ---------------------------------------------------------------------------
# Runtime
# ---------------------------------------------------------------------------

class Runtime:
    """QuickJS JavaScript 运行时，对应 C++ ``PyRuntime``。

    提供 JS 代码执行、ES 模块注册、Fetch API 初始化等功能。
    销毁时自动调用 ``DOMBindings::Cleanup`` 和 ``DOMBindingMap::Clear``。

    .. note::
        执行失败时抛出 ``ValueError``（而非返回错误对象）。
    """

    def __init__(self) -> None: ...

    def eval(self, code: str, filename: str = "<eval>") -> JsonValue:
        """执行普通 JavaScript 代码，返回最后一条语句的值。

        Raises
        ------
        ValueError
            JS 执行出错时抛出。
        """
        ...

    def eval_module(self, code: str, filename: str = "<module>") -> JsonValue:
        """以 ES6 模块模式执行代码（支持 ``import`` / ``export``）。

        Raises
        ------
        ValueError
            JS 模块执行出错时抛出。
        """
        ...

    def eval_file(self, path: str) -> JsonValue:
        """从文件路径读取并执行 JavaScript 代码。

        Raises
        ------
        ValueError
            文件执行出错时抛出。
        """
        ...

    def register_module(self, name: str, code: str) -> None:
        """注册虚拟 ES 模块，供 ``import`` 语句引用。

        ::

            runtime.register_module("preact", preact_js_source)
            runtime.register_module("preact/hooks", hooks_js_source)
            runtime.eval_module("import { h } from 'preact'; ...")
        """
        ...

    def set_base_module_path(self, path: str) -> None:
        """设置相对路径模块导入的基础目录。"""
        ...

    def init_fetch_bindings(self, task_scheduler: TaskScheduler) -> None:
        """初始化全局 ``fetch`` API，需要传入 TaskScheduler 驱动异步请求。"""
        ...


# ---------------------------------------------------------------------------
# HostBridge
# ---------------------------------------------------------------------------

class HostBridge:
    """Python ↔ JS 双向通信桥接，对应 C++ ``PyHostBridge``。

    在 JS 中自动注册 ``py`` 全局对象，JS 通过 ``py.funcName(args)`` 调用 Python 函数。
    Python 通过 ``emit()`` 向 JS 发送事件（线程安全）。

    参数
    ----
    runtime
        绑定到的 JS 运行时。
    app
        提供 StateManager 的 App 实例。
    """

    def __init__(self, runtime: Runtime, app: App) -> None: ...

    def bind(self, name: str, func: HostCallback) -> None:
        """注册 Python 函数供 JS 调用。

        JS 端调用方式：``py.funcName(args)``，``args`` 自动序列化为 JSON。
        Python 函数接收解析后的 Python 对象，返回值转为 JSON 传回 JS。
        """
        ...

    def unbind(self, name: str) -> None:
        """取消注册指定名称的 Python 函数。"""
        ...

    def call(self, name: str, args: JsonValue = None) -> JsonValue:
        """从 Python 调用桥接中注册的函数（主要用于测试）。"""
        ...

    def emit(self, event_name: str, data: JsonValue = None) -> None:
        """向 JS 端发送事件（线程安全，进入事件队列）。

        JS 端监听：``window.addEventListener('eventName', (e) => { ... })``

        参数
        ----
        event_name
            事件名（非空字符串）。
        data
            事件数据，必须为 JSON 兼容类型。

        Raises
        ------
        ValueError
            ``event_name`` 为空时抛出。
        TypeError
            ``data`` 不可 JSON 序列化时抛出。
        """
        ...

    def flush_events(self) -> None:
        """刷新事件队列，立即执行 JS 事件回调。

        正常运行时由 ``EventLoop`` 每帧自动调用；
        单元测试或手动循环时需显式调用。
        """
        ...

    def is_valid(self) -> bool:
        """HostBridge 是否已正确初始化（Runtime 和 StateManager 均有效）。"""
        ...


# ---------------------------------------------------------------------------
# DevToolsManager
# ---------------------------------------------------------------------------

class DevToolsManager:
    """开发者工具管理器（单例），对应 C++ ``DevToolsManager``。

    通过 ``DevToolsManager.get_instance()`` 获取唯一实例。
    """

    @staticmethod
    def get_instance() -> "DevToolsManager":
        """获取 DevToolsManager 单例。"""
        ...

    def initialize(self, document: Document, window: Window) -> None:
        """初始化 DevTools，绑定到指定的文档和窗口。"""
        ...

    def open(self) -> None:
        """打开 DevTools 面板。"""
        ...

    def close(self) -> None:
        """关闭 DevTools 面板。"""
        ...

    def toggle(self) -> None:
        """切换 DevTools 显示/隐藏状态。"""
        ...

    def is_open(self) -> bool:
        """DevTools 面板当前是否已打开。"""
        ...

    def shutdown(self) -> None:
        """关闭并销毁 DevTools 相关资源。"""
        ...
