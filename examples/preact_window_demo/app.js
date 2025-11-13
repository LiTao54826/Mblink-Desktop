/**
 * @file app.js
 * @brief Preact Window Demo - 带GUI窗口的Preact应用
 */

// Counter 组件 - 使用 Hooks
function Counter() {
    var state = PreactHooks.useState(0);
    var count = state[0];
    var setCount = state[1];

    return Preact.h('div', { className: 'counter' },
        Preact.h('h2', null, 'Counter Example'),
        Preact.h('p', null, 'Count: ' + count),
        Preact.h('button', {
            onclick: function() {
                console.log('Increment clicked! Current count:', count);
                setCount(function(prevCount) { return prevCount + 1; });
            }
        }, 'Increment'),
        Preact.h('button', {
            onclick: function() {
                console.log('Decrement clicked! Current count:', count);
                setCount(function(prevCount) { return prevCount - 1; });
            }
        }, 'Decrement'),
        Preact.h('button', {
            onclick: function() {
                console.log('Reset clicked!');
                setCount(0);
            }
        }, 'Reset')
    );
}

// 主应用组件
function App() {
    return Preact.h('div', { className: 'app' },
        Preact.h('h1', null, 'MBink + Preact Window Demo'),
        Preact.h(Counter)
    );
}

// 渲染应用
var appVNode = Preact.h(App);
Preact.render(appVNode, document.body);

console.log('\n========================================');
console.log('Application rendered successfully!');
console.log('You can click the buttons to test interactivity.');
console.log('========================================\n');

