/**
 * 简单 CSS 动画测试
 * 
 * 使用方法: esm_loader.exe test_animation_simple.js
 */

(function() {
    'use strict';

    var h = Preact.h;
    var render = Preact.render;

    console.log('=== 简单 CSS 动画测试 ===');

    // 添加 CSS 样式到 body
    var style = document.createElement('style');
    style.textContent = [
        '@keyframes fadeIn {',
        '    from { opacity: 0; }',
        '    to { opacity: 1; }',
        '}',
        '@keyframes slideRight {',
        '    from { transform: translateX(0px); }',
        '    to { transform: translateX(200px); }',
        '}',
        '@keyframes rotate {',
        '    from { transform: rotate(0deg); }',
        '    to { transform: rotate(360deg); }',
        '}',
        '@keyframes scale {',
        '    0% { transform: scale(1); }',
        '    50% { transform: scale(1.5); }',
        '    100% { transform: scale(1); }',
        '}',
        '.box {',
        '    width: 100px;',
        '    height: 100px;',
        '    margin: 30px;',
        '    display: inline-block;',
        '    border-radius: 10px;',
        '    text-align: center;',
        '    line-height: 100px;',
        '    color: white;',
        '    font-weight: bold;',
        '    font-size: 14px;',
        '}',
        '.fade {',
        '    background: #3498db;',
        '    animation: fadeIn 2s ease-in-out infinite alternate;',
        '}',
        '.slide {',
        '    background: #e74c3c;',
        '    animation: slideRight 2s ease-in-out infinite alternate;',
        '}',
        '.spin {',
        '    background: #2ecc71;',
        '    animation: rotate 3s linear infinite;',
        '}',
        '.pulse {',
        '    background: #9b59b6;',
        '    animation: scale 1.5s ease-in-out infinite;',
        '}'
    ].join('\n');
    
    // 尝试添加到 head，如果不存在则添加到 body
    if (document.head) {
        document.head.appendChild(style);
    } else {
        document.body.appendChild(style);
    }
    console.log('样式已添加');

    function App() {
        return h('div', { style: { padding: '20px' } },
            h('h1', { style: { color: '#2c3e50', marginBottom: '20px' } }, 'CSS Animation Demo'),
            h('div', { className: 'box fade' }, 'Fade'),
            h('div', { className: 'box slide' }, 'Slide'),
            h('div', { className: 'box spin' }, 'Spin'),
            h('div', { className: 'box pulse' }, 'Pulse')
        );
    }

    console.log('渲染中...');
    render(h(App), document.body);
    console.log('完成');
})();
