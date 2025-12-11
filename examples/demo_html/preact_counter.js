/**
 * @file preact_counter.js
 * @brief Preact 计数器示例 - 测试组件、状态和 hooks
 */

console.log('🎯 Preact 计数器应用启动...');

// NOTE: 不能使用解构赋值，因为 Preact 已经暴露了全局变量
// 使用 preact.h 和 preactHooks.useState 来避免 redeclaration 错误

// 计数器组件
function Counter() {
    const [count, setCount] = preactHooks.useState(0);

    console.log('Counter 组件渲染, count =', count);

    return preact.h('div', {
        style: 'padding: 20px; font-family: Arial;'
    },
        preact.h('h1', { style: 'color: #2c3e50;' }, '🎯 Preact 计数器'),

        preact.h('div', {
            style: 'margin: 30px 0; padding: 20px; background: #ecf0f1; border-radius: 10px; text-align: center;'
        },
            preact.h('div', { style: 'font-size: 48px; font-weight: bold; color: #3498db;' }, count)
        ),

        preact.h('div', { style: 'display: flex; gap: 10px; justify-content: center;' },
            preact.h('button', {
                onclick: () => {
                    console.log('减少按钮点击');
                    setCount(count - 1);
                },
                style: 'padding: 10px 20px; font-size: 16px; cursor: pointer; background: #e74c3c; color: white; border: none; border-radius: 5px;'
            }, '➖ 减少'),

            preact.h('button', {
                onclick: () => {
                    console.log('重置按钮点击');
                    setCount(0);
                },
                style: 'padding: 10px 20px; font-size: 16px; cursor: pointer; background: #95a5a6; color: white; border: none; border-radius: 5px;'
            }, '🔄 重置'),

            preact.h('button', {
                onclick: () => {
                    console.log('增加按钮点击');
                    setCount(count + 1);
                },
                style: 'padding: 10px 20px; font-size: 16px; cursor: pointer; background: #27ae60; color: white; border: none; border-radius: 5px;'
            }, '➕ 增加')
        ),

        preact.h('div', { style: 'margin-top: 30px; padding: 15px; background: #fff3cd; border-radius: 5px;' },
            preact.h('h3', { style: 'margin: 0 0 10px 0; color: #856404;' }, '✅ 功能验证'),
            preact.h('ul', { style: 'margin: 0; padding-left: 20px; color: #856404;' },
                preact.h('li', null, '✓ Preact 组件渲染'),
                preact.h('li', null, '✓ useState Hook'),
                preact.h('li', null, '✓ 事件处理'),
                preact.h('li', null, '✓ 状态更新和重渲染')
            )
        )
    );
}

// 渲染到 document.body
console.log('开始渲染 Counter 组件到 document.body...');
preact.render(preact.h(Counter), document.body);

console.log('✅ Preact 计数器应用启动完成！');
console.log('点击按钮测试状态管理和重渲染...');
