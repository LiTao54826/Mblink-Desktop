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

app = ui.App("LightUI 待办事项", 500, 450)
todos = app.state("todos", [])


@app.bindable
def addTodo(text):
    text = str(text) if text else ""
    if text.strip():
        todos.append(text.strip())


@app.bindable
def removeTodo(index):
    index = int(index) if index is not None else -1
    if 0 <= index < len(todos):
        todos.remove(index)


app.load_html("""
<!DOCTYPE html>
<html>
<head>
    <style>
        body {
            font-family: Arial, sans-serif;
            max-width: 460px;
            margin: 20px auto;
            padding: 0 10px;
            background: #f5f5f5;
        }
        h1 { color: #333; text-align: center; }
        .input-row { display: flex; gap: 8px; margin-bottom: 16px; }
        input[type="text"] {
            flex: 1; padding: 8px 12px; font-size: 14px;
            border: 1px solid #ccc; border-radius: 4px; outline: none;
        }
        input[type="text"]:focus { border-color: #2196F3; }
        .btn-add {
            padding: 8px 16px; font-size: 14px; border: none;
            border-radius: 4px; background: #4CAF50; color: white; cursor: pointer;
        }
        .btn-add:hover { opacity: 0.85; }
        ul { list-style: none; padding: 0; }
        li {
            display: flex; justify-content: space-between; align-items: center;
            padding: 10px 12px; margin-bottom: 6px; background: white;
            border-radius: 4px; box-shadow: 0 1px 2px rgba(0,0,0,0.1);
        }
        .btn-remove {
            padding: 4px 10px; font-size: 12px; border: none;
            border-radius: 3px; background: #f44336; color: white; cursor: pointer;
        }
        .btn-remove:hover { opacity: 0.85; }
        .empty-msg { text-align: center; color: #999; padding: 20px; }
    </style>
</head>
<body>
    <h1>待办事项</h1>
    <div class="input-row">
        <input type="text" id="todoInput" placeholder="输入待办事项..." />
        <button class="btn-add" onclick="doAdd()">添加</button>
    </div>
    <ul id="todoList">
        <div class="empty-msg">暂无待办事项</div>
    </ul>

    <script>
        function doAdd() {
            var input = document.getElementById('todoInput');
            var text = input.value.trim();
            if (!text) return;
            py.addTodo(text);
            input.value = '';
        }

        // 用 JS 注册 keydown，避免内联属性中 event 不可用的问题
        document.getElementById('todoInput').addEventListener('keydown', function(e) {
            if (e.key === 'Enter') doAdd();
        });

        function renderList(items) {
            var ul = document.getElementById('todoList');
            ul.innerHTML = '';
            if (!items || items.length === 0) {
                ul.innerHTML = '<div class="empty-msg">暂无待办事项</div>';
                return;
            }
            for (var i = 0; i < items.length; i++) {
                var li = document.createElement('li');
                li.innerHTML =
                    '<span>' + items[i] + '</span>' +
                    '<button class="btn-remove" onclick="py.removeTodo(' + i + ')">删除</button>';
                ul.appendChild(li);
            }
        }

        // 监听 todos 状态变化，自动重新渲染列表
        host.on("todos", function(items) {
            renderList(items);
        });
    </script>
</body>
</html>
""")

app.run()
