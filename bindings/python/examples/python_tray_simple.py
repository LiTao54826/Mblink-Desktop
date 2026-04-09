import sys
import os
import time
from pathlib import Path

# 允许直接从 bindings/python/examples 运行：python bindings/python/examples/python_tray_simple.py
# 本示例演示由用户态显式创建 tray、设置菜单，并通过装饰器按 id 分发处理函数。

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

from mbink import App


HTML = """
<!doctype html>
<html>
<head>
  <meta charset="utf-8" />
  <title>MBink Tray Demo</title>
  <style>
    body {
      font-family: "Segoe UI", sans-serif;
      margin: 24px;
      background: #111827;
      color: #f9fafb;
    }
    button {
      margin: 8px 8px 0 0;
      padding: 10px 16px;
      border: 0;
      border-radius: 8px;
      cursor: pointer;
      background: #2563eb;
      color: white;
    }
    .muted { color: #9ca3af; }
    code {
      color: #93c5fd;
      background: rgba(255,255,255,0.08);
      padding: 2px 6px;
      border-radius: 6px;
    }
  </style>
</head>
<body>
  <h2>MBink Windows Tray 示例</h2>
  <p>当前 tray 和菜单由 Python 用户态创建，不是框架默认内置。</p>
  <p class="muted">左键托盘图标会恢复主窗口，右键菜单通过 <code>@app.tray_action(...)</code> 按 id 分发。</p>

  <button onclick="backend.hide_to_tray()">隐藏到托盘</button>
  <button onclick="backend.show_main_window()">显示主窗口</button>
  <button onclick="backend.change_title()">修改标题 / Tooltip</button>
  <button onclick="backend.quit_app()">退出应用</button>
</body>
</html>
"""


def main():
    app = App("MBink Tray Simple Demo", 520, 260)

    tray_menu = [
        {"id": "show", "label": "显示"},
        {
            "type": "submenu",
            "label": "更多",
            "children": [
                {"id": "sync", "label": "同步", "enabled": True},
                {"id": "debug", "label": "调试模式", "checked": True},
            ],
        },
        {"id": "quit", "label": "退出"},
    ]

    app.create_tray(tooltip="MBink Tray Simple Demo", menu=tray_menu)
    app.load_html(HTML)

    @app.on_tray_click
    def _tray_click():
        app.show_main_window()

    @app.tray_action("show")
    def _tray_show():
        app.show_main_window()

    @app.tray_action("hide")
    def _tray_hide():
        app.hide_to_tray()

    @app.tray_action("sync", background=True)
    def _tray_sync():
        print("[tray] 开始后台同步...", flush=True)
        time.sleep(5)
        print("[tray] 后台同步完成", flush=True)

    @app.tray_action("quit")
    def _tray_quit():
        app.stop()
    
    @app.on_close_request
    def _close_request():
        print("on_close_request")
        app.hide_to_tray()
        return True

    app.run()


if __name__ == "__main__":
    main()
