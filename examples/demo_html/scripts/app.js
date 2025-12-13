/**
 * 外部脚本示例
 * 使用 Preact 创建一个计数器应用
 */

// 从全局对象获取 Preact API
const { h, render } = preact;
const { useState } = preactHooks;

// 计数器组件
function Counter() {
    const [count, setCount] = useState(0);
    
    return h('div', { class: 'counter' },
        h('button', { onclick: () => setCount(count - 1) }, '−'),
        h('span', { class: 'counter-value' }, count),
        h('button', { onclick: () => setCount(count + 1) }, '+')
    );
}

// 主应用组件
function App() {
    return h('div', { class: 'container' },
        h('h1', null, '🎨 External Resources Demo'),
        h('p', null, '这个示例演示了从外部文件加载样式和脚本的功能。'),
        h(Counter),
        h('div', { class: 'info' },
            h('strong', null, '已加载的外部资源:'),
            h('ul', null,
                h('li', null, 
                    '✅ styles/demo.css',
                    h('span', { class: 'badge' }, 'CSS')
                ),
                h('li', null, 
                    '✅ scripts/app.js',
                    h('span', { class: 'badge' }, 'JS')
                )
            )
        )
    );
}

// 渲染应用
const appContainer = document.getElementById('app');
if (appContainer) {
    render(h(App), appContainer);
    console.log('External app rendered successfully!');
} else {
    console.error('App container not found!');
}

