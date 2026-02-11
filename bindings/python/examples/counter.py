"""
LightUI 计数器示例

演示功能：
- host.on(stateName, callback) 自动响应状态变化更新 DOM
- @app.bindable 装饰器自动注册函数
- IntState 状态管理
- py.funcName() 调用 Python 函数（无需返回值）

运行方式：
    python counter.py
"""

import sys
import os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

import lightui as ui

app = ui.App("LightUI 计数器", 400, 300)
counter = app.state("counter", 0)


@app.bindable
def increment():
    global counter
    counter += 1


@app.bindable
def decrement():
    global counter
    counter -= 1


app.load_html("""
<!DOCTYPE html>
<html>
<head>
    <style>
        body {
            font-family: Arial, sans-serif;
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: center;
            height: 100vh;
            margin: 0;
            background: #f0f0f0;
        }
        h1 { color: #333; }
        #count {
            font-size: 48px;
            font-weight: bold;
            color: #2196F3;
            margin: 20px 0;
        }
        .buttons { display: flex; gap: 10px; }
        button {
            font-size: 20px;
            padding: 10px 24px;
            border: none;
            border-radius: 6px;
            cursor: pointer;
            color: white;
            transition: opacity 0.2s;
        }
        button:hover { opacity: 0.85; }
        .btn-inc { background: #4CAF50; }
        .btn-dec { background: #f44336; }
    </style>
</head>
<body>
    <h1>LightUI 计数器</h1>
    <div id="count">0</div>
    <div class="buttons">
        <button class="btn-dec" onclick="py.decrement()">- 减少</button>
        <button class="btn-inc" onclick="py.increment()">+ 增加</button>
    </div>
    <script>
        host.on("counter", function(val) {
            document.getElementById("count").textContent = val;
        });
    </script>
</body>
</html>
""")

app.run()
