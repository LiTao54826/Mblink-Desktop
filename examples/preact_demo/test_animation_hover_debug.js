/**
 * 动画 + Hover 调试测试
 * 
 * 测试鼠标移动是否会影响动画
 */

(function() {
    'use strict';

    var h = Preact.h;
    var render = Preact.render;

    console.log('=== 动画 Hover 调试测试 ===');

    // 添加 CSS 样式 - 注意：没有任何 :hover 规则
    var style = document.createElement('style');
    style.textContent = `
        @keyframes spin {
            from { transform: rotate(0deg); }
            to { transform: rotate(360deg); }
        }

        .container {
            padding: 40px;
            font-family: sans-serif;
        }

        .spin-box {
            width: 100px;
            height: 100px;
            background: #9b59b6;
            margin: 20px;
            animation: spin 2s linear infinite;
            display: inline-block;
        }

        .static-box {
            width: 100px;
            height: 100px;
            background: #3498db;
            margin: 20px;
            display: inline-block;
        }

        .info {
            margin-top: 20px;
            padding: 10px;
            background: #ecf0f1;
            border-radius: 4px;
        }
    `;
    document.body.appendChild(style);

    function App() {
        return h('div', { className: 'container' },
            h('h1', null, '动画 Hover 调试测试'),
            h('p', null, '在下面的方块上移动鼠标，观察旋转动画是否受影响'),
            h('div', null,
                h('div', { className: 'spin-box' }, 'Spin'),
                h('div', { className: 'static-box' }, 'Static')
            ),
            h('div', { className: 'info' },
                h('p', null, '如果鼠标移动导致旋转动画重置或闪烁，说明有问题'),
                h('p', null, '注意：这个测试没有任何 :hover 规则')
            )
        );
    }

    console.log('开始渲染...');
    render(h(App), document.body);
    console.log('渲染完成');
})();
