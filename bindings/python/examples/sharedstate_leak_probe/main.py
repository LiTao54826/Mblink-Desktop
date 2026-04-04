import os
import sys
import time
from datetime import datetime

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..'))

from mbink import App

WINDOW_W = 980
WINDOW_H = 720
HEARTBEAT_INTERVAL_SEC = 1.0

app = App(
    'SharedState Leak Probe',
    WINDOW_W,
    WINDOW_H,
    borderless=False,
    resizable=True,
    gpu=False,
)

sys_state = app.shared('sys')
sys_state.heartbeat_at = datetime.now().strftime('%H:%M:%S')
sys_state.tick = 0
sys_state.note = 'watch heartbeat only'

app.load_html_file('./index.html')

last_tick = time.monotonic()


@app.on_update
def _(delta):
    del delta
    now = time.monotonic()
    if now - last_tick_ref[0] < HEARTBEAT_INTERVAL_SEC:
        return
    last_tick_ref[0] = now
    sys_state.tick = sys_state.tick + 1
    sys_state.heartbeat_at = datetime.now().strftime('%H:%M:%S')
    if sys_state.tick % 10 == 0:
        print(
            f"[PY_HEARTBEAT] tick={sys_state.tick} heartbeat_at={sys_state.heartbeat_at}",
            flush=True,
        )


last_tick_ref = [last_tick]

app.load_js_file('./ui/app.js')
app.run()
