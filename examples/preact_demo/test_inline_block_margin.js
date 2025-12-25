/**
 * 最简单的inline-block margin测试
 */

(function () {
    'use strict';

    var h = Preact.h;
    var render = Preact.render;

    // 添加 CSS 样式
    var style = document.createElement('style');
    style.textContent = `
        .demo-box {
            width: 100px;
            height: 100px;
            margin: 20px;
            display: inline-block;
            background: #3498db;
            color: white;
            text-align: center;
            line-height: 100px;
        }
    `;
    document.head.appendChild(style);

    // 主应用 - 只有inline-block元素
    function App() {
        return h('div', null,
            h('div', { className: 'demo-box' }, '1'),
            h('div', { className: 'demo-box' }, '2'),
            h('div', { className: 'demo-box' }, '3')
        );
    }

    console.log('开始渲染...');
    render(h(App), document.body);
    console.log('渲染完成');
})();
