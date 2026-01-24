// LightUI Preact 待办事项应用
// 使用 Preact 构建交互式 UI

const { h, render, Component } = preact;
const { useState, useEffect, useRef } = preactHooks;

// 样式定义
const styles = `
* {
    box-sizing: border-box;
    margin: 0;
    padding: 0;
}

body {
    font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, "Helvetica Neue", Arial, sans-serif;
    background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
    min-height: 100vh;
    display: flex;
    align-items: center;
    justify-content: center;
    padding: 20px;
}

.app-container {
    width: 100%;
    max-width: 600px;
    background: white;
    border-radius: 16px;
    box-shadow: 0 20px 60px rgba(0, 0, 0, 0.3);
    overflow: hidden;
}

.app-header {
    background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
    color: white;
    padding: 32px 24px;
    text-align: center;
}

.app-title {
    font-size: 32px;
    font-weight: 700;
    margin-bottom: 8px;
}

.app-subtitle {
    font-size: 14px;
    opacity: 0.9;
}

.stats-bar {
    display: flex;
    justify-content: space-around;
    padding: 16px;
    background: #f8f9fa;
    border-bottom: 1px solid #e9ecef;
}

.stat-item {
    text-align: center;
}

.stat-value {
    font-size: 24px;
    font-weight: 700;
    color: #667eea;
}

.stat-label {
    font-size: 12px;
    color: #6c757d;
    margin-top: 4px;
}

.input-section {
    padding: 20px;
    border-bottom: 1px solid #e9ecef;
}

.input-wrapper {
    display: flex;
    gap: 12px;
}

.todo-input {
    flex: 1;
    padding: 12px 16px;
    border: 2px solid #e9ecef;
    border-radius: 8px;
    font-size: 14px;
    transition: all 0.2s;
}

.todo-input:focus {
    outline: none;
    border-color: #667eea;
}

.add-button {
    padding: 12px 24px;
    background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
    color: white;
    border: none;
    border-radius: 8px;
    font-size: 14px;
    font-weight: 600;
    cursor: pointer;
    transition: all 0.2s;
}

.add-button:hover {
    transform: translateY(-2px);
    box-shadow: 0 4px 12px rgba(102, 126, 234, 0.4);
}

.add-button:active {
    transform: translateY(0);
}

.filter-tabs {
    display: flex;
    padding: 16px 20px;
    gap: 8px;
    border-bottom: 1px solid #e9ecef;
}

.filter-tab {
    padding: 8px 16px;
    background: transparent;
    border: 1px solid #e9ecef;
    border-radius: 6px;
    font-size: 13px;
    cursor: pointer;
    transition: all 0.2s;
}

.filter-tab:hover {
    background: #f8f9fa;
}

.filter-tab.active {
    background: #667eea;
    color: white;
    border-color: #667eea;
}

.todos-list {
    max-height: 400px;
    overflow-y: auto;
}

.todo-item {
    display: flex;
    align-items: center;
    padding: 16px 20px;
    border-bottom: 1px solid #e9ecef;
    transition: background 0.2s;
}

.todo-item:hover {
    background: #f8f9fa;
}

.todo-checkbox {
    width: 20px;
    height: 20px;
    margin-right: 12px;
    cursor: pointer;
}

.todo-text {
    flex: 1;
    font-size: 14px;
    color: #212529;
}

.todo-text.completed {
    text-decoration: line-through;
    color: #adb5bd;
}

.todo-actions {
    display: flex;
    gap: 8px;
}

.action-button {
    padding: 6px 12px;
    background: transparent;
    border: 1px solid #e9ecef;
    border-radius: 4px;
    font-size: 12px;
    cursor: pointer;
    transition: all 0.2s;
}

.action-button:hover {
    background: #f8f9fa;
}

.action-button.delete:hover {
    background: #dc3545;
    color: white;
    border-color: #dc3545;
}

.empty-state {
    text-align: center;
    padding: 60px 20px;
    color: #6c757d;
}

.empty-icon {
    font-size: 48px;
    margin-bottom: 16px;
}

.empty-text {
    font-size: 16px;
}

.footer {
    padding: 16px 20px;
    background: #f8f9fa;
    display: flex;
    justify-content: space-between;
    align-items: center;
}

.footer-text {
    font-size: 13px;
    color: #6c757d;
}

.clear-button {
    padding: 8px 16px;
    background: #dc3545;
    color: white;
    border: none;
    border-radius: 6px;
    font-size: 13px;
    cursor: pointer;
    transition: all 0.2s;
}

.clear-button:hover {
    background: #c82333;
}

.clear-button:disabled {
    background: #e9ecef;
    color: #adb5bd;
    cursor: not-allowed;
}
`;

// 统计栏组件
function StatsBar({ stats }) {
    return h('div', { className: 'stats-bar' },
        h('div', { className: 'stat-item' },
            h('div', { className: 'stat-value' }, stats.total),
            h('div', { className: 'stat-label' }, '总计')
        ),
        h('div', { className: 'stat-item' },
            h('div', { className: 'stat-value' }, stats.active),
            h('div', { className: 'stat-label' }, '待完成')
        ),
        h('div', { className: 'stat-item' },
            h('div', { className: 'stat-value' }, stats.completed),
            h('div', { className: 'stat-label' }, '已完成')
        )
    );
}

// 输入区域组件
function InputSection({ onAdd }) {
    const [text, setText] = useState('');

    const handleSubmit = () => {
        if (text.trim()) {
            onAdd(text);
            setText('');
        }
    };

    const handleKeyPress = (e) => {
        if (e.key === 'Enter') {
            handleSubmit();
        }
    };

    return h('div', { className: 'input-section' },
        h('div', { className: 'input-wrapper' },
            h('input', {
                type: 'text',
                className: 'todo-input',
                placeholder: '输入新的待办事项...',
                value: text,
                onInput: (e) => setText(e.target.value),
                onKeyPress: handleKeyPress
            }),
            h('button', {
                className: 'add-button',
                onClick: handleSubmit
            }, '添加')
        )
    );
}

// 过滤标签组件
function FilterTabs({ filter, onFilterChange }) {
    const filters = [
        { id: 'all', label: '全部' },
        { id: 'active', label: '待完成' },
        { id: 'completed', label: '已完成' }
    ];

    return h('div', { className: 'filter-tabs' },
        filters.map(f =>
            h('button', {
                key: f.id,
                className: 'filter-tab' + (filter === f.id ? ' active' : ''),
                onClick: () => onFilterChange(f.id)
            }, f.label)
        )
    );
}

// 待办事项组件
function TodoItem({ todo, onToggle, onDelete, onEdit }) {
    const [isEditing, setIsEditing] = useState(false);
    const [editText, setEditText] = useState(todo.text);
    const inputRef = useRef(null);

    useEffect(() => {
        if (isEditing && inputRef.current) {
            inputRef.current.focus();
        }
    }, [isEditing]);

    const handleEdit = () => {
        if (editText.trim() && editText !== todo.text) {
            onEdit({ id: todo.id, text: editText });
        }
        setIsEditing(false);
    };

    const handleKeyPress = (e) => {
        if (e.key === 'Enter') {
            handleEdit();
        } else if (e.key === 'Escape') {
            setEditText(todo.text);
            setIsEditing(false);
        }
    };

    if (isEditing) {
        return h('div', { className: 'todo-item' },
            h('input', {
                ref: inputRef,
                type: 'text',
                className: 'todo-input',
                value: editText,
                onInput: (e) => setEditText(e.target.value),
                onKeyPress: handleKeyPress,
                onBlur: handleEdit
            })
        );
    }

    return h('div', { className: 'todo-item' },
        h('input', {
            type: 'checkbox',
            className: 'todo-checkbox',
            checked: todo.completed,
            onChange: () => onToggle(todo.id)
        }),
        h('div', {
            className: 'todo-text' + (todo.completed ? ' completed' : ''),
            onDblClick: () => setIsEditing(true)
        }, todo.text),
        h('div', { className: 'todo-actions' },
            h('button', {
                className: 'action-button',
                onClick: () => setIsEditing(true)
            }, '编辑'),
            h('button', {
                className: 'action-button delete',
                onClick: () => onDelete(todo.id)
            }, '删除')
        )
    );
}

// 待办列表组件
function TodosList({ todos, filter, onToggle, onDelete, onEdit }) {
    const filteredTodos = todos.filter(todo => {
        if (filter === 'active') return !todo.completed;
        if (filter === 'completed') return todo.completed;
        return true;
    });

    if (filteredTodos.length === 0) {
        return h('div', { className: 'empty-state' },
            h('div', { className: 'empty-icon' }, '📝'),
            h('div', { className: 'empty-text' },
                filter === 'completed' ? '还没有完成的待办事项' :
                filter === 'active' ? '太棒了！没有待完成的事项' :
                '还没有待办事项，添加一个吧！'
            )
        );
    }

    return h('div', { className: 'todos-list' },
        filteredTodos.map(todo =>
            h(TodoItem, {
                key: todo.id,
                todo: todo,
                onToggle: onToggle,
                onDelete: onDelete,
                onEdit: onEdit
            })
        )
    );
}

// 底部栏组件
function Footer({ stats, onClearCompleted }) {
    return h('div', { className: 'footer' },
        h('div', { className: 'footer-text' },
            `${stats.active} 项待完成`
        ),
        h('button', {
            className: 'clear-button',
            disabled: stats.completed === 0,
            onClick: onClearCompleted
        }, `清除已完成 (${stats.completed})`)
    );
}

// 主应用组件
function App() {
    const [todos, setTodos] = useState([]);
    const [filter, setFilter] = useState('all');
    const [stats, setStats] = useState({ total: 0, active: 0, completed: 0 });

    // 从 Python 加载初始数据
    useEffect(() => {
        // 加载初始数据
        const result = py.getStats();
        if (result) {
            setStats(result);
        }

        // 获取初始待办列表
        const initialTodos = py.getTodos();
        if (initialTodos) {
            setTodos(initialTodos);
        }
    }, []);

    // 刷新数据
    const refreshData = () => {
        const newTodos = py.getTodos();
        if (newTodos) {
            setTodos(newTodos);
        }

        const result = py.getStats();
        if (result) {
            setStats(result);
        }
    };

    // 添加待办事项
    const handleAdd = (text) => {
        console.log('[handleAdd] text:', text);
        const result = py.addTodo(text);
        console.log('[handleAdd] result:', JSON.stringify(result));
        if (result && result.success) {
            console.log('[handleAdd] calling refreshData');
            refreshData();
            console.log('[handleAdd] after refreshData, todos:', JSON.stringify(todos));
        }
    };

    // 切换完成状态
    const handleToggle = (id) => {
        console.log('[handleToggle] id:', id);
        const result = py.toggleTodo(id);
        console.log('[handleToggle] result:', JSON.stringify(result));
        if (result && result.success) {
            console.log('[handleToggle] calling refreshData');
            refreshData();
            console.log('[handleToggle] after refreshData, todos:', JSON.stringify(todos));
        }
    };

    // 删除待办事项
    const handleDelete = (id) => {
        const result = py.deleteTodo(id);
        if (result && result.success) {
            refreshData();
        }
    };

    // 编辑待办事项
    const handleEdit = (args) => {
        const result = py.editTodo(args);
        if (result && result.success) {
            refreshData();
        }
    };

    // 清除已完成
    const handleClearCompleted = () => {
        const result = py.clearCompleted();
        if (result && result.success) {
            refreshData();
        }
    };

    return h('div', { className: 'app-container' },
        h('div', { className: 'app-header' },
            h('h1', { className: 'app-title' }, '✨ 待办事项'),
            h('p', { className: 'app-subtitle' }, 'Preact + Python 驱动')
        ),
        h(StatsBar, { stats: stats }),
        h(InputSection, { onAdd: handleAdd }),
        h(FilterTabs, { filter: filter, onFilterChange: setFilter }),
        h(TodosList, {
            todos: todos,
            filter: filter,
            onToggle: handleToggle,
            onDelete: handleDelete,
            onEdit: handleEdit
        }),
        h(Footer, {
            stats: stats,
            onClearCompleted: handleClearCompleted
        })
    );
}

// 初始化应用
function init() {
    console.log('Initializing Preact Todo App...');

    // 确保 document.head 存在
    if (!document.head) {
        const head = document.createElement('head');
        if (document.documentElement) {
            document.documentElement.insertBefore(head, document.documentElement.firstChild);
        } else {
            document.appendChild(head);
        }
    }

    // 确保 document.body 存在
    if (!document.body) {
        const body = document.createElement('body');
        if (document.documentElement) {
            document.documentElement.appendChild(body);
        } else {
            document.appendChild(body);
        }
    }

    // 注入样式
    const styleEl = document.createElement('style');
    styleEl.textContent = styles;
    document.head.appendChild(styleEl);

    // 创建根容器
    const root = document.createElement('div');
    root.id = 'root';
    document.body.appendChild(root);

    // 渲染应用
    render(h(App), root);

    console.log('✅ Preact Todo App initialized');
}

// 启动应用
init();

