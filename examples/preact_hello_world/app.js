/**
 * @file app.js
 * @brief Preact Hello World示例应用
 */

// 主应用组件
function App() {
    return Preact.h('div', { className: 'app' },
        Preact.h('div', { className: 'container' },
            Preact.h('h1', null, 'Hello from Preact!'),
            Preact.h('p', null, 'This is running on MBink - a lightweight desktop framework')
        ),
        Preact.h('hr'),
        Preact.h('p', null, 'MBink + Preact integration successful!')
    );
}

// 渲染应用
var appVNode = Preact.h(App);
Preact.render(appVNode, document.body);

