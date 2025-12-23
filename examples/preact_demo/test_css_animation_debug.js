/**
 * CSS 动画调试测试
 * 
 * 使用方法: esm_loader.exe test_css_animation_debug.js
 */

(function() {
    'use strict';

    var h = Preact.h;
    var render = Preact.render;

    console.log('=== CSS 动画调试测试 ===');

    // 检查 document.head 是否存在
    console.log('document.head:', document.head);
    console.log('document.body:', document.body);

    // 添加 CSS 样式
    var style = document.createElement('style');
    console.log('创建 style 元素:', style);
    
    var cssText = [
        '@keyframes fadeIn {',
        '    from { opacity: 0; }',
        '    to { opacity: 1; }',
        '}',
        '.animated-box {',
        '    width: 100px;',
        '    height: 100px;',
        '    background: #3498db;',
        '    animation: fadeIn 2s ease-in-out infinite alternate;',
        '}'
    ].join('\n');
    
    console.log('CSS 内容:', cssText);
    
    style.textContent = cssText;
    console.log('设置 textContent 后:', style.textContent);
    
    // 添加到 head
    if (document.head) {
        document.head.appendChild(style);
        console.log('样式已添加到 head');
    } else {
        document.body.appendChild(style);
        console.log('样式已添加到 body (head 不存在)');
    }

    function App() {
        return h('div', { style: { padding: '20px' } },
            h('h1', { style: { color: '#2c3e50', marginBottom: '20px' } }, 'CSS Animation Debug'),
            h('p', { style: { color: '#7f8c8d', marginBottom: '20px' } }, '下面的盒子应该有淡入淡出动画'),
            h('div', { className: 'animated-box' }, 'Fade')
        );
    }

    console.log('渲染中...');
    render(h(App), document.body);
    console.log('完成');
})();
