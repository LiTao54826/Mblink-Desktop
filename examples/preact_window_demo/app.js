/**
 * @file app.js
 * @brief Preact Window Demo - 带GUI窗口的Preact应用
 */

// 主应用组件 - 简化版（无样式）
function App() {
    return Preact.h('div', null,
        Preact.h('h1', null, 'Hello from Preact!'),
        Preact.h('h2', null, 'Welcome to MBink'),
        Preact.h('p', null, 'This is a lightweight desktop application framework'),
        Preact.h('p', null, 'Powered by Preact + QuickJS + Skia'),
        Preact.h('p', null, 'MBink + Preact integration successful!')
    );
}

// 渲染应用
var appVNode = Preact.h(App);
Preact.render(appVNode, document.body);

