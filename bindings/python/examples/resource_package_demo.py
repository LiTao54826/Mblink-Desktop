"""资源包示例：编译 todo_app UI 为资源包并挂载后按本地路径加载。"""

import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

import mbink as ui

ROOT = os.path.dirname(__file__)
UI_DIR = os.path.join(ROOT, '信息', 'ui')
PACKAGE_FILE = os.path.join(ROOT, 'zonghe.mbrp')
KEY = 'mbink-demo-key'

ui.compile_resources(UI_DIR, PACKAGE_FILE, KEY)

app = ui.App('Resource Package Demo', 480, 600)
state = app.shared('state')
state.todos = []
state.filter = 'all'

app.mount_resource_package(PACKAGE_FILE, KEY)
app.load_html("""<!DOCTYPE html>
<html><head><meta charset='utf-8'>
<style>* { box-sizing: border-box; margin: 0; padding: 0; }</style>
</head><body><div id='root'></div></body></html>""")
app.load_js_file('/ui/app.js')
app.run()

