import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..'))

from mbink import App

host_exe = sys.executable.replace('\\', '/')
base_dir = os.path.dirname(__file__)
ui_dir = os.path.join(base_dir, 'ui')
os.makedirs(ui_dir, exist_ok=True)

html = f"""<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <title>Preact app://res.ico Verify</title>
  <style>
    body {{ margin: 0; padding: 24px; background: #0f1117; color: #e5e7eb; font-family: Arial, sans-serif; }}
    .card {{ padding: 16px; border-radius: 12px; border: 1px solid #2a3142; background: #1a1f2b; }}
  </style>
</head>
<body>
  <div class="card">
    <div>宿主 EXE：<code>{host_exe}</code></div>
    <div id="app" style="margin-top:16px;"></div>
  </div>
</body>
</html>
"""


app = App('Preact app://res.ico Verify', 720, 320, resizable=True, gpu=True)
app.load_html(html)
app.load_js_file(os.path.join(ui_dir, 'app.js'))
app.run()

