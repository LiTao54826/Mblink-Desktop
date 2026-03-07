"""
LightUI — 用 Python 构建桌面应用

快速开始：
    from lightui import App

    app = App("Counter", 400, 300)
    data = app.shared("data")
    data.count = 0

    @app.bind("increment")
    def _(args):
        data.count += 1

    app.load_html('<html><body><div id="root"></div></body></html>')
    app.load_preact("ui/app.js")
    app.run()
"""

__version__ = "0.5.0"
__author__ = "LightUI Team"

from .app import App
from .shared import SharedState

def version():
    return __version__

__all__ = ["App", "SharedState", "version"]
