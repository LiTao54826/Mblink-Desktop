"""
LightUI — 用 Python 构建桌面应用 (v2 ctypes 绑定)

快速开始（声明式 API）：
    from lightui import App
    from lightui.ui import Column, Text, Button

    app = App("Counter", 400, 300)
    count = app.state("count", 0)

    @app.bind("increment")
    def increment(args):
        count.value += 1

    app.ui(
        Column(
            Text(bind=count, font_size=48),
            Button("+1", on_click="increment"),
            align="center", padding=32, gap=16
        )
    )
    app.run()
"""

__version__ = "2.1.0"
__author__ = "LightUI Team"

# v2 高级 API (ctypes)
from .app_v2 import App
from ._state import State

# 声明式 UI 组件
from .ui import (
    Column, Row, Box, Spacer,
    Text, Button, Input, Checkbox, Image, Link, Divider,
    build_html,
)

# 版本函数
def version():
    """返回 LightUI Python 包版本号"""
    return __version__

__all__ = [
    "App", "State", "version",
    "Column", "Row", "Box", "Spacer",
    "Text", "Button", "Input", "Checkbox", "Image", "Link", "Divider",
    "build_html",
]
