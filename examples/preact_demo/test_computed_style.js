/**
 * @file test_computed_style.js
 * @brief 简单的 getComputedStyle 测试
 * 
 * 使用方法: esm_loader.exe examples/preact_demo/test_computed_style.js
 */

(function () {
    'use strict';

    var h = Preact.h;
    var render = Preact.render;

    function App() {
        return h('div', { style: { padding: '20px', background: '#1e1e1e', color: '#d4d4d4' } },
            h('h1', { style: { color: '#569cd6' } }, 'getComputedStyle 测试'),
            h('div', { 
                id: 'test-box',
                style: { 
                    backgroundColor: '#264f78',
                    color: '#ffffff',
                    padding: '15px',
                    margin: '10px 0',
                    borderRadius: '4px'
                }
            }, '这是测试元素'),
            h('div', { id: 'log', style: { fontFamily: 'monospace', fontSize: '14px' } })
        );
    }

    console.log('Rendering App...');
    render(h(App, null), document.body);
    console.log('App rendered!');

    // 延迟执行测试
    setTimeout(function() {
        console.log('Running getComputedStyle test...');
        
        var logDiv = document.getElementById('log');
        var testBox = document.getElementById('test-box');
        
        function log(msg) {
            console.log(msg);
            if (logDiv) {
                var p = document.createElement('div');
                p.textContent = msg;
                logDiv.appendChild(p);
            }
        }
        
        if (!testBox) {
            log('ERROR: test-box not found!');
            return;
        }
        
        log('=== getComputedStyle 测试 ===');
        
        var style = window.getComputedStyle(testBox);
        log('backgroundColor: ' + style.backgroundColor);
        log('color: ' + style.color);
        log('padding: ' + style.padding);
        log('paddingTop: ' + style.paddingTop);
        
        // 测试 getPropertyValue
        if (style.getPropertyValue) {
            var bgColor = style.getPropertyValue('background-color');
            log('getPropertyValue("background-color"): ' + bgColor);
        }
        
        // 测试动态创建元素
        log('');
        log('=== 动态创建元素测试 ===');
        
        var dynamicDiv = document.createElement('div');
        dynamicDiv.style.backgroundColor = 'red';
        dynamicDiv.style.padding = '10px';
        dynamicDiv.textContent = '动态创建';
        document.body.appendChild(dynamicDiv);
        
        var dynamicStyle = window.getComputedStyle(dynamicDiv);
        log('动态元素 backgroundColor: ' + dynamicStyle.backgroundColor);
        log('动态元素 padding: ' + dynamicStyle.padding);
        
    }, 500);
})();
