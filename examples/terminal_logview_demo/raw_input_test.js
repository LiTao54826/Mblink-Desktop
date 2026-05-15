import { h, render } from 'preact';
import { useState } from 'preact/hooks';

/**
 * 原始 input 元素测试
 * 
 * 测试 IIFE 风格下原始 input 元素是否能正常工作
 * 使用方法: esm_loader.exe examples/terminal_logview_demo/raw_input_test.js
 */

(function() {
    'use strict';

    console.log('=== 原始 Input 元素测试 ===');

    // 样式
    var style = document.createElement('style');
    style.textContent = `
        .container {
            padding: 20px;
            font-family: 'Consolas', monospace;
            background: #1e1e1e;
            color: #d4d4d4;
            min-height: 100vh;
        }
        .section {
            margin: 20px 0;
            padding: 15px;
            background: #2d2d2d;
            border-radius: 8px;
        }
        .title {
            font-size: 18px;
            color: #569cd6;
            margin-bottom: 10px;
        }
        .input-field {
            width: 300px;
            padding: 8px 12px;
            font-size: 14px;
            background: #3c3c3c;
            border: 1px solid #555;
            border-radius: 4px;
            color: #d4d4d4;
            outline: none;
            margin-right: 10px;
        }
        .input-field:focus {
            border-color: #007acc;
        }
        .log {
            font-size: 12px;
            color: #4fc1ff;
            margin: 5px 0;
        }
        .value-display {
            margin-top: 10px;
            padding: 10px;
            background: #1e1e1e;
            border-radius: 4px;
            font-size: 13px;
        }
    `;
    document.head.appendChild(style);

    function App() {
        var state1 = useState('');
        var value1 = state1[0];
        var setValue1 = state1[1];

        var state2 = useState('');
        var value2 = state2[0];
        var setValue2 = state2[1];

        var logsState = useState([]);
        var logs = logsState[0];
        var setLogs = logsState[1];

        function addLog(msg) {
            setLogs(function(prev) {
                return prev.concat([{ msg: msg, time: new Date().toLocaleTimeString() }]);
            });
        }

        return h('div', { className: 'container' },
            h('div', { className: 'title' }, '🧪 原始 Input 元素测试'),
            
            // 测试 1: onInput 事件
            h('div', { className: 'section' },
                h('div', { className: 'title' }, '测试 1: onInput 事件'),
                h('input', {
                    className: 'input-field',
                    type: 'text',
                    placeholder: '输入文字测试 onInput...',
                    value: value1,
                    onInput: function(e) {
                        console.log('onInput triggered:', e.target.value);
                        setValue1(e.target.value);
                        addLog('onInput: ' + e.target.value);
                    }
                }),
                h('div', { className: 'value-display' }, '当前值: "' + value1 + '"')
            ),

            // 测试 2: onKeyDown 事件 (回车)
            h('div', { className: 'section' },
                h('div', { className: 'title' }, '测试 2: onKeyDown 事件 (按回车)'),
                h('input', {
                    className: 'input-field',
                    type: 'text',
                    placeholder: '输入后按回车...',
                    value: value2,
                    onInput: function(e) {
                        setValue2(e.target.value);
                    },
                    onKeyDown: function(e) {
                        console.log('onKeyDown:', e.key);
                        if (e.key === 'Enter') {
                            addLog('回车! 值: ' + value2);
                            setValue2('');
                        }
                    }
                }),
                h('div', { className: 'value-display' }, '当前值: "' + value2 + '"')
            ),

            // 事件日志
            h('div', { className: 'section' },
                h('div', { className: 'title' }, '事件日志'),
                logs.length === 0 
                    ? h('div', { className: 'log' }, '(暂无事件)')
                    : logs.slice(-10).map(function(log, i) {
                        return h('div', { key: i, className: 'log' }, 
                            '[' + log.time + '] ' + log.msg
                        );
                    })
            )
        );
    }

    console.log('开始渲染...');
    render(h(App), document.body);
    console.log('渲染完成');

})();
