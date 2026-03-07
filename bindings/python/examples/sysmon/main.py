"""
LightUI 系统监视器示例

功能：
  - 无边框深色主题窗口（borderless + resizable）
  - 实时采集 CPU / 内存 / 磁盘 / 网络数据（psutil）
  - SharedState 推送到 Preact UI，1 秒刷新一次
  - SVG 折线图展示 CPU 使用率历史（60 秒）
  - 自定义标题栏（拖拽 + 窗口控制按钮）

依赖：
  pip install psutil

运行：
  cd bindings/python/examples/sysmon
  python main.py
"""

import sys
import os
import time
from collections import deque
from datetime import datetime

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..'))

try:
    import psutil
except ImportError:
    print("[ERROR] 请先安装 psutil：pip install psutil")
    sys.exit(1)

from lightui import App

# ── 配置 ─────────────────────────────────────────────────────────────────────
HISTORY_LEN  = 60    # 保留 60 个采样点（60 秒历史）
SAMPLE_RATE  = 1.0   # 采样间隔（秒）
WINDOW_W     = 900
WINDOW_H     = 560

# ── 初始化 App ────────────────────────────────────────────────────────────────
app = App(
    "System Monitor", WINDOW_W, WINDOW_H,
    borderless=True, resizable=True, gpu=True,
    min_size=(640, 400),
)

# ── SharedState ───────────────────────────────────────────────────────────────
sys_state = app.shared("sys")

# ── 历史数据环形缓冲区 ─────────────────────────────────────────────────────────
_cpu_hist  = deque([0.0] * HISTORY_LEN, maxlen=HISTORY_LEN)
_mem_hist  = deque([0.0] * HISTORY_LEN, maxlen=HISTORY_LEN)

# ── 网络/磁盘速率基准值 ────────────────────────────────────────────────────────
_prev_net  = psutil.net_io_counters()
_prev_disk = psutil.disk_io_counters()
_prev_time = time.monotonic()

# ── 初始化状态字段 ─────────────────────────────────────────────────────────────
def _init_state():
    with sys_state.batch():
        sys_state.cpu_total   = 0.0
        sys_state.cpu_cores   = []
        sys_state.mem_used    = 0
        sys_state.mem_total   = 0
        sys_state.mem_percent = 0.0
        sys_state.disk_read   = 0.0
        sys_state.disk_write  = 0.0
        sys_state.net_up      = 0.0
        sys_state.net_down    = 0.0
        sys_state.cpu_hist    = list(_cpu_hist)
        sys_state.mem_hist    = list(_mem_hist)
        sys_state.timestamp   = ""

# ── 采样函数 ──────────────────────────────────────────────────────────────────
def _sample():
    """采集一次系统数据，推送到 SharedState"""
    global _prev_net, _prev_disk, _prev_time

    now = time.monotonic()
    dt  = now - _prev_time or 1.0
    _prev_time = now

    # CPU
    cpu_total = psutil.cpu_percent(interval=None)
    cpu_cores = psutil.cpu_percent(interval=None, percpu=True)
    _cpu_hist.append(cpu_total)

    # 内存
    mem = psutil.virtual_memory()
    mem_used    = mem.used >> 20          # bytes → MB
    mem_total   = mem.total >> 20
    mem_percent = mem.percent
    _mem_hist.append(mem_percent)

    # 磁盘 I/O 速率
    disk = psutil.disk_io_counters()
    disk_read  = (disk.read_bytes  - _prev_disk.read_bytes)  / dt / 1024   # KB/s
    disk_write = (disk.write_bytes - _prev_disk.write_bytes) / dt / 1024
    _prev_disk = disk

    # 网络速率
    net = psutil.net_io_counters()
    net_up   = (net.bytes_sent - _prev_net.bytes_sent) / dt / 1024    # KB/s
    net_down = (net.bytes_recv - _prev_net.bytes_recv) / dt / 1024
    _prev_net = net

    # 推送（批量减少 JS 通知次数）
    with sys_state.batch():
        sys_state.cpu_total   = round(cpu_total, 1)
        sys_state.cpu_cores   = [round(c, 1) for c in cpu_cores]
        sys_state.mem_used    = mem_used
        sys_state.mem_total   = mem_total
        sys_state.mem_percent = round(mem_percent, 1)
        sys_state.disk_read   = round(max(disk_read,  0.0), 1)
        sys_state.disk_write  = round(max(disk_write, 0.0), 1)
        sys_state.net_up      = round(max(net_up,   0.0), 1)
        sys_state.net_down    = round(max(net_down, 0.0), 1)
        sys_state.cpu_hist    = list(_cpu_hist)
        sys_state.mem_hist    = list(_mem_hist)
        sys_state.timestamp   = datetime.now().strftime("%H:%M:%S")

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
    print("[SysMon] 窗口关闭")

# ── 加载 UI ───────────────────────────────────────────────────────────────────
app.load_html("""<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    html, body { width: 100%; height: 100%; overflow: hidden; }
    #root { width: 100%; height: 100%; }

    /* 无边框：拖拽区域 */
    .drag    { -webkit-app-region: drag; }
    .no-drag { -webkit-app-region: no-drag; }

    /* 无边框：窗口控制按钮 */
    .wc-btn {
      width: 13px; height: 13px;
      border-radius: 50%;
      border: none; cursor: pointer; padding: 0;
      display: inline-block; flex-shrink: 0;
    }
    .wc-pin      { background: #4f8ef7; -webkit-window-control: pin; }
    .wc-minimize { background: #64748b; -webkit-window-control: minimize; }
    .wc-maximize { background: #34d399; -webkit-window-control: maximize; }
    .wc-close    { background: #f87171; -webkit-window-control: close; }
  </style>
</head>
<body><div id="root"></div></body>
</html>""")

_init_state()

app.load_preact("ui/app.js")
app.run()

