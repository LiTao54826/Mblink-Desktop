"""资源包示例：编译 todo_app UI 为资源包并挂载后按本地路径加载。"""

import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

import mblink as ui

ROOT = os.path.dirname(__file__)
UI_DIR = os.path.join(ROOT, 'todo_app')
PACKAGE_FILE = os.path.join(ROOT, 'todo_app.mbrp')
KEY = 'mblink-demo-key'
print(UI_DIR)
ui.compile_resources(UI_DIR, PACKAGE_FILE, KEY)

app = ui.App('Resource Package Demo', 480, 600)
state = app.shared('state')
state.todos = []
state.filter = 'all'
app.mount_resource_package(PACKAGE_FILE, KEY)
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
""")
app.load_js_file('./todo_app/ui/app.js')
app.run()

