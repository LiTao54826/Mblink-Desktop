/**
 * 元素渲染测试 - 同时在浏览器和 MBink 中运行
 * 对比渲染效果，逐个修复差异
 *
 * 当前测试: input[checkbox], input[radio]
 */

var h = Preact.h;
var render = Preact.render;

// ========== 当前测试元素 ==========
function App() {
    return h('div', { style: 'padding: 20px; font-family: Arial, sans-serif;' },
        h('h1', null, 'Checkbox & Radio 测试'),

        // 1. input[type=checkbox]
        h('section', { style: 'margin: 20px 0; padding: 10px; border: 1px dashed #ccc;' },
            h('h2', null, '1. input[type=checkbox]'),
            h('input', { type: 'checkbox' }),
            h('span', null, ' Unchecked'),
            h('br'),
            h('input', { type: 'checkbox', checked: true }),
            h('span', null, ' Checked')
        ),

        // 2. input[type=radio]
        h('section', { style: 'margin: 20px 0; padding: 10px; border: 1px dashed #ccc;' },
            h('h2', null, '2. input[type=radio]'),
            h('input', { type: 'radio', name: 'test' }),
            h('span', null, ' Option A'),
            h('br'),
            h('input', { type: 'radio', name: 'test', checked: true }),
            h('span', null, ' Option B')
        )
    );
}

// 渲染
render(h(App), document.body);
console.log('Test app rendered!');

