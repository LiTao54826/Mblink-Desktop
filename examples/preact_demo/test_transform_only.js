/**
 * 纯 Transform 动画测试
 * 
 * 使用方法: esm_loader.exe test_transform_only.js
 */

(function() {
    'use strict';

    var h = Preact.h;
    var render = Preact.render;

    console.log('=== 纯 Transform 动画测试 ===');

    // 添加 CSS 样式
    var style = document.createElement('style');
    style.textContent = `
        body {
            margin: 0;
            padding: 20px;
            background: #f0f0f0;
            font-family: sans-serif;
        }
        
        h1 { color: #333; }
        
        .container {
            display: flex;
            flex-wrap: wrap;
            gap: 20px;
            padding: 20px;
        }
        
        .box {
            width: 100px;
            height: 100px;
            background: #3498db;
            border-radius: 8px;
            will-change: transform;
        }
        
        @keyframes spin {
            from { transform: rotate(0deg); }
            to { transform: rotate(360deg); }
        }
        
        .spin {
            animation: spin 2s linear infinite;
        }
        
        .info {
            background: #fff;
            padding: 15px;
            border-radius: 8px;
            margin-bottom: 20px;
        }
    `;
    document.head.appendChild(style);

    function App() {
        return h('div', null,
            h('h1', null, '纯 Transform 动画测试'),
            
            h('div', { className: 'info' },
                h('p', null, h('strong', null, '测试说明：')),
                h('ul', null,
                    h('li', null, '只使用 transform: rotate() 动画'),
                    h('li', null, '所有元素都有 will-change: transform'),
                    h('li', null, '理论上 GPU 占用应该很低（< 10%）')
                )
            ),
            
            h('div', { className: 'container' },
                h('div', { className: 'box spin' }),
                h('div', { className: 'box spin' }),
                h('div', { className: 'box spin' }),
                h('div', { className: 'box spin' })
            )
        );
    }

    console.log('开始渲染...');
    render(h(App), document.body);
    console.log('渲染完成');
})();
