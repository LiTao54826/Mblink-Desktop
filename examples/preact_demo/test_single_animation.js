/**
 * 单个动画测试 - 用于测试 GPU 使用率
 */

(function() {
    'use strict';

    var h = Preact.h;
    var render = Preact.render;

    console.log('=== 单个动画测试 ===');

    // 添加 CSS 样式
    var style = document.createElement('style');
    style.textContent = `
        @keyframes spin {
            from { transform: rotate(0deg); }
            to { transform: rotate(360deg); }
        }

        .container {
            padding: 40px;
        }

        .spin-box {
            width: 100px;
            height: 100px;
            background: #9b59b6;
            animation: spin 2s linear infinite;
            border-radius: 8px;
        }
    `;
    document.body.appendChild(style);

    function App() {
        return h('div', { className: 'container' },
            h('h1', null, '单个动画测试'),
            h('p', null, '观察 GPU 使用率'),
            h('div', { className: 'spin-box' })
        );
    }

    render(h(App), document.body);
    console.log('完成');
})();
