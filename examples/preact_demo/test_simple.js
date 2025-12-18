/**
 * @file test_simple.js
 * @brief 简单的文本渲染测试
 */

(function () {
    'use strict';

    var h = Preact.h;
    var render = Preact.render;

    function App() {
        return h(
            'div',
            { style: { padding: '20px' } },
            h('h1', null, '文本渲染测试'),
            h('div', { 
                style: { 
                    fontSize: '32px', 
                    color: 'green',
                    padding: '20px',
                    backgroundColor: '#f0f0f0'
                } 
            }, '这是一段测试文本'),
            h('div', { 
                style: { 
                    fontSize: '48px', 
                    color: 'blue',
                    padding: '20px',
                    backgroundColor: '#e0e0e0'
                } 
            }, '12:34:56'),
            h('div', { 
                style: { 
                    fontSize: '24px', 
                    color: 'red',
                    padding: '20px',
                    backgroundColor: '#d0d0d0'
                } 
            }, 123456)
        );
    }

    console.log('Starting Simple Test...');
    render(h(App, null), document.body);
    console.log('Simple Test rendered!');
})();
