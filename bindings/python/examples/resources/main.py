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
    # dll_path=BUILD_DLL,
    borderless=True, resizable=True, gpu=False,
    min_size=(WINDOW_W, WINDOW_H),
)
sys_state = app.shared("sys")

tray_menu = [
    {"id": "show", "label": "显示主界面"},
    {"id": "hide", "label": "隐藏窗口"},
    {"id": "sync", "label": "后台同步(耗时示例)"},
    {"type": "separator"},
    {"id": "quit", "label": "退出应用"},
]

app.create_tray(tooltip="MBink Tray Simple Demo", menu=tray_menu)

@app.on_tray_click
def _tray_click():
    app.show_main_window()

@app.on_close_request
def _close_request():
    print("1111111")
    app.hide_to_tray()
    return True

# ── 窗口关闭回调 ──────────────────────────────────────────────────────────────
@app.on_close
def _():
    print("[WorkClient] 窗口关闭")


@app.on_update
def _(a):
    sys_state.heartbeat_at = datetime.now().strftime("%H:%M:%S")

# ── 加载 UI ───────────────────────────────────────────────────────────────────
app.load_html_file('./index.html')
app.load_js_file("./ui/app.js")
app.run()



'''
pyinstaller -Fw main.py -i logo.ico --add-binary "mbink\bin\mbink.dll;mbink\bin" --add-data "ui;ui"
'''


