"""
LightUI 声明式 API 示例 — Counter

纯 Python 构建桌面 UI，不写一行 HTML/CSS/JS
"""
import sys
import os

# 确保能找到 lightui 包
sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))

from lightui import App
from lightui.ui import Column, Row, Text, Button, Input, Spacer, Divider

# ========== 创建应用 ==========

app = App("Counter Demo", 400, 350)

# ========== 声明状态 ==========

count = app.state("count", 0)
name = app.state("name", "World")

# ========== 绑定函数 ==========

@app.bind("increment")
def increment(args):
    count.value += 1

@app.bind("decrement")
def decrement(args):
    count.value -= 1

@app.bind("reset")
def reset(args):
    count.value = 0

# ========== 声明式 UI ==========

app.ui(
    Column(
        Text("Hello, ", font_size=20),
        Text(bind=name, font_size=20, color="#007bff", font_weight="bold"),
        Spacer(height=24),
        Text("Count:", font_size=16, color="#666"),
        Text(bind=count, font_size=64, color="#333", font_weight="bold"),
        Spacer(height=16),
        Row(
            Button("-1", on_click="decrement", font_size=18, padding="8px 24px"),
            Button("Reset", on_click="reset", font_size=14, padding="8px 16px"),
            Button("+1", on_click="increment", font_size=18, padding="8px 24px"),
            gap=12, justify="center"
        ),
        Divider(),
        Input(placeholder="输入你的名字", bind=name, width=200, font_size=14),
        align="center", justify="center", padding=32, gap=12,
        height="100vh"
    )
)

# ========== 运行 ==========

app.run()

