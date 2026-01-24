"""
LightUI Python Binding - Preact 待办事项应用示例

展示如何使用 Preact 构建交互式桌面应用：
- Preact 组件化开发
- Python 后端数据处理
- 状态管理和双向通信
- 本地存储持久化
"""

import sys
import os
import json
from datetime import datetime

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

from lightui import LightUIApp


def main():
    print("=" * 50)
    print("LightUI Preact 待办事项应用")
    print("=" * 50)
    
    # 数据存储文件
    data_file = os.path.join(os.path.dirname(__file__), 'todos.json')
    
    # 加载已保存的待办事项
    def load_todos():
        if os.path.exists(data_file):
            try:
                with open(data_file, 'r', encoding='utf-8') as f:
                    return json.load(f)
            except:
                return []
        return []
    
    # 保存待办事项
    def save_todos(todos):
        try:
            with open(data_file, 'w', encoding='utf-8') as f:
                json.dump(todos, f, ensure_ascii=False, indent=2)
            return True
        except Exception as e:
            print(f"保存失败: {e}")
            return False
    
    with LightUIApp("Preact 待办事项", 800, 700) as app:
        # 创建状态
        todos_state = app.state("todos", load_todos())
        filter_state = app.state("filter", "all")
        
        # 绑定：添加待办事项
        @app.bind("addTodo")
        def add_todo(text):
            """添加新的待办事项"""
            if not text or not text.strip():
                return {"success": False, "error": "内容不能为空"}
            
            todos = todos_state.get()
            new_todo = {
                "id": len(todos) + 1,
                "text": text.strip(),
                "completed": False,
                "createdAt": datetime.now().isoformat()
            }
            todos.append(new_todo)
            todos_state.set(todos)
            save_todos(todos)
            
            print(f"✅ 添加待办: {text}")
            return {"success": True, "todo": new_todo}
        
        # 绑定：切换完成状态
        @app.bind("toggleTodo")
        def toggle_todo(todo_id):
            """切换待办事项的完成状态"""
            todos = todos_state.get()
            # 创建新列表和新对象，确保引用变化
            new_todos = []
            for todo in todos:
                if todo["id"] == todo_id:
                    # 创建新对象而不是修改原对象
                    new_todo = {**todo, "completed": not todo["completed"]}
                    new_todos.append(new_todo)
                    print(f"🔄 切换状态: ID={todo_id}, {todo['completed']} -> {new_todo['completed']}")
                else:
                    new_todos.append({**todo})  # 也创建副本
            todos_state.set(new_todos)
            save_todos(new_todos)

            return {"success": True, "todos": new_todos}
        
        # 绑定：删除待办事项
        @app.bind("deleteTodo")
        def delete_todo(todo_id):
            """删除待办事项"""
            todos = todos_state.get()
            todos = [t for t in todos if t["id"] != todo_id]
            todos_state.set(todos)
            save_todos(todos)
            
            print(f"🗑️ 删除待办: ID={todo_id}")
            return {"success": True, "todos": todos}
        
        # 绑定：编辑待办事项
        @app.bind("editTodo")
        def edit_todo(args):
            """编辑待办事项"""
            todo_id = args.get("id")
            new_text = args.get("text", "").strip()
            
            if not new_text:
                return {"success": False, "error": "内容不能为空"}
            
            todos = todos_state.get()
            for todo in todos:
                if todo["id"] == todo_id:
                    todo["text"] = new_text
                    break
            todos_state.set(todos)
            save_todos(todos)
            
            print(f"✏️ 编辑待办: ID={todo_id}")
            return {"success": True, "todos": todos}
        
        # 绑定：清除已完成
        @app.bind("clearCompleted")
        def clear_completed():
            """清除所有已完成的待办事项"""
            todos = todos_state.get()
            todos = [t for t in todos if not t["completed"]]
            todos_state.set(todos)
            save_todos(todos)
            
            print("🧹 清除已完成")
            return {"success": True, "todos": todos}
        
        # 绑定：获取所有待办事项
        @app.bind("getTodos")
        def get_todos(args=None):
            """获取所有待办事项"""
            return todos_state.get()

        # 绑定：获取统计信息
        @app.bind("getStats")
        def get_stats(args=None):
            """获取统计信息"""
            todos = todos_state.get()
            total = len(todos)
            completed = sum(1 for t in todos if t["completed"])
            active = total - completed

            return {
                "total": total,
                "completed": completed,
                "active": active,
                "completionRate": round(completed / total * 100, 1) if total > 0 else 0
            }
        
        # 加载 Preact 库
        preact_path = os.path.join(os.path.dirname(__file__), '..', '..', '..', 'js', 'preact')
        preact_js = os.path.join(preact_path, 'preact.js')
        hooks_js = os.path.join(preact_path, 'hooks.js')

        # 先加载基础 HTML 文档（必须先有 document.body 和 document.head）
        print("📄 加载 HTML 文档...")
        app.load_html("""
        <!DOCTYPE html>
        <html>
        <head>
            <meta charset="UTF-8">
            <title>Preact 待办事项</title>
        </head>
        <body>
            <!-- Preact 会在这里渲染 -->
        </body>
        </html>
        """)

        print(f"📦 加载 Preact 库: {preact_js}")
        app.load_js_file(preact_js)

        print(f"📦 加载 Preact Hooks: {hooks_js}")
        app.load_js_file(hooks_js)

        # 加载 Preact UI
        js_file = os.path.join(os.path.dirname(__file__), 'preact_todo_app.js')
        # js_file = os.path.join(os.path.dirname(__file__), 'todo_checkbox_test.js')
        print(f"📦 加载应用代码: {js_file}")
        app.load_js_file(js_file)

        print("\n✅ Preact 待办事项应用已启动")
        print(f"📁 数据文件: {data_file}")
        print("📝 开始管理你的待办事项吧！")

        app.run()


if __name__ == "__main__":
    main()

