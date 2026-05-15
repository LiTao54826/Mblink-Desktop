import os
import sys
import time
import random
import threading
from datetime import datetime

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..'))
from mbink import App

app = App("MBink Threading Demo", 1100, 760, gpu=False)

state = app.shared("demo")
stop_event = threading.Event()
workers = []
workers_lock = threading.Lock()

with state.batch():
    state.running = False
    state.worker_count = 0
    state.tick = 0
    state.last_worker = "-"
    state.last_value = 0
    state.last_log = "ready"
    state.last_test = "not started"
    state.last_test_mode = "-"

app.load_html_file('./ui/index.html')
logs = app.logview("logs")
term = app.terminal("term")


def worker_loop(name: str):
    local_tick = 0
    while not stop_event.is_set():
        local_tick += 1
        value = random.randint(1, 999)
        with state.batch():
            state.running = True
            state.tick = int(state.tick or 0) + 1
            state.last_worker = name
            state.last_value = value
            state.last_log = f"{name} -> value={value}"
        logs.append("INFO", name, f"push value={value} tick={local_tick}")
        term.write(f"[{datetime.now():%H:%M:%S}] {name}: value={value}\r\n")
        time.sleep(0.5 + random.random() * 0.7)
    logs.append("WARN", name, "worker stopped")
    term.write(f"[{datetime.now():%H:%M:%S}] {name}: stopped\r\n")


def start_workers(count: int = 3):
    with workers_lock:
        if workers:
            return False
        stop_event.clear()
        for i in range(count):
            t = threading.Thread(target=worker_loop, args=(f"worker-{i+1}",), daemon=True)
            workers.append(t)
            t.start()
        state.running = True
        state.worker_count = len(workers)
    logs.append("INFO", "main", f"started {count} workers")
    term.write(f"[{datetime.now():%H:%M:%S}] main: started {count} workers\r\n")
    return True


def stop_workers():
    stop_event.set()
    with workers_lock:
        current = list(workers)
        workers.clear()
    for t in current:
        t.join(timeout=1.5)
    with state.batch():
        state.running = False
        state.worker_count = 0
        state.last_log = "all workers stopped"
    logs.append("WARN", "main", "all workers stopped")
    term.write(f"[{datetime.now():%H:%M:%S}] main: all workers stopped\r\n")
    return True


def blocking_test(mode: str, seconds: float = 3.0):
    logs.append("INFO", "test", f"{mode} begin: sleep {seconds:.1f}s")
    term.write(f"[{datetime.now():%H:%M:%S}] test: {mode} begin sleep {seconds:.1f}s\r\n")
    with state.batch():
        state.last_test_mode = mode
        state.last_test = f"running {mode}..."
    time.sleep(seconds)
    done = f"done {mode} at {datetime.now():%H:%M:%S}"
    with state.batch():
        state.last_test = done
    logs.append("INFO", "test", done)
    term.write(f"[{datetime.now():%H:%M:%S}] test: {done}\r\n")
    return {"ok": True, "mode": mode, "slept": seconds, "done": done}


@app.bind("start_workers")
def _start(_args):
    started = start_workers()
    return {"ok": True, "started": started}


@app.bind("stop_workers")
def _stop(_args):
    stopped = stop_workers()
    return {"ok": True, "stopped": stopped}


@app.bind("block_sync")
def _block_sync(args):
    seconds = float((args or {}).get("seconds", 3))
    return blocking_test("bind(sync)", seconds)


@app.bind_async("block_async")
def _block_async(args):
    seconds = float((args or {}).get("seconds", 3))
    return blocking_test("bind_async", seconds)


@app.on_close
def _close():
    stop_workers()


if __name__ == "__main__":
    app.run()

