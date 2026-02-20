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

from lightui import App

app = App("Preact Counter", 400, 350)

# 创建共享 C 对象 — JS 中可通过 globalThis.data 访问
data = app.shared("data")
data.count = 0

# 加载 HTML（提供 #root 挂载点）
app.load_html("""
<!DOCTYPE html>
<html>
<head><meta charset="utf-8"></head>
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
app.load_preact("ui/app.js")

app.run()

