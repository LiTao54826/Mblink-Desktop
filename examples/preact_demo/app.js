/**
 * @file app.js
 * @brief Preact 桌面应用示例
 *
 * 使用方法: app_loader.exe examples/preact_demo/app.js
 *
 * 功能演示:
 * - Preact 组件和 JSX (使用 h 函数)
 * - useState 状态管理
 * - useEffect 副作用
 * - 事件处理
 * - 样式设置
 * - 列表渲染
 */

(function () {
    'use strict';

    // 在闭包内部获取 Preact API，避免全局变量污染
    var h = Preact.h;
    var render = Preact.render;
    var useState = PreactHooks.useState;
    var useEffect = PreactHooks.useEffect;

    // ========== 样式定义 ==========
    var styles = {
        app: {
            fontFamily: 'Arial, sans-serif',
            padding: '20px',
            maxWidth: '600px',
            margin: '0 auto'
        },
        header: {
            textAlign: 'center',
            color: '#333',
            marginBottom: '20px'
        },
        card: {
            backgroundColor: '#f5f5f5',
            borderRadius: '8px',
            padding: '16px',
            marginBottom: '16px',
            boxShadow: '0 2px 4px rgba(0,0,0,0.1)'
        },
        button: {
            backgroundColor: '#007bff',
            color: 'white',
            border: 'none',
            padding: '10px 20px',
            borderRadius: '4px',
            cursor: 'pointer',
            marginRight: '8px',
            marginBottom: '8px'
        },
        buttonDanger: {
            backgroundColor: '#dc3545',
            color: 'white',
            border: 'none',
            padding: '10px 20px',
            borderRadius: '4px',
            cursor: 'pointer',
            marginRight: '8px'
        },
        input: {
            padding: '8px 12px',
            border: '1px solid #ddd',
            borderRadius: '4px',
            marginRight: '8px',
            width: '200px'
        },
        todoItem: {
            display: 'flex',
            alignItems: 'center',
            padding: '8px',
            backgroundColor: 'white',
            borderRadius: '4px',
            marginBottom: '8px'
        },
        counter: {
            fontSize: '48px',
            textAlign: 'center',
            color: '#007bff',
            margin: '20px 0'
        }
    };

    // ========== 工具函数 ==========
    function formatTime(date) {
        var hours = date.getHours();
        var minutes = date.getMinutes();
        var seconds = date.getSeconds();
        var hh = hours < 10 ? '0' + hours : '' + hours;
        var mm = minutes < 10 ? '0' + minutes : '' + minutes;
        var ss = seconds < 10 ? '0' + seconds : '' + seconds;
        return hh + ':' + mm + ':' + ss;
    }

    // ========== 时钟组件 ==========
    function Clock() {
        var timeState = useState(formatTime(new Date()));
        var time = timeState[0];
        var setTime = timeState[1];

        useEffect(function () {
            var timer = setInterval(function () {
                setTime(formatTime(new Date()));
            }, 1000);
            return function () {
                clearInterval(timer);
            };
        }, []);

        return h(
            'div',
            { style: styles.card },
            h('h3', null, '实时时钟'),
            h('div', {
                style: {
                    fontSize: '32px',
                    textAlign: 'center',
                    fontFamily: 'monospace',
                    color: '#28a745'
                },
                textContent: time
            })
        );
    }

    // ========== 计数器组件 ==========
    function Counter() {
        var countState = useState(0);
        var count = countState[0];
        var setCount = countState[1];

        return h(
            'div',
            { style: styles.card },
            h('h3', null, '计数器'),
            h('div', { style: styles.counter }, count),
            h(
                'div',
                { style: { textAlign: 'center' } },
                h(
                    'button',
                    {
                        style: styles.button,
                        onClick: function () {
                            setCount(count - 1);
                        }
                    },
                    '减少'
                ),
                h(
                    'button',
                    {
                        style: styles.button,
                        onClick: function () {
                            setCount(count + 1);
                        }
                    },
                    '增加'
                ),
                h(
                    'button',
                    {
                        style: styles.buttonDanger,
                        onClick: function () {
                            setCount(0);
                        }
                    },
                    '重置'
                )
            )
        );
    }

    // ========== 待办事项组件 ==========
    function TodoList() {
        var todosState = useState([
            { id: 1, text: '学习 Preact', done: false },
            { id: 2, text: '构建桌面应用', done: false },
            { id: 3, text: '享受编程', done: true }
        ]);
        var todos = todosState[0];
        var setTodos = todosState[1];

        var inputState = useState('');
        var inputValue = inputState[0];
        var setInputValue = inputState[1];

        var nextIdState = useState(4);
        var nextId = nextIdState[0];
        var setNextId = nextIdState[1];

        function addTodo() {
            if (inputValue.trim()) {
                setTodos(
                    todos.concat([
                        {
                            id: nextId,
                            text: inputValue.trim(),
                            done: false
                        }
                    ])
                );
                setNextId(nextId + 1);
                setInputValue('');
            }
        }

        function toggleTodo(id) {
            setTodos(
                todos.map(function (todo) {
                    if (todo.id === id) {
                        return { id: todo.id, text: todo.text, done: !todo.done };
                    }
                    return todo;
                })
            );
        }

        function deleteTodo(id) {
            setTodos(
                todos.filter(function (todo) {
                    return todo.id !== id;
                })
            );
        }

        return h(
            'div',
            { style: styles.card },
            h('h3', null, '待办事项'),
            h(
                'div',
                { style: { marginBottom: '12px' } },
                h('input', {
                    type: 'text',
                    style: styles.input,
                    placeholder: '添加新任务...',
                    value: inputValue,
                    onInput: function (e) {
                        setInputValue(e.target.value);
                    }
                }),
                h(
                    'button',
                    {
                        style: styles.button,
                        onClick: addTodo
                    },
                    '添加'
                )
            ),
            h(
                'div',
                null,
                todos.map(function (todo) {
                    return h(
                        'div',
                        { key: todo.id, style: styles.todoItem },
                        h('input', {
                            type: 'checkbox',
                            checked: todo.done,
                            onChange: function () {
                                toggleTodo(todo.id);
                            }
                        }),
                        h(
                            'span',
                            {
                                style: {
                                    flex: '1',
                                    marginLeft: '8px',
                                    textDecoration: todo.done ? 'line-through' : 'none',
                                    color: todo.done ? '#999' : '#333'
                                }
                            },
                            todo.text
                        ),
                        h(
                            'button',
                            {
                                style: {
                                    backgroundColor: '#dc3545',
                                    color: 'white',
                                    border: 'none',
                                    padding: '4px 8px',
                                    borderRadius: '4px',
                                    cursor: 'pointer'
                                },
                                onClick: function () {
                                    deleteTodo(todo.id);
                                }
                            },
                            '删除'
                        )
                    );
                })
            ),
            h(
                'div',
                { style: { marginTop: '12px', color: '#666' } },
                '完成: ',
                todos.filter(function (t) {
                    return t.done;
                }).length,
                ' / ',
                todos.length
            )
        );
    }

    // ========== 颜色选择器组件 ==========
    function ColorPicker() {
        var colorState = useState('#007bff');
        var color = colorState[0];
        var setColor = colorState[1];

        var colors = ['#007bff', '#28a745', '#dc3545', '#ffc107', '#17a2b8', '#6f42c1'];

        return h(
            'div',
            { style: styles.card },
            h('h3', null, '颜色选择器'),
            h('div', {
                style: {
                    width: '100%',
                    height: '60px',
                    backgroundColor: color,
                    borderRadius: '4px',
                    marginBottom: '12px'
                }
            }),
            h(
                'div',
                { style: { display: 'flex', flexWrap: 'wrap' } },
                colors.map(function (c) {
                    return h('button', {
                        key: c,
                        style: {
                            width: '40px',
                            height: '40px',
                            backgroundColor: c,
                            border: color === c ? '3px solid #333' : '1px solid #ddd',
                            borderRadius: '4px',
                            cursor: 'pointer',
                            marginRight: '8px',
                            marginBottom: '8px'
                        },
                        onClick: function () {
                            setColor(c);
                        }
                    });
                })
            ),
            h('div', { style: { marginTop: '8px', color: '#666' } }, '当前颜色: ', color)
        );
    }

    // ========== 主应用组件 ==========
    function App() {
        return h(
            'div',
            { style: styles.app },
            h('h1', { style: styles.header }, '🚀 Preact 桌面应用示例'),
            h(
                'p',
                { style: { textAlign: 'center', color: '#666', marginBottom: '24px' } },
                '使用 Preact + MBink 构建的原生桌面应用'
            ),
            h(Clock, null),
            h(Counter, null),
            h(TodoList, null),
            h(ColorPicker, null),
            h(
                'div',
                {
                    style: {
                        textAlign: 'center',
                        marginTop: '20px',
                        padding: '12px',
                        backgroundColor: '#e9ecef',
                        borderRadius: '4px',
                        color: '#666'
                    }
                },
                '© 2024 MBink Preact Demo'
            )
        );
    }

    // ========== 渲染应用 ==========
    console.log('Starting Preact Demo App...');
    render(h(App, null), document.body);
    console.log('Preact Demo App rendered!');
})();
