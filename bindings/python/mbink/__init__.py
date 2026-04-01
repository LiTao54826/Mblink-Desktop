"""
MBink Python 包入口

快速开始：
    from mbink import App

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
__author__ = "MBink Team"

from .app import App
from .controls import LogView, Terminal
from .shared import SharedState

def version():
    return __version__

__all__ = ["App", "SharedState", "LogView", "Terminal", "version"]
