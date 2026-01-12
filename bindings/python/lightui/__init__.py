"""
LightUI Python 绑定

提供 Python 友好的 API 来构建桌面应用。

功能：
- 窗口创建和管理
- 状态管理（响应式数据绑定）
- Python ↔ JS 双向通信
- DOM 操作
- 事件循环

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

低级 API：
    import lightui_core as lui
    
    # 直接使用 C++ 绑定
    window = lui.Window("Test", 800, 600)
    runtime = lui.Runtime()
    app = lui.App()
    bridge = lui.HostBridge(runtime, app)
"""

__version__ = "0.4.0"
__author__ = "LightUI Team"

# 导入高级 API
from .app import LightUIApp, App

# 尝试导入低级 API（pyd 文件在 bin 子目录）
try:
    import sys
    import os
    _bin_path = os.path.join(os.path.dirname(__file__), 'bin')
    if _bin_path not in sys.path:
        sys.path.insert(0, _bin_path)
    import lightui_core
    
    # 导出低级 API 类
    Window = lightui_core.Window
    Document = lightui_core.Document
    Element = lightui_core.Element
    Runtime = lightui_core.Runtime
    EventLoop = lightui_core.EventLoop
    HostBridge = lightui_core.HostBridge
    
    # 状态类
    State = lightui_core.State
    IntState = lightui_core.IntState
    StringState = lightui_core.StringState
    ListState = lightui_core.ListState
    DictState = lightui_core.DictState
    
    # 辅助类
    BatchContext = lightui_core.BatchContext
    
    # 函数
    version = lightui_core.version
    
except ImportError:
    # 如果 C++ 模块未构建，只提供高级 API
    pass

__all__ = [
    # 高级 API
    "LightUIApp",
    "App",
    
    # 低级 API（如果可用）
    "Window",
    "Document", 
    "Element",
    "Runtime",
    "EventLoop",
    "HostBridge",
    "State",
    "IntState",
    "StringState",
    "ListState",
    "DictState",
    "BatchContext",
    "version",
]
