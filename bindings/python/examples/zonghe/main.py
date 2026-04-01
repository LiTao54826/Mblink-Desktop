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

sys_state = app.shared("sys")
work_log = None


_boot_time = time.monotonic()
_manual_mode = False
def _mock_runtime(now: float):
    phases = [
        {
            "runtime_status": "监听中",
            "current_task_id": "--",
            "current_task_type": "--",
            "current_stage": "subscribe",
            "current_account": "--",
            "current_action": "等待任务下发",
            "log_level": "INFO",
            "log_text": "监听队列中，等待新任务",
        },
        {
            "runtime_status": "执行中",
            "current_task_id": "T2025-0001",
            "current_task_type": "制单",
            "current_stage": "make_voucher",
            "current_account": "中信银行 / ****0189",
            "current_action": "打开页面 -> 填单 -> 等待确认",
            "log_level": "INFO",
            "log_text": "任务执行中，正在等待网银确认",
        },
        {
            "runtime_status": "执行中",
            "current_task_id": "T2025-0002",
            "current_task_type": "签收",
            "current_stage": "receive",
            "current_account": "苏宁银行 / ****6621",
            "current_action": "查询票据 -> 签收确认 -> 回传结果",
            "log_level": "INFO",
            "log_text": "签收任务处理中，等待结果回传",
        },
        {
            "runtime_status": "异常",
            "current_task_id": "T2025-0003",
            "current_task_type": "制单",
            "current_stage": "ukey_confirm",
            "current_account": "中信银行 / ****0189",
            "current_action": "等待 UKey 确认超时",
            "log_level": "ERROR",
            "log_text": "UKey 确认超时，请人工处理",
        },
    ]
    return phases[int(now // 12) % len(phases)]


def _set_log(level: str, text: str):
    sys_state.last_log_level = level
    sys_state.last_log_text = text
    sys_state.timestamp = datetime.now().strftime("%H:%M:%S")
    if work_log is not None:
        work_log.append(level, "runtime", text)


def _init_state():
    global _manual_mode, _boot_time
    _manual_mode = False
    _boot_time = time.monotonic()
    sys_state.robot_name = "票据机器人-长沙01"
    sys_state.current_user = "robot_maker"
    sys_state.online_status = "在线"
    sys_state.runtime_status = "监听中"
    sys_state.current_task_id = "--"
    sys_state.current_task_type = "--"
    sys_state.current_stage = "subscribe"
    sys_state.current_account = "--"
    sys_state.current_action = "等待任务下发"
    sys_state.task_elapsed = 0
    sys_state.heartbeat_at = "--:--:--"
    sys_state.today_success = 12
    sys_state.today_failed = 1
    if work_log is not None:
        work_log.clear()
    _set_log("INFO", "执行端已启动，等待任务下发")


def _sample():
    if _manual_mode:
        sys_state.heartbeat_at = datetime.now().strftime("%H:%M:%S")
        return

    now = time.monotonic()
    elapsed = int(now - _boot_time)
    phase = _mock_runtime(now)

    sys_state.runtime_status = phase["runtime_status"]
    sys_state.current_task_id = phase["current_task_id"]
    sys_state.current_task_type = phase["current_task_type"]
    sys_state.current_stage = phase["current_stage"]
    sys_state.current_account = phase["current_account"]
    sys_state.current_action = phase["current_action"]
    sys_state.task_elapsed = elapsed
    sys_state.heartbeat_at = datetime.now().strftime("%H:%M:%S")
    _set_log(phase["log_level"], phase["log_text"])


@app.bind("start_listen")
def _(_args):
    del _args
    global _manual_mode, _boot_time
    _manual_mode = True
    _boot_time = time.monotonic()
    sys_state.runtime_status = "监听中"
    sys_state.current_task_id = "--"
    sys_state.current_task_type = "--"
    sys_state.current_stage = "subscribe"
    sys_state.current_account = "--"
    sys_state.current_action = "等待任务下发"
    sys_state.task_elapsed = 0
    sys_state.heartbeat_at = datetime.now().strftime("%H:%M:%S")
    _set_log("INFO", "已手动切换到监听模式")
    return {"ok": True}


@app.bind("retry_task")
def _(_args):
    del _args
    global _manual_mode, _boot_time
    _manual_mode = True
    _boot_time = time.monotonic()
    sys_state.runtime_status = "执行中"
    sys_state.current_task_id = sys_state.current_task_id if sys_state.current_task_id not in (None, "", "--") else "T2025-RETRY"
    sys_state.current_task_type = sys_state.current_task_type if sys_state.current_task_type not in (None, "", "--") else "签收"
    sys_state.current_stage = "retry"
    sys_state.current_account = sys_state.current_account if sys_state.current_account not in (None, "", "--") else "中信银行 / ****0189"
    sys_state.current_action = "重新发起任务执行"
    sys_state.task_elapsed = 0
    sys_state.heartbeat_at = datetime.now().strftime("%H:%M:%S")
    _set_log("WARN", "已收到重试指令，准备重新执行任务")
    return {"ok": True}


@app.bind("stop_task")
def _(_args):
    del _args
    global _manual_mode
    _manual_mode = True
    sys_state.runtime_status = "空闲"
    sys_state.current_stage = "stopped"
    sys_state.current_action = "任务已停止，等待下一步指令"
    sys_state.heartbeat_at = datetime.now().strftime("%H:%M:%S")
    _set_log("ERROR", "任务已手动停止")
    return {"ok": True}


@app.bind("reset_state")
def _(_args):
    print("reset_state")
    del _args
    _init_state()
    return {"ok": True}

# ── on_update：每帧调用，累积时间控制采样频率 ──────────────────────────────────
_accum = 0.0

@app.on_update
def _(delta: float):
    global _accum
    _accum += delta
    if _accum >= SAMPLE_RATE:
        _accum -= SAMPLE_RATE
        _sample()

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
work_log = app.logview("work-log")
_init_state()
app.run()



'''
pyinstaller -Fw main.py -i logo.ico --add-binary "mbink\bin\mbink.dll;mbink\bin" --add-data "ui;ui"
'''
