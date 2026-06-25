"""
Todo App 示例 - 展示 MBlink Python 绑定的核心功能

功能演示：
  - 多个 SharedState 字段（列表 + 字符串 + 数字）
  - Python → JS 数据推送（添加/删除/完成 todo）
  - JS → Python 函数调用（py.addTodo / py.toggleTodo / py.deleteTodo）
  - on_update 定时器（每秒更新时钟）
  - 动态修改窗口标题
"""
import os
import sys
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..'))

from mblink import App

app = App("Todo App", 700, 600)

# ── 共享状态（初始值在 load_html 之后设置）──────────────────
state = app.shared("state")
_next_id = 1
_todos = []   # Python 侧本地维护，避免在 JS 回调里重入 QuickJS

def _push_todos():
    """把本地 _todos 推送到 JS，并更新标题"""
    state.todos = list(_todos)
    active = sum(1 for t in _todos if not t["done"])
    app.title = f"Todo App ({active} 待完成)"

# ── Python 函数（供 JS 调用）────────────────────────────────
@app.bind("addTodo")
def _(args):
    global _next_id
    text = (args or {}).get("text", "").strip()
    if not text:
        return
    _todos.append({"id": _next_id, "text": text, "done": False})
    _next_id += 1
    _push_todos()

@app.bind("toggleTodo")
def _(args):
    tid = (args or {}).get("id")
    for t in _todos:
        if t["id"] == tid:
            t["done"] = not t["done"]
            break
    _push_todos()

@app.bind("deleteTodo")
def _(args):
    tid = (args or {}).get("id")
    _todos[:] = [t for t in _todos if t["id"] != tid]
    _push_todos()

@app.bind("setFilter")
def _(args):
    state.filter = (args or {}).get("value", "all")

@app.bind("clearDone")
def _(args):
    _todos[:] = [t for t in _todos if not t["done"]]
    _push_todos()



# ── 加载 UI ───────────────────────────────────────────────
app.load_html("""<!DOCTYPE html>
<html><head><meta charset="utf-8">
<style>* { box-sizing: border-box; margin: 0; padding: 0; }</style>
</head><body><div id="root"></div></body></html>""")

# ── 共享状态初始值（load_html 之后设置，确保 JS runtime 就绪）──
state.todos = []
state.filter = "all"

app.load_js_file("ui/app.js")
app.run()

