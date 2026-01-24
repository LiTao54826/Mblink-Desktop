// LightUI Preact 待办事项应用 - 纯 JS 版本（用于测试 checkbox 重绘）
const { h, render, Component } = preact;
const { useState, useEffect, useRef } = preactHooks;

// 简化样式
const styles = `
* { box-sizing: border-box; margin: 0; padding: 0; }
body {
    font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Arial, sans-serif;
    background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
    min-height: 100vh;
    display: flex;
    align-items: center;
    justify-content: center;
    padding: 20px;
}
.app-container {
    width: 100%;
    max-width: 500px;
    background: white;
    border-radius: 16px;
    box-shadow: 0 20px 60px rgba(0, 0, 0, 0.3);
    overflow: hidden;
}
.app-header {
    background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
    color: white;
    padding: 24px;
    text-align: center;
}
.app-title { font-size: 28px; font-weight: 700; }
.input-section { padding: 16px; border-bottom: 1px solid #e9ecef; }
.input-wrapper { display: flex; gap: 12px; }
.todo-input {
    flex: 1;
    padding: 12px 16px;
    border: 2px solid #e9ecef;
    border-radius: 8px;
    font-size: 14px;
}
.todo-input:focus { outline: none; border-color: #667eea; }
.add-button {
    padding: 12px 24px;
    background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
    color: white;
    border: none;
    border-radius: 8px;
    font-size: 14px;
    font-weight: 600;
    cursor: pointer;
}
.todos-list { max-height: 400px; overflow-y: auto; }
.todo-item {
    display: flex;
    align-items: center;
    padding: 16px 20px;
    border-bottom: 1px solid #e9ecef;
}
.todo-checkbox { width: 20px; height: 20px; margin-right: 12px; cursor: pointer; }
.todo-text { flex: 1; font-size: 14px; color: #212529; }
.todo-text.completed { text-decoration: line-through; color: #adb5bd; }
.delete-btn {
    padding: 6px 12px;
    background: #dc3545;
    color: white;
    border: none;
    border-radius: 4px;
    font-size: 12px;
    cursor: pointer;
}
.empty-state { text-align: center; padding: 40px 20px; color: #6c757d; }
.stats { padding: 12px 20px; background: #f8f9fa; font-size: 13px; color: #6c757d; }
`;

// 生成唯一 ID
let nextId = 1;
const generateId = () => nextId++;

// 待办事项组件
function TodoItem({ todo, onToggle, onDelete }) {
    console.log(`[TodoItem] Rendering todo ${todo.id}: completed=${todo.completed}`);
    
    return h('div', { className: 'todo-item' },
        h('input', {
            type: 'checkbox',
            className: 'todo-checkbox',
            checked: todo.completed,
            onChange: () => {
                console.log(`[TodoItem] Checkbox clicked for todo ${todo.id}`);
                onToggle(todo.id);
            }
        }),
        h('div', {
            className: 'todo-text' + (todo.completed ? ' completed' : '')
        }, todo.text),
        h('button', {
            className: 'delete-btn',
            onClick: () => onDelete(todo.id)
        }, '删除')
    );
}

// 主应用组件
function App() {
    const [todos, setTodos] = useState([
        { id: generateId(), text: '学习 Preact', completed: false },
        { id: generateId(), text: '测试 Checkbox', completed: true },
        { id: generateId(), text: '修复重绘问题', completed: false }
    ]);
    const [inputText, setInputText] = useState('');

    const handleAdd = () => {
        if (inputText.trim()) {
            console.log('[App] Adding todo:', inputText);
            setTodos([...todos, { id: generateId(), text: inputText, completed: false }]);
            setInputText('');
        }
    };

    const handleToggle = (id) => {
        console.log('[App] Toggling todo:', id);
        setTodos(todos.map(todo =>
            todo.id === id ? { ...todo, completed: !todo.completed } : todo
        ));
    };

    const handleDelete = (id) => {
        console.log('[App] Deleting todo:', id);
        setTodos(todos.filter(todo => todo.id !== id));
    };

    const activeCount = todos.filter(t => !t.completed).length;
    const completedCount = todos.filter(t => t.completed).length;

    console.log('[App] Rendering, todos:', todos.length);

    return h('div', { className: 'app-container' },
        h('div', { className: 'app-header' },
            h('h1', { className: 'app-title' }, '✨ 待办事项')
        ),
        h('div', { className: 'input-section' },
            h('div', { className: 'input-wrapper' },
                h('input', {
                    type: 'text',
                    className: 'todo-input',
                    placeholder: '输入新的待办事项...',
                    value: inputText,
                    onInput: (e) => setInputText(e.target.value),
                    onKeyPress: (e) => e.key === 'Enter' && handleAdd()
                }),
                h('button', { className: 'add-button', onClick: handleAdd }, '添加')
            )
        ),
        h('div', { className: 'todos-list' },
            todos.length === 0
                ? h('div', { className: 'empty-state' }, '📝 还没有待办事项')
                : todos.map(todo => h(TodoItem, {
                    key: todo.id,
                    todo: todo,
                    onToggle: handleToggle,
                    onDelete: handleDelete
                }))
        ),
        h('div', { className: 'stats' },
            `待完成: ${activeCount} | 已完成: ${completedCount} | 总计: ${todos.length}`
        )
    );
}

