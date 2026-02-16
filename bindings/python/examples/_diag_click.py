"""
最小诊断：确认 host.call 是否能从 JS 到达 Python
不依赖 ui.py / State / watch，纯手写 HTML
"""
import sys, os, datetime
sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))
from lightui.app_v2 import App

LOG_FILE = os.path.join(os.path.dirname(__file__), "_diag_log.txt")

def log(msg):
    line = f"[{datetime.datetime.now().isoformat()}] {msg}"
    print(line, flush=True)
    with open(LOG_FILE, "a", encoding="utf-8") as f:
        f.write(line + "\n")

log("=== diag start ===")

app = App("Diag Click", 400, 300)
log(f"handle = {app._handle}")

# 注册一个最简单的回调
@app.bind("test_click")
def on_test(args):
    log(f">>> PYTHON CALLBACK REACHED!  args={args}")
    return {"ok": True}

log("bind 'test_click' done")

# 手写 HTML，不依赖 ui.py / State
html = """<!DOCTYPE html>
<html><head><meta charset="utf-8">
<style>
body { display:flex; flex-direction:column; align-items:center;
       justify-content:center; height:100vh; font-family:sans-serif; }
button { padding:20px 40px; font-size:24px; cursor:pointer; }
#msg { margin-top:20px; font-size:18px; color:#666; }
</style>
</head><body>
<button onclick="host.call('test_click')">CLICK ME</button>
<div id="msg">click the button ...</div>
<script>
// 如果这行执行了，说明 ExecuteScripts 工作
host.call('test_click', 'script_init');
</script>
</body></html>"""

app.load_html(html)
log("load_html done, entering run loop ...")
app.run()
log("=== diag end ===")

