/**
 * 平移动画测试
 * 
 * 使用方法: esm_loader.exe test_translate_animation.js
 */

(function() {
    'use strict';

    var h = Preact.h;
    var render = Preact.render;

    console.log('=== 平移动画测试 ===');

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
            gap: 40px;
            padding: 40px;
        }
        
        .box {
            width: 100px;
            height: 100px;
            border-radius: 8px;
            will-change: transform;
            display: flex;
            align-items: center;
            justify-content: center;
            color: white;
            font-weight: bold;
        }
        
        @keyframes bounce {
            0%, 100% { transform: translateY(0px); }
            50% { transform: translateY(-30px); }
        }
        
        @keyframes slideX {
            0% { transform: translateX(-50px); }
            100% { transform: translateX(50px); }
        }
        
        @keyframes spin {
            from { transform: rotate(0deg); }
            to { transform: rotate(360deg); }
        }
        
        @keyframes pulse {
            0%, 100% { transform: scale(1); }
            50% { transform: scale(1.2); }
        }
        
        .bounce {
            background: #2ecc71;
            animation: bounce 1s ease-in-out infinite;
        }
        
        .slideX {
            background: #e74c3c;
            animation: slideX 1s ease-in-out infinite alternate;
        }
        
        .spin {
            background: #9b59b6;
            animation: spin 2s linear infinite;
        }
        
        .pulse {
            background: #f39c12;
            animation: pulse 1s ease-in-out infinite;
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
            h('h1', null, '平移动画测试'),
            
            h('div', { className: 'info' },
                h('p', null, h('strong', null, '测试说明：')),
                h('ul', null,
                    h('li', null, 'Bounce: translateY(-30px) 上下弹跳'),
                    h('li', null, 'SlideX: translateX(-50px ~ 50px) 左右滑动'),
                    h('li', null, 'Spin: rotate(360deg) 旋转'),
                    h('li', null, 'Pulse: scale(1.2) 缩放')
                )
            ),
            
            h('div', { className: 'container' },
                h('div', { className: 'box bounce' }, 'Bounce'),
                h('div', { className: 'box slideX' }, 'SlideX'),
                h('div', { className: 'box spin' }, 'Spin'),
                h('div', { className: 'box pulse' }, 'Pulse')
            )
        );
    }

    console.log('开始渲染...');
    render(h(App), document.body);
    console.log('渲染完成');
})();
