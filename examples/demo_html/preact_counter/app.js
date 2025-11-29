/**
 * @file app.js
 * @brief Preact Counter示例 - 展示useState hook
 */

// Counter组件 - 使用useState hook
function Counter() {
    var state = PreactHooks.useState(0);
    var count = state[0];
    var setCount = state[1];
    
    return Preact.h('div', { className: 'counter' },
        Preact.h('h2', null, 'Counter Example'),
        Preact.h('p', null, 'Count: ' + count),
        Preact.h('button', {
            onclick: function() {
                setCount(function(prevCount) { return prevCount + 1; });
            }
        }, 'Increment'),
        Preact.h('button', {
            onclick: function() {
                setCount(function(prevCount) { return prevCount - 1; });
            }
        }, 'Decrement'),
        Preact.h('button', {
            onclick: function() {
                setCount(0);
            }
        }, 'Reset')
    );
}

// 主应用组件
function App() {
    return Preact.h('div', { className: 'app' },
        Preact.h('h1', null, 'MBink + Preact Hooks Demo'),
        Preact.h(Counter)
    );
}

// 渲染应用
var appVNode = Preact.h(App);
Preact.render(appVNode, document.body);

