/**
 * 简单 CSS 动画测试
 * 直接在 CSS 中定义动画，不通过 JS 动态设置
 */

(function() {
    'use strict';

    var h = Preact.h;
    var render = Preact.render;

    console.log('=== 简单 CSS 动画测试 ===');

    // 添加 CSS 样式
    console.log('Creating style element...');
    var style = document.createElement('style');
    console.log('Style element created, tag:', style.tagName);
    
    var cssText = [
        '@keyframes rotate360 {',
        '    from { transform: rotate(0deg); }',
        '    to { transform: rotate(360deg); }',
        '}',
        '',
        '.container {',
        '    padding: 20px;',
        '    font-family: sans-serif;',
        '}',
        '',
        '.box {',
        '    width: 100px;',
        '    height: 100px;',
        '    background: #3498db;',
        '    margin: 20px;',
        '    display: inline-block;',
        '    animation: rotate360 2s linear infinite;',
        '}'
    ].join('\n');
    
    console.log('Setting textContent, length:', cssText.length);
    style.textContent = cssText;
    console.log('textContent set, actual length:', style.textContent.length);
    
    console.log('Appending to body...');
    document.body.appendChild(style);
    console.log('Style element appended');

    function App() {
        return h('div', { className: 'container' },
            h('h1', null, '简单 CSS 动画测试'),
            h('p', null, '蓝色方块应该在旋转'),
            h('div', { className: 'box' })
        );
    }

    console.log('开始渲染...');
    render(h(App), document.body);
    console.log('渲染完成');
})();
