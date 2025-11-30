/**
 * @file app.js
 * @brief Preact Todo App - Interactive todo list with useState
 *
 * Features:
 * - Add new todos
 * - Toggle todo completion status
 * - Delete todos
 * - Real-time statistics
 */

// ========== 子组件 ==========

// Header组件
function Header(props) {
    return Preact.h('div', {
        style: 'background-color: #2196F3; color: white; padding: 20px; margin-bottom: 20px;'
    },
        Preact.h('h1', {
            style: 'margin: 0; font-size: 28px;'
        }, props.title),
        Preact.h('p', {
            style: 'margin: 5px 0 0 0; font-size: 14px; opacity: 0.9;'
        }, props.subtitle)
    );
}

// AddTodo组件 - 添加新todo的输入框
function AddTodo(props) {
    var useState = PreactHooks.useState;
    var state = useState('');
    var inputValue = state[0];
    var setInputValue = state[1];

    var handleSubmit = function(e) {
        console.log('[AddTodo] handleSubmit called');
        console.log('[AddTodo] inputValue=' + inputValue);
        console.log('[AddTodo] inputValue.trim()=' + inputValue.trim());
        e.preventDefault();
        if (inputValue.trim()) {
            console.log('[AddTodo] Calling props.onAdd with: ' + inputValue);
            props.onAdd(inputValue);
            console.log('[AddTodo] Calling setInputValue with empty string');
            setInputValue('');
        } else {
            console.log('[AddTodo] inputValue is empty, not adding');
        }
    };

    var handleChange = function(e) {
        setInputValue(e.target.value);
    };

    var containerStyle = 'background-color: white; padding: 15px; margin-bottom: 20px; border-radius: 8px;';
    var formStyle = 'display: flex; gap: 10px;';
    var inputStyle = 'flex: 1; padding: 10px; font-size: 16px; border: 2px solid #ddd; border-radius: 4px;';
    var buttonStyle = 'padding: 10px 20px; font-size: 16px; background-color: #4CAF50; color: white; ' +
                      'border: none; border-radius: 4px; cursor: pointer;';

    return Preact.h('div', { style: containerStyle },
        Preact.h('form', { style: formStyle, onSubmit: handleSubmit },
            Preact.h('input', {
                type: 'text',
                style: inputStyle,
                placeholder: 'What needs to be done?',
                value: inputValue,
                onChange: handleChange
            }),
            Preact.h('button', { type: 'submit', style: buttonStyle }, '+ Add')
        )
    );
}

// TodoItem组件
function TodoItem(props) {
    var item = props.item;
    var index = props.index;

    var itemStyle = 'padding: 12px; margin-bottom: 8px; background-color: white; border-left: 4px solid ' +
                    (item.completed ? '#4CAF50' : '#FF9800') + '; display: flex; align-items: center; ' +
                    'justify-content: space-between;';

    var textStyle = 'margin: 0; font-size: 16px; flex: 1; ' +
                    (item.completed ? 'text-decoration: line-through; color: #999;' : 'color: #333;');

    var statusStyle = 'display: inline-block; padding: 2px 8px; margin-left: 10px; font-size: 12px; ' +
                      'background-color: ' + (item.completed ? '#4CAF50' : '#FF9800') + '; ' +
                      'color: white; border-radius: 3px;';

    var buttonContainerStyle = 'display: flex; gap: 5px;';

    var toggleButtonStyle = 'padding: 5px 10px; font-size: 12px; border: none; border-radius: 3px; ' +
                            'cursor: pointer; background-color: ' + (item.completed ? '#FF9800' : '#4CAF50') + '; ' +
                            'color: white;';

    var deleteButtonStyle = 'padding: 5px 10px; font-size: 12px; border: none; border-radius: 3px; ' +
                            'cursor: pointer; background-color: #f44336; color: white;';

    var handleToggle = function() {
        props.onToggle(item.id);
    };

    var handleDelete = function() {
        props.onDelete(item.id);
    };

    return Preact.h('div', { style: itemStyle },
        Preact.h('p', { style: textStyle },
            '#' + (index + 1) + ' - ' + item.text,
            Preact.h('span', { style: statusStyle },
                item.completed ? '✓ Done' : '⏳ Pending'
            )
        ),
        Preact.h('div', { style: buttonContainerStyle },
            Preact.h('button', {
                style: toggleButtonStyle,
                onClick: handleToggle
            }, item.completed ? 'Undo' : 'Done'),
            Preact.h('button', {
                style: deleteButtonStyle,
                onClick: handleDelete
            }, 'Delete')
        )
    );
}

// TodoList组件
function TodoList(props) {
    var items = props.items;

    var containerStyle = 'background-color: #f5f5f5; padding: 15px; border-radius: 8px;';

    var titleStyle = 'margin: 0 0 15px 0; font-size: 20px; color: #333; font-weight: bold;';

    var emptyStyle = 'text-align: center; padding: 40px; color: #999; font-size: 16px;';

    // 创建TodoItem数组
    var todoItems = [];
    for (var i = 0; i < items.length; i++) {
        todoItems.push(
            Preact.h(TodoItem, {
                item: items[i],
                index: i,
                key: 'todo-' + items[i].id,
                onToggle: props.onToggle,
                onDelete: props.onDelete
            })
        );
    }

    return Preact.h('div', { style: containerStyle },
        Preact.h('h2', { style: titleStyle },
            '📝 Todo List (' + items.length + ' items)'
        ),
        items.length === 0
            ? Preact.h('div', { style: emptyStyle }, '🎉 No todos! Add one above to get started.')
            : Preact.h('div', null, todoItems)
    );
}

// Stats组件 - 显示统计信息
function Stats(props) {
    var items = props.items;
    
    var total = items.length;
    var completed = 0;
    var pending = 0;
    
    for (var i = 0; i < items.length; i++) {
        if (items[i].completed) {
            completed++;
        } else {
            pending++;
        }
    }
    
    var containerStyle = 'display: flex; margin-top: 20px;';
    
    var cardStyle = 'flex: 1; padding: 15px; margin-right: 10px; border-radius: 8px; text-align: center;';
    
    var totalCardStyle = cardStyle + ' background-color: #2196F3; color: white;';
    var completedCardStyle = cardStyle + ' background-color: #4CAF50; color: white;';
    var pendingCardStyle = cardStyle + ' background-color: #FF9800; color: white; margin-right: 0;';
    
    var numberStyle = 'font-size: 32px; font-weight: bold; margin: 0;';
    var labelStyle = 'font-size: 14px; margin: 5px 0 0 0; opacity: 0.9;';
    
    return Preact.h('div', { style: containerStyle },
        Preact.h('div', { style: totalCardStyle },
            Preact.h('p', { style: numberStyle }, total.toString()),
            Preact.h('p', { style: labelStyle }, 'Total Tasks')
        ),
        Preact.h('div', { style: completedCardStyle },
            Preact.h('p', { style: numberStyle }, completed.toString()),
            Preact.h('p', { style: labelStyle }, 'Completed')
        ),
        Preact.h('div', { style: pendingCardStyle },
            Preact.h('p', { style: numberStyle }, pending.toString()),
            Preact.h('p', { style: labelStyle }, 'Pending')
        )
    );
}

// Footer组件
function Footer() {
    var footerStyle = 'margin-top: 30px; padding: 15px; background-color: #f5f5f5; ' +
                      'border-top: 2px solid #ddd; text-align: center;';
    
    var textStyle = 'margin: 0; font-size: 14px; color: #666;';
    
    return Preact.h('div', { style: footerStyle },
        Preact.h('p', { style: textStyle }, 
            '🚀 Powered by MBink + Preact + QuickJS + Skia'
        ),
        Preact.h('p', { style: textStyle }, 
            '✨ Lightweight Desktop Application Framework'
        )
    );
}

// ========== 主应用组件 ==========

function App() {
    var useState = PreactHooks.useState;

    // 初始数据
    var initialTodos = [
        { id: 1, text: 'Implement Preact integration', completed: true },
        { id: 2, text: 'Create Virtual DOM renderer', completed: true },
        { id: 3, text: 'Build component system', completed: true },
        { id: 4, text: 'Add event handling', completed: true },
        { id: 5, text: 'Implement useState hook', completed: true },
        { id: 6, text: 'Add useEffect hook', completed: false },
        { id: 7, text: 'Create interactive examples', completed: false }
    ];

    // 使用useState管理todos
    var state = useState(initialTodos);
    var todos = state[0];
    var setTodos = state[1];

    // 添加todo
    var handleAddTodo = function(text) {
        console.log('[App] handleAddTodo called with text=' + text);
        var newTodo = {
            id: Date.now(), // 使用时间戳作为ID
            text: text,
            completed: false
        };
        console.log('[App] Created newTodo with id=' + newTodo.id);
        setTodos(function(prevTodos) {
            console.log('[App] setTodos updater called, prevTodos.length=' + prevTodos.length);
            var result = prevTodos.concat([newTodo]);
            console.log('[App] Returning new todos with length=' + result.length);
            return result;
        });
    };

    // 切换todo完成状态
    var handleToggleTodo = function(id) {
        setTodos(function(prevTodos) {
            var newTodos = [];
            for (var i = 0; i < prevTodos.length; i++) {
                var todo = prevTodos[i];
                if (todo.id === id) {
                    newTodos.push({
                        id: todo.id,
                        text: todo.text,
                        completed: !todo.completed
                    });
                } else {
                    newTodos.push(todo);
                }
            }
            return newTodos;
        });
    };

    // 删除todo
    var handleDeleteTodo = function(id) {
        console.log('[App] handleDeleteTodo called with id=' + id);
        setTodos(function(prevTodos) {
            console.log('[App] setTodos updater called, prevTodos.length=' + prevTodos.length);
            var newTodos = [];
            for (var i = 0; i < prevTodos.length; i++) {
                if (prevTodos[i].id !== id) {
                    newTodos.push(prevTodos[i]);
                }
            }
            console.log('[App] newTodos.length=' + newTodos.length);
            return newTodos;
        });
    };

    var appStyle = 'padding: 0; margin: 0; background-color: #e0e0e0; min-height: 100vh;';

    var containerStyle = 'max-width: 800px; margin: 0 auto; padding: 20px;';

    return Preact.h('div', { style: appStyle },
        Preact.h('div', { style: containerStyle },
            Preact.h(Header, {
                title: 'MBink Todo App',
                subtitle: 'Interactive todo list with useState - Click buttons to interact!'
            }),
            Preact.h(Stats, { items: todos }),
            Preact.h('div', { style: 'height: 20px;' }), // Spacer
            Preact.h(AddTodo, { onAdd: handleAddTodo }),
            Preact.h(TodoList, {
                items: todos,
                onToggle: handleToggleTodo,
                onDelete: handleDeleteTodo
            }),
            Preact.h(Footer)
        )
    );
}

// ========== 渲染应用 ==========

console.log('Starting Todo App...');
var appVNode = Preact.h(App);
Preact.render(appVNode, document.body);
console.log('Todo App rendered!');

