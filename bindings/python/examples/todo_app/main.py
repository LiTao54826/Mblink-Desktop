"""
Todo App 示例 — 展示 LightUI Python 绑定的核心功能

功能演示：
  - 多个 SharedState 字段（列表 + 字符串 + 数字）
  - Python → JS 数据推送（添加/删除/完成 todo）
  - JS → Python 函数调用（py.addTodo / py.toggleTodo / py.deleteTodo）
  - on_update 定时器（每秒更新时钟）
  - 动态修改窗口标题
"""

import time
from lightui import App

app = App("Todo App", 480, 600)

# ── 共享状态（初始值在 load_html 之后设置）──────────────────
state = app.shared("state")
_next_id = 1

# ── Python 函数（供 JS 调用）────────────────────────────────
@app.bind("addTodo")
def _(args):
    global _next_id
    text = (args or {}).get("text", "").strip()
    if not text:
        return
    todos = list(state.todos)
    todos.append({"id": _next_id, "text": text, "done": False})
    _next_id += 1
    state.todos = todos
    # 更新标题显示未完成数量
    _update_title()

@app.bind("toggleTodo")
def _(args):
    tid = (args or {}).get("id")
    todos = [
        {**t, "done": not t["done"]} if t["id"] == tid else t
        for t in state.todos
    ]
    state.todos = todos
    _update_title()

@app.bind("deleteTodo")
def _(args):
    tid = (args or {}).get("id")
    state.todos = [t for t in state.todos if t["id"] != tid]
    _update_title()

@app.bind("setFilter")
def _(args):
    state.filter = (args or {}).get("value", "all")

@app.bind("clearDone")
def _(args):
    state.todos = [t for t in state.todos if not t["done"]]
    _update_title()

def _update_title():
    active = sum(1 for t in state.todos if not t["done"])
    app.title = f"Todo App ({active} 待完成)"

# ── 时钟（on_update 在主线程事件循环里调用，QuickJS 线程安全）──
_last_clock = ""

@app.on_update
def _(dt):
    global _last_clock
    now = time.strftime("%H:%M:%S")
    if now != _last_clock:
        _last_clock = now
        state.clock = now

# ── 加载 UI ───────────────────────────────────────────────
app.load_html("""<!DOCTYPE html>
<html><head><meta charset="utf-8">
<style>* { box-sizing: border-box; margin: 0; padding: 0; }</style>
</head><body><div id="root"></div></body></html>""")

# ── 共享状态初始值（load_html 之后设置，确保 JS runtime 就绪）──
state.todos = []
state.clock = time.strftime("%H:%M:%S")
state.filter = "all"

app.load_preact("ui/app.js")
app.run()

