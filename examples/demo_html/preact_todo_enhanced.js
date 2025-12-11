/**
 * @file preact_todo_enhanced.js
 * @brief 增强版 Preact Todo List 应用
 * 
 * 功能：
 * - 添加、删除、编辑任务
 * - 标记完成状态
 * - 过滤功能（全部/已完成/未完成）
 * - 任务统计
 */

console.log('🚀 启动增强版 Preact Todo List 应用...');

// ========== 工具函数 ==========

let nextId = 1;

function generateId() {
    return nextId++;
}

// ========== 子组件 ==========

/**
 * Todo 项组件
 */
function TodoItem(props) {
    const useState = preactHooks.useState;
    const [isEditing, setIsEditing] = useState(false);
    const [editText, setEditText] = useState(props.todo.text);

    function handleToggle() {
        props.onToggle(props.todo.id);
    }

    function handleDelete() {
        props.onDelete(props.todo.id);
    }

    function handleEdit() {
        setIsEditing(true);
        setEditText(props.todo.text);
    }

    function handleSave() {
        if (editText.trim()) {
            props.onEdit(props.todo.id, editText.trim());
            setIsEditing(false);
        }
    }

    function handleCancel() {
        setIsEditing(false);
        setEditText(props.todo.text);
    }

    function handleKeyDown(e) {
        if (e.key === 'Enter') {
            handleSave();
        } else if (e.key === 'Escape') {
            handleCancel();
        }
    }

    const itemStyle = 'display: flex; align-items: center; gap: 12px; padding: 15px; background: white; ' +
        'border-radius: 8px; margin-bottom: 10px; box-shadow: 0 1px 3px rgba(0,0,0,0.1); ' +
        (props.todo.completed ? 'opacity: 0.7;' : '');

    return preact.h('div', { style: itemStyle },
        // Checkbox
        preact.h('input', {
            type: 'checkbox',
            checked: props.todo.completed,
            onchange: handleToggle,
            style: 'width: 20px; height: 20px; cursor: pointer;'
        }),

        // 文本或编辑框
        isEditing
            ? preact.h('input', {
                type: 'text',
                value: editText,
                oninput: function (e) { setEditText(e.target.value); },
                onkeydown: handleKeyDown,
                style: 'flex: 1; padding: 8px; border: 2px solid #667eea; border-radius: 4px; font-size: 14px;',
                autofocus: true
            })
            : preact.h('span', {
                style: 'flex: 1; font-size: 16px; color: #333; ' +
                    (props.todo.completed ? 'text-decoration: line-through; color: #999;' : '')
            }, props.todo.text),

        // 按钮组
        isEditing
            ? preact.h('div', { style: 'display: flex; gap: 8px;' },
                preact.h('button', {
                    onclick: handleSave,
                    style: 'padding: 6px 12px; background: #27ae60; color: white; border: none; border-radius: 4px; cursor: pointer; font-size: 12px;'
                }, '✓ 保存'),
                preact.h('button', {
                    onclick: handleCancel,
                    style: 'padding: 6px 12px; background: #95a5a6; color: white; border: none; border-radius: 4px; cursor: pointer; font-size: 12px;'
                }, '✗ 取消')
            )
            : preact.h('div', { style: 'display: flex; gap: 8px;' },
                preact.h('button', {
                    onclick: handleEdit,
                    style: 'padding: 6px 12px; background: #3498db; color: white; border: none; border-radius: 4px; cursor: pointer; font-size: 12px;'
                }, '✎ 编辑'),
                preact.h('button', {
                    onclick: handleDelete,
                    style: 'padding: 6px 12px; background: #e74c3c; color: white; border: none; border-radius: 4px; cursor: pointer; font-size: 12px;'
                }, '✗ 删除')
            )
    );
}

/**
 * 添加 Todo 表单组件
 */
function AddTodoForm(props) {
    const useState = preactHooks.useState;
    const [text, setText] = useState('');

    function handleSubmit(e) {
        e.preventDefault();
        if (text.trim()) {
            props.onAdd(text.trim());
            setText('');
        }
    }

    return preact.h('form', {
        onsubmit: handleSubmit,
        style: 'display: flex; gap: 10px; margin-bottom: 20px;'
    },
        preact.h('input', {
            type: 'text',
            value: text,
            oninput: function (e) { setText(e.target.value); },
            placeholder: '输入新任务...',
            style: 'flex: 1; padding: 12px; border: 2px solid #ddd; border-radius: 8px; font-size: 16px;'
        }),
        preact.h('button', {
            type: 'submit',
            style: 'padding: 12px 24px; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); ' +
                'color: white; border: none; border-radius: 8px; cursor: pointer; font-size: 16px; font-weight: bold;'
        }, '➕ 添加')
    );
}

/**
 * 过滤器组件
 */
function TodoFilter(props) {
    const filters = [
        { value: 'all', label: '全部', emoji: '📋' },
        { value: 'active', label: '未完成', emoji: '⏳' },
        { value: 'completed', label: '已完成', emoji: '✅' }
    ];

    return preact.h('div', {
        style: 'display: flex; gap: 10px; margin-bottom: 20px;'
    },
        filters.map(function (filter) {
            const isActive = props.current === filter.value;
            return preact.h('button', {
                key: filter.value,
                onclick: function () { props.onChange(filter.value); },
                style: 'flex: 1; padding: 12px; border: 2px solid ' + (isActive ? '#667eea' : '#ddd') + '; ' +
                    'background: ' + (isActive ? '#667eea' : 'white') + '; ' +
                    'color: ' + (isActive ? 'white' : '#666') + '; ' +
                    'border-radius: 8px; cursor: pointer; font-size: 14px; font-weight: bold;'
            }, filter.emoji + ' ' + filter.label);
        })
    );
}

/**
 * 统计组件
 */
function TodoStats(props) {
    return preact.h('div', {
        style: 'background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white; ' +
            'padding: 20px; border-radius: 12px; margin-bottom: 20px;'
    },
        preact.h('div', {
            style: 'display: grid; grid-template-columns: repeat(3, 1fr); gap: 15px; text-align: center;'
        },
            preact.h('div', null,
                preact.h('div', { style: 'font-size: 32px; font-weight: bold; margin-bottom: 5px;' }, props.total),
                preact.h('div', { style: 'font-size: 12px; opacity: 0.9;' }, '总任务')
            ),
            preact.h('div', null,
                preact.h('div', { style: 'font-size: 32px; font-weight: bold; margin-bottom: 5px;' }, props.active),
                preact.h('div', { style: 'font-size: 12px; opacity: 0.9;' }, '未完成')
            ),
            preact.h('div', null,
                preact.h('div', { style: 'font-size: 32px; font-weight: bold; margin-bottom: 5px;' }, props.completed),
                preact.h('div', { style: 'font-size: 12px; opacity: 0.9;' }, '已完成')
            )
        )
    );
}

// ========== 主应用组件 ==========

function App() {
    const useState = preactHooks.useState;

    const [todos, setTodos] = useState([
        { id: generateId(), text: '学习 Preact 基础', completed: true },
        { id: generateId(), text: '创建 Todo List 应用', completed: false },
        { id: generateId(), text: '测试所有功能', completed: false }
    ]);

    const [filter, setFilter] = useState('all');

    // 添加任务
    function addTodo(text) {
        setTodos(function (prev) {
            return prev.concat([{
                id: generateId(),
                text: text,
                completed: false
            }]);
        });
    }

    // 切换完成状态
    function toggleTodo(id) {
        setTodos(function (prev) {
            return prev.map(function (todo) {
                if (todo.id === id) {
                    return { id: todo.id, text: todo.text, completed: !todo.completed };
                }
                return todo;
            });
        });
    }

    // 删除任务
    function deleteTodo(id) {
        setTodos(function (prev) {
            return prev.filter(function (todo) {
                return todo.id !== id;
            });
        });
    }

    // 编辑任务
    function editTodo(id, newText) {
        setTodos(function (prev) {
            return prev.map(function (todo) {
                if (todo.id === id) {
                    return { id: todo.id, text: newText, completed: todo.completed };
                }
                return todo;
            });
        });
    }

    // 过滤任务
    const filteredTodos = todos.filter(function (todo) {
        if (filter === 'active') return !todo.completed;
        if (filter === 'completed') return todo.completed;
        return true;
    });

    // 统计
    const stats = {
        total: todos.length,
        active: todos.filter(function (t) { return !t.completed; }).length,
        completed: todos.filter(function (t) { return t.completed; }).length
    };

    return preact.h('div', {
        style: 'min-height: 100vh; background: linear-gradient(to bottom, #e0e0e0, #f5f5f5); ' +
            'padding: 30px; font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;'
    },
        preact.h('div', {
            style: 'max-width: 800px; margin: 0 auto;'
        },
            // 头部
            preact.h('div', {
                style: 'text-align: center; margin-bottom: 30px;'
            },
                preact.h('h1', {
                    style: 'margin: 0 0 10px 0; font-size: 48px; color: #667eea;'
                }, '📝 Todo List'),
                preact.h('p', {
                    style: 'margin: 0; font-size: 16px; color: #666;'
                }, '增强版待办事项管理器')
            ),

            // 统计
            preact.h(TodoStats, stats),

            // 添加表单
            preact.h(AddTodoForm, { onAdd: addTodo }),

            // 过滤器
            preact.h(TodoFilter, { current: filter, onChange: setFilter }),

            // Todo 列表
            preact.h('div', {
                style: 'min-height: 200px;'
            },
                filteredTodos.length > 0
                    ? filteredTodos.map(function (todo) {
                        return preact.h(TodoItem, {
                            key: todo.id,
                            todo: todo,
                            onToggle: toggleTodo,
                            onDelete: deleteTodo,
                            onEdit: editTodo
                        });
                    })
                    : preact.h('div', {
                        style: 'text-align: center; padding: 60px 20px; color: #999; font-size: 18px;'
                    }, filter === 'completed' ? '还没有完成的任务 🎯' :
                        filter === 'active' ? '太棒了！所有任务都完成了！🎉' :
                            '暂无任务，添加一个开始吧！✨')
            ),

            // 页脚
            preact.h('div', {
                style: 'margin-top: 40px; padding: 20px; background: white; border-radius: 12px; text-align: center;'
            },
                preact.h('p', {
                    style: 'margin: 0 0 10px 0; color: #666; font-size: 14px;'
                }, '🚀 Powered by MBink + Preact + QuickJS'),
                preact.h('p', {
                    style: 'margin: 0; color: #999; font-size: 12px;'
                }, '功能：添加 • 编辑 • 删除 • 过滤 • 统计')
            )
        )
    );
}

// ========== 渲染应用 ==========

console.log('开始渲染 Todo List 应用...');
preact.render(preact.h(App), document.body);
console.log('✅ 应用渲染完成！');
