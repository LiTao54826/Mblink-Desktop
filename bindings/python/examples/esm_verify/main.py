"""
分阶段验证 Python 绑定的 HTML / ESM 加载链路。

运行：
  cd bindings/python/examples/esm_verify
  python main.py html-inline
  python main.py html-file
  python main.py js-module
  python main.py html-esm
"""

import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..'))

from mbink import App

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
UI_DIR = os.path.join(BASE_DIR, "ui")


def log(step, ok, extra=""):
    state = "OK" if ok else "FAIL"
    print(f"[{state}] {step}{' - ' + extra if extra else ''}", flush=True)


def mode_html_inline(app: App):
    log("load_html(内联 HTML)", True)
    app.load_html("""
    <!DOCTYPE html>
    <html>
    <body style=\"margin:0;background:#111827;color:#e5e7eb;font-family:Arial;padding:24px;\">
      <h2>step1: inline html ok</h2>
      <p>如果你能看到这段，说明最基础的 HTML 渲染是通的。</p>
    </body>
    </html>
    """)


def mode_html_file(app: App):
    html_path = os.path.join(UI_DIR, "index_noscript.html")
    log("load_html_file(纯 HTML 文件)", os.path.exists(html_path), html_path)
    app.load_html_file(html_path)


def mode_js_module(app: App):
    app.load_html("<html><body><div id='app'></div></body></html>")
    js_path = os.path.join(UI_DIR, "app.js")
    log("load_js_file(独立 ESM)", os.path.exists(js_path), js_path)
    app.load_js_file(js_path)


def mode_html_esm(app: App):
    html_path = os.path.join(UI_DIR, "index.html")
    log("load_html_file(HTML + <script type=module>)", os.path.exists(html_path), html_path)
    app.load_html_file(html_path)


MODES = {
    "html-inline": mode_html_inline,
    "html-file": mode_html_file,
    "js-module": mode_js_module,
    "html-esm": mode_html_esm,
}


mode = sys.argv[1] if len(sys.argv) > 1 else "html-esm"
if mode not in MODES:
    print(f"未知模式: {mode}")
    print("可用模式:", ", ".join(MODES.keys()))
    sys.exit(2)

app = App(f"ESM Verify - {mode}", 820, 520, resizable=True, gpu=True)
log("create app", True, mode)
MODES[mode](app)
app.run()

