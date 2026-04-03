"""
MBink 票据机器人执行端示例

功能：
  - 无边框深色主题窗口（borderless + resizable）
  - 基于 SharedState 向前端推送执行端状态
  - 展示任务状态、执行信息、日志与操作栏

运行：
  python main.py
"""

import sys
import os
import time
from datetime import datetime

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..'))

from mbink import App

SAMPLE_RATE = 1.0
WINDOW_W = 900 * 2
WINDOW_H = 600 * 2

app = App(
    "票据机器人 Work Client", WINDOW_W, WINDOW_H,
    borderless=True, resizable=True, gpu=False,
    min_size=(WINDOW_W, WINDOW_H),
)

# ── 窗口关闭回调 ──────────────────────────────────────────────────────────────
@app.on_close
def _():
    print("[WorkClient] 窗口关闭")

# ── 加载 UI ───────────────────────────────────────────────────────────────────
app.load_html("""<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    html, body { width: 100%; height: 100%; margin: 0; overflow: hidden; background: #eef4ff; }
    body { font-family: "Segoe UI", Arial, sans-serif; }
    #root { width: 100vw; height: 100vh; overflow: hidden; }
    .drag { -webkit-app-region: drag; }
    .no-drag { -webkit-app-region: no-drag; }
    .window-control {
      width: 12px;
      height: 12px;
      border-radius: 50%;
      display: inline-block;
      flex-shrink: 0;
    }
    .wc-minimize { background: #64748b; -webkit-window-control: minimize; }
    .wc-maximize { background: #34d399; -webkit-window-control: maximize; }
    .wc-close { background: #f87171; -webkit-window-control: close; }
  </style>
</head>
<body><div id="root"></div></body>
</html>""")

app.load_preact("ui/app.js")
app.run()



'''
pyinstaller -Fw main.py -i logo.ico --add-binary "mbink\bin\mbink.dll;mbink\bin" --add-data "ui;ui"
'''


