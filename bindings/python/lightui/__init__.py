"""
LightUI — 用 Python 构建桌面应用

快速开始：
    import lightui as ui

    app = ui.App("My App", 800, 600)
    counter = app.state("counter", 0)

    @app.bindable
    def increment():
        counter.increment()
        return counter.get()

    app.load_html('<button onclick="py.increment()">Click</button>')
    app.run()

低级 API（直接访问 C++ 绑定）：
    from lightui.core import Window, Runtime, HostBridge
"""

__version__ = "0.5.0"
__author__ = "LightUI Team"

# 高级 API
from .app import App

# 版本函数（从 C++ 绑定获取）
try:
    from .core import version
except ImportError:
    def version():
        """返回 LightUI 版本号（C++ 模块不可用时返回 Python 包版本）"""
        return __version__

# 向后兼容：LightUIApp 作为 App 的别名
LightUIApp = App

__all__ = ["App", "version"]
