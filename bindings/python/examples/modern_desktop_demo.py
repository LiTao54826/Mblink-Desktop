"""
LightUI 待办事项示例

演示功能：
- host.on(stateName, callback) 自动响应列表状态变化重新渲染
- ListState 列表状态管理
- @app.bindable 多参数绑定
- py.funcName() 调用 Python 函数（无需返回值）

运行方式：
    python todo_app.py
"""

import sys
import os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

import lightui as ui

app = ui.App("MBinkUI示例", 500, 450, borderless=True, resizable=True, gpu=False)


app.load_preact(r'D:\code\C\MBink\examples\modern_desktop_demo\app.js')


app.run()
