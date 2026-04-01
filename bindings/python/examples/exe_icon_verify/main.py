"""
验证宿主 EXE 图标加载。

运行：
  cd bindings/python/examples/exe_icon_verify
  python main.py
"""

import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..'))

from mbink import App

host_exe = sys.executable.replace('\\', '/')

html = f"""
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <title>Host EXE Icon Verify</title>
  <style>
    body {{
      margin: 0;
      padding: 24px;
      background: #0f1117;
      color: #e5e7eb;
      font-family: Arial, sans-serif;
    }}
    .row {{ display: flex; gap: 16px; flex-wrap: wrap; }}
    .card {{
      width: 220px;
      padding: 16px;
      border-radius: 12px;
      border: 1px solid #2a3142;
      background: #1a1f2b;
    }}
    .icon-box {{
      width: 96px;
      height: 96px;
      display: flex;
      align-items: center;
      justify-content: center;
      border: 1px dashed #4b5563;
      border-radius: 10px;
      background: #111827;
      margin-top: 12px;
    }}
    img {{ width: 64px; height: 64px; object-fit: contain; }}
    code {{ color: #93c5fd; word-break: break-all; }}
    .ok {{ color: #34d399; font-weight: bold; }}
  </style>
</head>
<body>
  <h2>宿主 EXE 图标验证</h2>
  <p class="ok">这里显示的应该是宿主进程 EXE 的图标，不是 MBink DLL 的图标。</p>
  <p>当前宿主 EXE：<code>{host_exe}</code></p>

  <div class="row">
    <div class="card">
      <div><code>&lt;img src="app://res.ico"&gt;</code></div>
      <div class="icon-box">
        <img src="app://res.ico" alt="app://res.ico">
      </div>
    </div>

    <div class="card">
      <div><code>JS dynamic img.src = "app://res.ico"</code></div>
      <div class="icon-box" id="dynamic-box"></div>
    </div>
  </div>

  <script>
    (function () {{
      const img = document.createElement('img');
      img.alt = 'dynamic-app-icon';
      img.src = 'app://res.ico';
      document.getElementById('dynamic-box').appendChild(img);
    }})();
  </script>

  <p style="margin-top: 20px; color: #9ca3af;">
    如果三个框都空白，说明宿主应用图标提取链路有问题；
    如果都能显示，说明 <code>app://res.ico</code> 在 HTML 和 JS 动态创建两条链路都生效。
  </p>
</body>
</html>
"""

app = App("Host EXE Icon Verify", 760, 360, resizable=True, gpu=True)
app.load_html(html)
app.run()

