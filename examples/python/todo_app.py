"""
LightUI Python示例 - Todo应用

功能：
- 完整的Todo应用
- Python和JavaScript交互
- 数据持久化

运行方法：
    python examples/python/todo_app.py
"""

import sys
import os
import json

# 添加bindings/python到路径
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..', 'bindings', 'python'))

import lightui

# 数据文件
DATA_FILE = "todos.json"

def load_todos():
    """从文件加载todos"""
    if os.path.exists(DATA_FILE):
        with open(DATA_FILE, 'r', encoding='utf-8') as f:
            return json.load(f)
    return []

def save_todos(todos):
    """保存todos到文件"""
    with open(DATA_FILE, 'w', encoding='utf-8') as f:
        json.dump(todos, f, indent=2, ensure_ascii=False)

def main():
    # 创建窗口
    app = lightui.Window("Todo App", 600, 800)
    
    # 绑定Python函数
    @app.bind("getTodos")
    def get_todos():
        """获取所有todos"""
        return load_todos()
    
    @app.bind("addTodo")
    def add_todo(text):
        """添加todo"""
        todos = load_todos()
        new_todo = {
            "id": len(todos) + 1,
            "text": text,
            "done": False
        }
        todos.append(new_todo)
        save_todos(todos)
        return new_todo
    
    @app.bind("toggleTodo")
    def toggle_todo(todo_id):
        """切换todo状态"""
        todos = load_todos()
        for todo in todos:
            if todo["id"] == todo_id:
                todo["done"] = not todo["done"]
                break
        save_todos(todos)
        return todos
    
    @app.bind("deleteTodo")
    def delete_todo(todo_id):
        """删除todo"""
        todos = load_todos()
        todos = [t for t in todos if t["id"] != todo_id]
        save_todos(todos)
        return todos
    
    # 加载UI
    app.load_ui("""
        import { render } from 'preact';
        import { useState, useEffect } from 'preact/hooks';
        
        function TodoApp() {
            const [todos, setTodos] = useState([]);
            const [input, setInput] = useState('');
            
            // 加载todos
            useEffect(() => {
                const loadedTodos = window.getTodos();
                setTodos(loadedTodos);
            }, []);
            
            // 添加todo
            const handleAdd = () => {
                if (input.trim()) {
                    const newTodo = window.addTodo(input);
                    setTodos([...todos, newTodo]);
                    setInput('');
                }
            };
            
            // 切换todo
            const handleToggle = (id) => {
                const updatedTodos = window.toggleTodo(id);
                setTodos(updatedTodos);
            };
            
            // 删除todo
            const handleDelete = (id) => {
                const updatedTodos = window.deleteTodo(id);
                setTodos(updatedTodos);
            };
            
            return (
                <div style={{
                    maxWidth: '600px',
                    margin: '0 auto',
                    padding: '20px',
                    fontFamily: 'Arial, sans-serif'
                }}>
                    <h1>Todo App</h1>
                    
                    {/* 输入框 */}
                    <div style={{ marginBottom: '20px' }}>
                        <input
                            type="text"
                            value={input}
                            onInput={(e) => setInput(e.target.value)}
                            onKeyPress={(e) => e.key === 'Enter' && handleAdd()}
                            placeholder="What needs to be done?"
                            style={{
                                width: '100%',
                                padding: '10px',
                                fontSize: '16px',
                                border: '1px solid #ddd',
                                borderRadius: '4px'
                            }}
                        />
                    </div>
                    
                    {/* Todo列表 */}
                    <ul style={{ listStyle: 'none', padding: 0 }}>
                        {todos.map(todo => (
                            <li key={todo.id} style={{
                                display: 'flex',
                                alignItems: 'center',
                                padding: '10px',
                                marginBottom: '10px',
                                backgroundColor: '#f9f9f9',
                                borderRadius: '4px'
                            }}>
                                <input
                                    type="checkbox"
                                    checked={todo.done}
                                    onChange={() => handleToggle(todo.id)}
                                    style={{ marginRight: '10px' }}
                                />
                                <span style={{
                                    flex: 1,
                                    textDecoration: todo.done ? 'line-through' : 'none',
                                    color: todo.done ? '#999' : '#333'
                                }}>
                                    {todo.text}
                                </span>
                                <button
                                    onClick={() => handleDelete(todo.id)}
                                    style={{
                                        padding: '5px 10px',
                                        backgroundColor: '#ff4444',
                                        color: 'white',
                                        border: 'none',
                                        borderRadius: '4px',
                                        cursor: 'pointer'
                                    }}
                                >
                                    Delete
                                </button>
                            </li>
                        ))}
                    </ul>
                    
                    {/* 统计 */}
                    <div style={{ marginTop: '20px', color: '#666' }}>
                        {todos.filter(t => !t.done).length} items left
                    </div>
                </div>
            );
        }
        
        render(<TodoApp />, document.body);
    """)
    
    # 运行
    app.run()

if __name__ == "__main__":
    main()

