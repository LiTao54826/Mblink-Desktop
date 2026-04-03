from datetime import datetime
import sys
import os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..'))

from mbink import App

app = App("Native Controls Demo", 1100, 760, gpu=False)

app.load_html("""<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <style>
    * { box-sizing: border-box; }
    html, body { width: 100%; height: 100%; margin: 0; background: #0f172a; color: #e2e8f0; }
    body { font-family: "Segoe UI", Arial, sans-serif; }
    .layout { display: grid; grid-template-columns: 320px 1fr; gap: 12px; width: 100vw; height: 100vh; padding: 12px; }
    .panel { background: #111827; border: 1px solid #334155; border-radius: 10px; padding: 12px; display: flex; flex-direction: column; gap: 10px; }
    .title { font-size: 18px; font-weight: 700; }
    .desc { font-size: 12px; color: #94a3b8; line-height: 1.5; }
    .actions { display: grid; grid-template-columns: 1fr 1fr; gap: 8px; }
    button { border: 0; border-radius: 8px; padding: 10px 12px; font-size: 14px; cursor: pointer; background: #2563eb; color: white; }
    button.secondary { background: #475569; }
    button.warn { background: #d97706; }
    button.danger { background: #dc2626; }
    .status { padding: 10px; border-radius: 8px; background: #0b1220; border: 1px solid #223047; font-family: Consolas, monospace; font-size: 12px; white-space: pre-wrap; }
    .right { display: grid; grid-template-rows: 1fr 1fr; gap: 12px; min-height: 0; }
    logview, terminal { width: 100%; height: 100%; display: block; min-height: 0; background: #020617; border: 1px solid #334155; border-radius: 10px; overflow: hidden; }
  </style>
</head>
<body>
  <div class="layout">
    <div class="panel">
      <div class="title">MBink Native Controls</div>
      <div class="desc">左侧是 Preact 控制面板；右侧直接使用原生 &lt;logview&gt; 与 &lt;terminal&gt; 元素，由 Python 通过 C API 直接操作。</div>
      <div id="root"></div>
      <div class="status" id="status"></div>
    </div>
    <div class="right">
      <logview id="logs"></logview>
      <terminal id="term"></terminal>
    </div>
  </div>
</body>
</html>""")

logs = app.logview("logs")
term = app.terminal("term")
state = app.shared("demo")
state.last_action = "ready"
state.command_count = 0
state.last_log = "demo booted"
state.started_at = datetime.now().strftime("%H:%M:%S")


def mark(action: str, message: str, level: str = "INFO"):
    state.last_action = action
    state.last_log = message
    state.command_count = int(state.command_count or 0) + 1
    logs.append(level, "python", message)


logs.append("INFO", "python", "native controls demo booted")
term.write("MBink terminal ready.\r\n")
term.write("Type actions from the left panel.\r\n")


@app.bind("append_info")
def _append_info(_args):
    mark("append_info", "manual info log appended")
    return {"ok": True}


@app.bind("append_error")
def _append_error(_args):
    mark("append_error", "manual error log appended", "ERROR")
    return {"ok": True}


@app.bind("clear_logs")
def _clear_logs(_args):
    logs.clear()
    state.last_action = "clear_logs"
    state.last_log = "logs cleared"
    return {"ok": True}


@app.bind("write_terminal")
def _write_terminal(_args):
    now = datetime.now().strftime("%H:%M:%S")
    term.write(f"[{now}] hello from python terminal.write()\\r\\n")
    mark("write_terminal", "terminal.write called")
    return {"ok": True}


@app.bind("run_command")
def _run_command(_args):
    term.execute("echo hello from terminal.execute")
    mark("run_command", "terminal.execute called")
    return {"ok": True}


@app.bind("clear_terminal")
def _clear_terminal(_args):
    term.clear()
    state.last_action = "clear_terminal"
    state.last_log = "terminal cleared"
    return {"ok": True}


@app.bind("export_logs")
def _export_logs(_args):
    content = logs.export("text")
    preview = content[-240:] if content else "<empty>"
    state.last_action = "export_logs"
    state.last_log = f"exported {len(content)} chars"
    return {"ok": True, "preview": preview}


@app.bind("snapshot_terminal")
def _snapshot_terminal(_args):
    content = term.serialize()
    preview = content[-240:] if content else "<empty>"
    state.last_action = "snapshot_terminal"
    state.last_log = f"snapshot {len(content)} chars"
    return {"ok": True, "preview": preview}


app.load_preact("ui/app.js")
app.run()

