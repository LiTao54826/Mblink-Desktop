"""
Counter v3: eval_js 直接模式 (CEF 风格)
不使用 State/watch，Python 直接通过 eval_js 操作 DOM
"""
import sys, os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))
from lightui.app_v2 import App

app = App("Counter v3 — eval_js 直接模式", 420, 320)

# Python 直接持有变量，不走 State
count = 0
name = "World"

def update_ui():
    """将 Python 变量推送到 DOM"""
    app.eval_js(f"document.getElementById('count').textContent = '{count}';")
    app.eval_js(f"document.getElementById('greeting').textContent = 'Hello, {name}!';")

@app.bind("increment")
def _(args):
    global count
    count += 1
    update_ui()

@app.bind("decrement")
def _(args):
    global count
    count -= 1
    update_ui()

@app.bind("reset")
def _(args):
    global count
    count = 0
    update_ui()

@app.bind("set_name")
def _(args):
    global name
    if args and isinstance(args, str):
        name = args
    elif args and isinstance(args, dict) and 'value' in args:
        name = args['value']
    update_ui()

html = """<!DOCTYPE html>
<html><head><meta charset="utf-8">
<style>
* { margin:0; padding:0; box-sizing:border-box; }
body {
  display:flex; flex-direction:column; align-items:center;
  justify-content:center; height:100vh;
  font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
  background: #f5f5f5;
}
#count {
  font-size: 80px; font-weight: bold; color: #333;
  margin-bottom: 24px;
}
.btn-row { display:flex; gap:12px; margin-bottom:24px; }
button {
  padding: 12px 28px; font-size: 18px;
  border: 1px solid #ccc; border-radius: 8px;
  background: #fff; cursor: pointer;
}
button:hover { background: #e8e8e8; }
button:active { background: #d0d0d0; }
input {
  padding: 10px 16px; font-size: 16px; width: 260px;
  border: 1px solid #ccc; border-radius: 8px; outline: none;
}
input:focus { border-color: #2196f3; box-shadow: 0 0 0 2px rgba(33,150,243,0.2); }
#greeting { margin-top: 16px; font-size: 20px; color: #666; }
</style>
</head><body>

<div id="count">0</div>

<div class="btn-row">
  <button onclick="host.call('decrement')">-1</button>
  <button onclick="host.call('reset')">Reset</button>
  <button onclick="host.call('increment')">+1</button>
</div>

<input id="nameInput" placeholder="输入你的名字"
       oninput="host.call('set_name', this.value)" />

<div id="greeting">Hello, World!</div>

</body></html>"""

app.load_html(html)
app.run()

