/**
 * @file test_nested_structure.js
 * @brief 测试嵌套结构是否影响文本显示
 */

(function () {
    'use strict';

    var h = Preact.h;
    var render = Preact.render;

    function App() {
        return h(
            'div',
            { style: { padding: '20px' } },
            h('h1', null, '嵌套结构测试'),
            
            // 测试1: 简单结构 - div > text
            h('h2', null, '1. 简单结构 (div > text):'),
            h('div', { style: { fontSize: '24px', color: 'blue', backgroundColor: '#e0e0e0', padding: '10px' } }, 
                '简单文本'
            ),
            
            // 测试2: 嵌套结构 - div > div > text
            h('h2', null, '2. 嵌套结构 (div > div > text):'),
            h('div', { style: { backgroundColor: '#f0f0f0', padding: '10px' } },
                h('div', { style: { fontSize: '24px', color: 'green' } }, '嵌套文本')
            ),
            
            // 测试3: 兄弟结构 - div > (h3 + div)
            h('h2', null, '3. 兄弟结构 (div > h3 + div):'),
            h('div', { style: { backgroundColor: '#d0d0d0', padding: '10px' } },
                h('h3', null, '标题'),
                h('div', { style: { fontSize: '24px', color: 'red' } }, '兄弟文本')
            ),
            
            // 测试4: 完全模拟 app.js 的 Clock 结构
            h('h2', null, '4. Clock 结构 (完全模拟):'),
            h('div', { style: { backgroundColor: '#f5f5f5', borderRadius: '8px', padding: '16px' } },
                h('h3', null, '实时时钟'),
                h('div', { 
                    style: { 
                        fontSize: '32px', 
                        textAlign: 'center', 
                        fontFamily: 'monospace', 
                        color: '#28a745' 
                    } 
                }, '12:34:56')
            )
        );
    }

    console.log('Starting nested structure test...');
    render(h(App, null), document.body);
    console.log('Nested structure test rendered!');
})();
