import { h, render } from 'preact';
import { useEffect, useRef, useState } from 'preact/hooks';

/**
 * Terminal 和 LogView 简单测试
 * 
 * 测试 JS 绑定是否正常工作
 */

(function() {
    'use strict';

    console.log('=== Terminal & LogView 简单测试 ===');

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
        .log {
            font-size: 12px;
            color: #4fc1ff;
            margin: 5px 0;
        }
        .success { color: #4ec9b0; }
        .error { color: #f14c4c; }
        .btn {
            padding: 8px 16px;
            margin: 5px;
            border: none;
            border-radius: 4px;
            cursor: pointer;
            background: #0e639c;
            color: white;
        }
    `;
    document.head.appendChild(style);

    function App() {
        var logsState = useState([]);
        var logs = logsState[0];
        var setLogs = logsState[1];
        var terminalRef = useRef(null);
        var logviewRef = useRef(null);

        function addLog(msg, type) {
            setLogs(function(prev) {
                return prev.concat([{ msg: msg, type: type || 'log', time: new Date().toLocaleTimeString() }]);
            });
        }

        function testTerminal() {
            addLog('测试 Terminal 元素...');
            
            // 创建 terminal 元素
            var terminal = document.createElement('terminal');
            addLog('创建 terminal 元素: ' + (terminal ? '成功' : '失败'), terminal ? 'success' : 'error');
            
            // 检查 write 方法
            if (terminal && typeof terminal.write === 'function') {
                addLog('terminal.write 方法存在', 'success');
                try {
                    terminal.write('Hello Terminal!');
                    addLog('terminal.write() 调用成功', 'success');
                } catch (e) {
                    addLog('terminal.write() 调用失败: ' + e.message, 'error');
                }
            } else {
                addLog('terminal.write 方法不存在', 'error');
                addLog('terminal 对象类型: ' + typeof terminal);
                if (terminal) {
                    addLog('terminal 属性: ' + Object.keys(terminal).join(', '));
                }
            }

            // 检查其他方法
            var methods = ['clear', 'scrollTo', 'execute', 'focus'];
            methods.forEach(function(method) {
                if (terminal && typeof terminal[method] === 'function') {
                    addLog('terminal.' + method + ' 方法存在', 'success');
                } else {
                    addLog('terminal.' + method + ' 方法不存在', 'error');
                }
            });
        }

        function testLogView() {
            addLog('测试 LogView 元素...');
            
            // 创建 logview 元素
            var logview = document.createElement('logview');
            addLog('创建 logview 元素: ' + (logview ? '成功' : '失败'), logview ? 'success' : 'error');
            
            // 检查 append 方法
            if (logview && typeof logview.append === 'function') {
                addLog('logview.append 方法存在', 'success');
                try {
                    logview.append('INFO', 'test', 'Hello LogView!');
                    addLog('logview.append() 调用成功', 'success');
                } catch (e) {
                    addLog('logview.append() 调用失败: ' + e.message, 'error');
                }
            } else {
                addLog('logview.append 方法不存在', 'error');
                addLog('logview 对象类型: ' + typeof logview);
                if (logview) {
                    addLog('logview 属性: ' + Object.keys(logview).join(', '));
                }
            }

            // 检查其他方法
            var methods = ['clear', 'search', 'export', 'scrollTo'];
            methods.forEach(function(method) {
                if (logview && typeof logview[method] === 'function') {
                    addLog('logview.' + method + ' 方法存在', 'success');
                } else {
                    addLog('logview.' + method + ' 方法不存在', 'error');
                }
            });
        }

        function testRef() {
            addLog('测试 Preact ref...');
            
            if (terminalRef.current) {
                addLog('terminalRef.current 存在', 'success');
                if (typeof terminalRef.current.write === 'function') {
                    addLog('terminalRef.current.write 方法存在', 'success');
                } else {
                    addLog('terminalRef.current.write 方法不存在', 'error');
                }
            } else {
                addLog('terminalRef.current 为 null', 'error');
            }

            if (logviewRef.current) {
                addLog('logviewRef.current 存在', 'success');
                if (typeof logviewRef.current.append === 'function') {
                    addLog('logviewRef.current.append 方法存在', 'success');
                } else {
                    addLog('logviewRef.current.append 方法不存在', 'error');
                }
            } else {
                addLog('logviewRef.current 为 null', 'error');
            }
        }

        return h('div', { className: 'container' },
            h('div', { className: 'title' }, '🧪 Terminal & LogView 绑定测试'),
            
            h('div', { className: 'section' },
                h('div', { className: 'title' }, '测试按钮'),
                h('button', { className: 'btn', onClick: testTerminal }, '测试 Terminal'),
                h('button', { className: 'btn', onClick: testLogView }, '测试 LogView'),
                h('button', { className: 'btn', onClick: testRef }, '测试 Ref'),
                h('button', { className: 'btn', onClick: function() { setLogs([]); } }, '清除日志')
            ),

            h('div', { className: 'section' },
                h('div', { className: 'title' }, '隐藏的测试元素'),
                h('terminal', { ref: terminalRef, style: { display: 'none' } }),
                h('logview', { ref: logviewRef, style: { display: 'none' } })
            ),

            h('div', { className: 'section' },
                h('div', { className: 'title' }, '测试日志'),
                logs.map(function(log, i) {
                    return h('div', { 
                        key: i, 
                        className: 'log ' + log.type 
                    }, '[' + log.time + '] ' + log.msg);
                })
            )
        );
    }

    console.log('开始渲染...');
    render(h(App), document.body);
    console.log('渲染完成');

})();
