"""
Preact-First Counter 示例

架构：
  Python  →  SharedState (C 对象)  →  JS (Preact VDOM)
  data.count = 0                       globalThis.data.count → 0
  data.count += 1                      __onSharedUpdate → re-render
  py.increment()                       JS 调用 Python 函数

用法：
  cd bindings/python/examples/counter_preact
  python main.py
"""
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..'))

from mbink import App

app = App("Preact Counter", 800, 600)

# 创建共享 C 对象 — JS 中可通过 globalThis.data 访问
data = app.shared("data")
data.count = 0

# 加载 HTML（提供 #root 挂载点）
app.load_html("""
<!DOCTYPE html>
<html>
<head><meta charset="utf-8">
<style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    html, body {overflow: hidden;}
    #root { width: 100%; height: 100%; background: #ff4d4f;}
  </style>
</head>
              
<body><div id="root"></div></body>
</html>
""")

# 注册 Python 函数 — JS 中通过 py.increment() / py.decrement() 调用
@app.bind("increment")
def _(args):
    data.count += 1

@app.bind("decrement")
def _(args):
    data.count -= 1

@app.bind("reset")
def _(args):
    data.count = 0

# 加载 Preact UI（ES module）
app.load_js_file("ui/app.js")

app.run()

