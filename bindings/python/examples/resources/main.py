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

BUILD_DLL = os.path.normpath(
    os.path.join(os.path.dirname(__file__), '..', '..', '..', '..', 'build', 'bin', 'Release', 'mbink.dll')
)
print(f"[MBinkPythonDiag] dll_path={BUILD_DLL} exists={os.path.exists(BUILD_DLL)}")

app = App(
    "票据机器人 Work Client", WINDOW_W, WINDOW_H,
    # dll_path=BUILD_DLL,
    borderless=True, resizable=True, gpu=False,
    min_size=(WINDOW_W, WINDOW_H),
)
sys_state = app.shared("sys")
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


