"""
LightUI — 用 Python 构建桌面应用 (v2 ctypes 绑定)

快速开始：
    from lightui import App

    app = App("My App", 800, 600)
    counter = app.state("counter", 0)

    @app.bind("increment")
    def increment(args):
        counter.value += 1
        return {"count": counter.value}

    app.load_html('<button onclick="host.call(\'increment\')">Click</button>')
    app.run()
"""

__version__ = "2.0.0"
__author__ = "LightUI Team"

# v2 高级 API (ctypes)
from .app_v2 import App
from ._state import State

# 版本函数
def version():
    """返回 LightUI Python 包版本号"""
    return __version__

__all__ = ["App", "State", "version"]
