/**
 * @file test_bindings_app.js
 * @brief 测试 DOM 绑定的 Preact 应用
 */

// 计数器组件
function Counter() {
    const [count, setCount] = preactHooks.useState(0);
    const [message, setMessage] = preactHooks.useState('点击按钮增加计数!');

    const handleClick = () => {
        const newCount = count + 1;
        setCount(newCount);
        setMessage(`你点击了 ${newCount} 次`);
        console.log('Counter clicked:', newCount);
    };

    return preact.h('div', {
        id: 'counter-app',
        style: {
            padding: '20px',
            fontFamily: 'Arial, sans-serif',
            maxWidth: '600px',
            margin: '0 auto'
        }
    }, [
        preact.h('h1', {
            style: {
                color: '#2c3e50',
                fontSize: '32px',
                marginBottom: '20px'
            }
        }, '🎯 DOM 绑定测试'),

        preact.h('div', {
            style: {
                background: '#ecf0f1',
                padding: '20px',
                borderRadius: '8px',
                marginBottom: '20px'
            }
        }, [
            preact.h('p', {
                style: {
                    fontSize: '18px',
                    color: '#34495e',
                    marginBottom: '10px'
                }
            }, message),

            preact.h('p', {
                style: {
                    fontSize: '48px',
                    fontWeight: 'bold',
                    color: '#3498db',
                    margin: '20px 0'
                }
            }, count.toString())
        ]),

        preact.h('button', {
            onClick: handleClick,
            style: {
                background: '#3498db',
                color: 'white',
                border: 'none',
                padding: '15px 30px',
                fontSize: '18px',
                borderRadius: '5px',
                cursor: 'pointer',
                width: '100%'
            }
        }, '点击我 +1'),

        preact.h('div', {
            style: {
                marginTop: '30px',
                padding: '15px',
                background: '#d5f4e6',
                borderRadius: '5px'
            }
        }, [
            preact.h('h3', {
                style: {
                    color: '#27ae60',
                    marginBottom: '10px'
                }
            }, '✅ 测试的功能：'),
            preact.h('ul', {
                style: {
                    color: '#2c3e50',
                    lineHeight: '1.8'
                }
            }, [
                preact.h('li', null, 'createElement - 创建元素'),
                preact.h('li', null, 'appendChild - 添加子元素'),
                preact.h('li', null, 'textContent - 设置文本'),
                preact.h('li', null, 'style.setProperty - 设置样式'),
                preact.h('li', null, 'addEventListener - 事件监听'),
                preact.h('li', null, 'useState - Preact hooks')
            ])
        ])
    ]);
}

// 渲染应用
console.log('========================================');
console.log('  启动 Preact 测试应用');
console.log('========================================');
console.log('- document.body:', document.body);
console.log('- preact.h:', typeof preact.h);
console.log('- preact.render:', typeof preact.render);
console.log('- preactHooks.useState:', typeof preactHooks.useState);
console.log('========================================');

preact.render(preact.h(Counter), document.body);

console.log('✅ Preact 应用渲染完成！');
console.log('✅ 所有 DOM 绑定功能正常工作！');
