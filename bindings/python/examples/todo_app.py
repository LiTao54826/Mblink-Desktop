"""
LightUI Python Binding - Todo 应用示例

完整的 Todo 应用，展示：
- 列表状态管理
- 复杂数据结构
- CRUD 操作
- Python ↔ JS 双向通信
"""

import sys
import os
import time

# 添加 lightui 包到路径
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

from lightui import LightUIApp


def main():
    print("=" * 50)
    print("LightUI Todo 应用示例")
    print("=" * 50)
    
    with LightUIApp("Todo App", 500, 600) as app:
        # 创建 Todo 列表状态
        todos = app.state("todos", [])
        next_id = app.state("nextId", 1)
        
        # 添加 Todo
        @app.bind("addTodo")
        def add_todo(args):
            if not args or not isinstance(args, dict):
                return {"error": "Invalid arguments"}
            
            text = args.get("text", "").strip()
            if not text:
                return {"error": "Text is required"}
            
            todo = {
                "id": next_id.get(),
                "text": text,
                "completed": False,
                "createdAt": int(time.time() * 1000)
            }
            
            todos.append(todo)
            next_id.increment()
            
            return {"success": True, "todo": todo}
        
        # 切换完成状态
        @app.bind("toggleTodo")
        def toggle_todo(args):
            if not args or not isinstance(args, dict):
                return {"error": "Invalid arguments"}
            
            todo_id = args.get("id")
            if todo_id is None:
                return {"error": "ID is required"}
            
            current_todos = todos.get()
            for i, todo in enumerate(current_todos):
                if todo["id"] == todo_id:
                    todo["completed"] = not todo["completed"]
                    todos[i] = todo
                    return {"success": True, "todo": todo}
            
            return {"error": "Todo not found"}
        
        # 删除 Todo
        @app.bind("deleteTodo")
        def delete_todo(args):
            if not args or not isinstance(args, dict):
                return {"error": "Invalid arguments"}
            
            todo_id = args.get("id")
            if todo_id is None:
                return {"error": "ID is required"}
            
            current_todos = todos.get()
            for i, todo in enumerate(current_todos):
                if todo["id"] == todo_id:
                    todos.remove(i)
                    return {"success": True}
            
            return {"error": "Todo not found"}
        
        # 获取所有 Todo
        @app.bind("getTodos")
        def get_todos(args):
            return todos.get()
        
        # 清除已完成
        @app.bind("clearCompleted")
        def clear_completed(args):
            current_todos = todos.get()
            # 从后往前删除，避免索引问题
            for i in range(len(current_todos) - 1, -1, -1):
                if current_todos[i]["completed"]:
                    todos.remove(i)
            return {"success": True}
        
        # 获取统计信息
        @app.bind("getStats")
        def get_stats(args):
            current_todos = todos.get()
            total = len(current_todos)
            completed = sum(1 for t in current_todos if t["completed"])
            return {
                "total": total,
                "completed": completed,
                "active": total - completed
            }
        
        # 加载 UI
        app.load_html('''
            <!DOCTYPE html>
            <html>
            <head>
                <style>
                    * {
                        box-sizing: border-box;
                    }
                    body {
                        font-family: 'Segoe UI', Arial, sans-serif;
                        max-width: 500px;
                        margin: 0 auto;
                        padding: 20px;
                        background: #f5f5f5;
                    }
                    h1 {
                        text-align: center;
                        color: #333;
                    }
                    .input-container {
                        display: flex;
                        gap: 10px;
                        margin-bottom: 20px;
                    }
                    input[type="text"] {
                        flex: 1;
                        padding: 12px;
                        font-size: 16px;
                        border: 2px solid #ddd;
                        border-radius: 8px;
                        outline: none;
                    }
                    input[type="text"]:focus {
                        border-color: #4CAF50;
                    }
                    button {
                        padding: 12px 20px;
                        font-size: 16px;
                        border: none;
                        border-radius: 8px;
                        cursor: pointer;
                        transition: background 0.2s;
                    }
                    .add-btn {
                        background: #4CAF50;
                        color: white;
                    }
                    .add-btn:hover {
                        background: #45a049;
                    }
                    .todo-list {
                        list-style: none;
                        padding: 0;
                    }
                    .todo-item {
                        display: flex;
                        align-items: center;
                        padding: 15px;
                        background: white;
                        border-radius: 8px;
                        margin-bottom: 10px;
                        box-shadow: 0 2px 4px rgba(0,0,0,0.1);
                    }
                    .todo-item.completed .todo-text {
                        text-decoration: line-through;
                        color: #999;
                    }
                    .todo-checkbox {
                        width: 24px;
                        height: 24px;
                        margin-right: 15px;
                        cursor: pointer;
                    }
                    .todo-text {
                        flex: 1;
                        font-size: 16px;
                    }
                    .delete-btn {
                        background: #f44336;
                        color: white;
                        padding: 8px 12px;
                        font-size: 14px;
                    }
                    .delete-btn:hover {
                        background: #d32f2f;
                    }
                    .stats {
                        display: flex;
                        justify-content: space-between;
                        align-items: center;
                        padding: 15px;
                        background: white;
                        border-radius: 8px;
                        margin-top: 20px;
                    }
                    .clear-btn {
                        background: #ff9800;
                        color: white;
                        padding: 8px 16px;
                        font-size: 14px;
                    }
                    .clear-btn:hover {
                        background: #f57c00;
                    }
                    .empty-message {
                        text-align: center;
                        color: #999;
                        padding: 40px;
                    }
                </style>
            </head>
            <body>
                <h1>📝 Todo App</h1>
                
                <div class="input-container">
                    <input type="text" id="todoInput" placeholder="What needs to be done?" 
                           onkeypress="if(event.key==='Enter') addTodo()">
                    <button class="add-btn" onclick="addTodo()">Add</button>
                </div>
                
                <ul class="todo-list" id="todoList"></ul>
                
                <div class="stats" id="stats" style="display: none;">
                    <span id="statsText"></span>
                    <button class="clear-btn" onclick="clearCompleted()">Clear Completed</button>
                </div>
                
                <script>
                    function render() {
                        const todos = host.call('getTodos', null);
                        const stats = host.call('getStats', null);
                        const list = document.getElementById('todoList');
                        const statsDiv = document.getElementById('stats');
                        const statsText = document.getElementById('statsText');
                        
                        if (todos.length === 0) {
                            list.innerHTML = '<li class="empty-message">No todos yet. Add one above!</li>';
                            statsDiv.style.display = 'none';
                        } else {
                            list.innerHTML = todos.map(todo => `
                                <li class="todo-item ${todo.completed ? 'completed' : ''}">
                                    <input type="checkbox" class="todo-checkbox" 
                                           ${todo.completed ? 'checked' : ''} 
                                           onchange="toggleTodo(${todo.id})">
                                    <span class="todo-text">${escapeHtml(todo.text)}</span>
                                    <button class="delete-btn" onclick="deleteTodo(${todo.id})">Delete</button>
                                </li>
                            `).join('');
                            
                            statsDiv.style.display = 'flex';
                            statsText.textContent = `${stats.active} active, ${stats.completed} completed`;
                        }
                    }
                    
                    function escapeHtml(text) {
                        const div = document.createElement('div');
                        div.textContent = text;
                        return div.innerHTML;
                    }
                    
                    function addTodo() {
                        const input = document.getElementById('todoInput');
                        const text = input.value.trim();
                        if (text) {
                            host.call('addTodo', {text: text});
                            input.value = '';
                            render();
                        }
                    }
                    
                    function toggleTodo(id) {
                        host.call('toggleTodo', {id: id});
                        render();
                    }
                    
                    function deleteTodo(id) {
                        host.call('deleteTodo', {id: id});
                        render();
                    }
                    
                    function clearCompleted() {
                        host.call('clearCompleted', null);
                        render();
                    }
                    
                    // 初始渲染
                    render();
                </script>
            </body>
            </html>
        ''')
        
        print("Todo 应用已创建")
        print("\n演示 Python 端操作：")
        
        # 添加一些示例 Todo
        add_todo({"text": "Learn LightUI"})
        add_todo({"text": "Build awesome apps"})
        add_todo({"text": "Share with friends"})
        
        print(f"添加了 3 个 Todo")
        print(f"当前 Todo 列表: {todos.get()}")
        
        # 完成第一个
        toggle_todo({"id": 1})
        print(f"\n完成第一个 Todo 后:")
        print(f"统计: {get_stats(None)}")
        
        # 运行事件循环
        # app.run()  # 取消注释以运行窗口


if __name__ == "__main__":
    main()
