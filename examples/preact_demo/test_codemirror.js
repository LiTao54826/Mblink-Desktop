/**
 * @file test_codemirror.js
 * @brief CodeMirror 样式测试 - 使用 Preact
 * 
 * 使用方法: esm_loader.exe examples/preact_demo/test_codemirror.js
 * 
 * 测试 getComputedStyle 和动态创建元素的样式应用
 */

(function () {
    'use strict';

    var h = Preact.h;
    var render = Preact.render;
    var useState = PreactHooks.useState;
    var useEffect = PreactHooks.useEffect;
    var useRef = PreactHooks.useRef;

    // 全局样式
    var globalStyles = `
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            margin: 0;
            padding: 20px;
            background: #1e1e1e;
            color: #d4d4d4;
        }
        
        .container {
            max-width: 900px;
            margin: 0 auto;
        }
        
        h1 {
            color: #569cd6;
            margin-bottom: 20px;
        }
        
        .test-section {
            background: #252526;
            border: 1px solid #3c3c3c;
            border-radius: 4px;
            padding: 15px;
            margin-bottom: 20px;
        }
        
        .test-title {
            color: #4ec9b0;
            font-size: 16px;
            margin-bottom: 10px;
        }
        
        .log-area {
            background: #1e1e1e;
            border: 1px solid #3c3c3c;
            padding: 10px;
            font-family: 'Consolas', 'Monaco', monospace;
            font-size: 12px;
            max-height: 200px;
            overflow-y: auto;
        }
        
        .log-item {
            margin: 2px 0;
        }
        
        .log-success { color: #4ec9b0; }
        .log-error { color: #f14c4c; }
        .log-info { color: #569cd6; }
        
        .editor-container {
            border: 1px solid #569cd6;
            min-height: 200px;
        }
        
        .styled-box {
            padding: 10px;
            margin: 5px 0;
        }
        
        .test-class-a {
            background-color: #264f78;
            color: #ffffff;
            padding: 8px;
            border-radius: 4px;
        }
        
        .test-class-b {
            background-color: #4ec9b0;
            color: #1e1e1e;
            font-weight: bold;
        }
    `;

    // 日志组件
    function LogArea(props) {
        var logs = props.logs || [];
        
        return h('div', { className: 'log-area' },
            logs.map(function(log, i) {
                return h('div', { 
                    key: i, 
                    className: 'log-item log-' + log.type 
                }, log.message);
            })
        );
    }

    // 测试 getComputedStyle
    function TestGetComputedStyle(props) {
        var _state = useState([]);
        var logs = _state[0];
        var setLogs = _state[1];
        var testRef = useRef(null);

        function addLog(type, message) {
            setLogs(function(prev) {
                return prev.concat([{ type: type, message: message }]);
            });
        }

        function runTest() {
            setLogs([]);
            
            // 测试 1: 获取已渲染元素的样式
            addLog('info', '=== 测试 getComputedStyle ===');
            
            if (testRef.current) {
                var style = window.getComputedStyle(testRef.current);
                addLog('info', 'backgroundColor: ' + style.backgroundColor);
                addLog('info', 'color: ' + style.color);
                addLog('info', 'padding: ' + style.padding);
                addLog('info', 'fontWeight: ' + style.fontWeight);
                
                if (style.backgroundColor) {
                    addLog('success', '✓ getComputedStyle 返回了样式值');
                } else {
                    addLog('error', '✗ getComputedStyle 返回空值');
                }
            }
            
            // 测试 2: 动态创建元素并获取样式
            addLog('info', '=== 测试动态创建元素 ===');
            
            var dynamicDiv = document.createElement('div');
            dynamicDiv.className = 'test-class-a';
            dynamicDiv.textContent = '动态创建的元素';
            document.body.appendChild(dynamicDiv);
            
            var dynamicStyle = window.getComputedStyle(dynamicDiv);
            addLog('info', '动态元素 backgroundColor: ' + dynamicStyle.backgroundColor);
            addLog('info', '动态元素 color: ' + dynamicStyle.color);
            
            if (dynamicStyle.backgroundColor && dynamicStyle.backgroundColor !== '') {
                addLog('success', '✓ 动态元素样式正确应用');
            } else {
                addLog('error', '✗ 动态元素样式未应用');
            }
            
            // 清理
            document.body.removeChild(dynamicDiv);
            
            // 测试 3: getPropertyValue 方法
            addLog('info', '=== 测试 getPropertyValue ===');
            if (testRef.current) {
                var style2 = window.getComputedStyle(testRef.current);
                var bgColor = style2.getPropertyValue('background-color');
                addLog('info', 'getPropertyValue("background-color"): ' + bgColor);
                
                if (typeof style2.getPropertyValue === 'function') {
                    addLog('success', '✓ getPropertyValue 方法存在');
                } else {
                    addLog('error', '✗ getPropertyValue 方法不存在');
                }
            }
        }

        useEffect(function() {
            // 延迟执行测试，确保元素已渲染
            setTimeout(runTest, 100);
        }, []);

        return h('div', { className: 'test-section' },
            h('div', { className: 'test-title' }, 'getComputedStyle 测试'),
            h('div', { 
                ref: testRef,
                className: 'test-class-b styled-box'
            }, '这是一个带样式的测试元素'),
            h('button', { 
                onClick: runTest,
                style: { marginTop: '10px', padding: '5px 10px' }
            }, '重新运行测试'),
            h(LogArea, { logs: logs })
        );
    }

    // 测试 querySelector
    function TestQuerySelector(props) {
        var _state = useState([]);
        var logs = _state[0];
        var setLogs = _state[1];

        function addLog(type, message) {
            setLogs(function(prev) {
                return prev.concat([{ type: type, message: message }]);
            });
        }

        function runTest() {
            setLogs([]);
            addLog('info', '=== 测试 querySelector ===');
            
            // 测试 document.querySelector
            var h1 = document.querySelector('h1');
            if (h1) {
                addLog('success', '✓ document.querySelector("h1") 找到元素');
                addLog('info', '  textContent: ' + h1.textContent);
            } else {
                addLog('error', '✗ document.querySelector("h1") 返回 null');
            }
            
            // 测试类选择器
            var testSection = document.querySelector('.test-section');
            if (testSection) {
                addLog('success', '✓ document.querySelector(".test-section") 找到元素');
            } else {
                addLog('error', '✗ document.querySelector(".test-section") 返回 null');
            }
            
            // 测试 querySelectorAll
            addLog('info', '=== 测试 querySelectorAll ===');
            var allSections = document.querySelectorAll('.test-section');
            addLog('info', '找到 ' + allSections.length + ' 个 .test-section 元素');
            
            if (allSections.length > 0) {
                addLog('success', '✓ querySelectorAll 正常工作');
            } else {
                addLog('error', '✗ querySelectorAll 返回空数组');
            }
        }

        useEffect(function() {
            setTimeout(runTest, 200);
        }, []);

        return h('div', { className: 'test-section' },
            h('div', { className: 'test-title' }, 'querySelector 测试'),
            h('button', { 
                onClick: runTest,
                style: { marginBottom: '10px', padding: '5px 10px' }
            }, '重新运行测试'),
            h(LogArea, { logs: logs })
        );
    }

    // 模拟 CodeMirror 样式的编辑器
    function MockCodeMirror(props) {
        var _state = useState([]);
        var logs = _state[0];
        var setLogs = _state[1];

        function addLog(type, message) {
            setLogs(function(prev) {
                return prev.concat([{ type: type, message: message }]);
            });
        }

        useEffect(function() {
            addLog('info', '=== 模拟 CodeMirror 结构 ===');
            
            // 动态创建类似 CodeMirror 的 DOM 结构
            var container = document.getElementById('mock-cm-container');
            if (!container) return;
            
            // 创建 CodeMirror 结构
            var cmWrapper = document.createElement('div');
            cmWrapper.className = 'CodeMirror';
            cmWrapper.style.cssText = 'height: 150px; background: #1e1e1e; color: #d4d4d4; font-family: monospace;';
            
            var cmScroll = document.createElement('div');
            cmScroll.className = 'CodeMirror-scroll';
            cmScroll.style.cssText = 'height: 100%; overflow: auto;';
            
            var cmCode = document.createElement('div');
            cmCode.className = 'CodeMirror-code';
            cmCode.style.cssText = 'padding: 10px;';
            
            // 添加一些代码行
            var lines = [
                '// 这是模拟的 CodeMirror',
                'function hello() {',
                '    console.log("Hello!");',
                '}'
            ];
            
            lines.forEach(function(line, i) {
                var lineDiv = document.createElement('div');
                lineDiv.className = 'CodeMirror-line';
                lineDiv.textContent = line;
                cmCode.appendChild(lineDiv);
            });
            
            cmScroll.appendChild(cmCode);
            cmWrapper.appendChild(cmScroll);
            container.appendChild(cmWrapper);
            
            // 测试动态创建元素的样式
            var style = window.getComputedStyle(cmWrapper);
            addLog('info', 'CodeMirror wrapper:');
            addLog('info', '  height: ' + style.height);
            addLog('info', '  background: ' + style.background);
            addLog('info', '  fontFamily: ' + style.fontFamily);
            
            if (style.height && style.height !== '') {
                addLog('success', '✓ 动态创建的 CodeMirror 结构样式正确');
            } else {
                addLog('error', '✗ 样式未正确应用');
            }
        }, []);

        return h('div', { className: 'test-section' },
            h('div', { className: 'test-title' }, '模拟 CodeMirror 结构'),
            h('div', { 
                id: 'mock-cm-container',
                className: 'editor-container'
            }),
            h(LogArea, { logs: logs })
        );
    }

    // 主应用
    function App() {
        return h('div', { className: 'container' },
            h('style', null, globalStyles),
            h('h1', null, '🔧 getComputedStyle 测试'),
            h('p', null, '测试动态创建元素的样式计算'),
            h(TestGetComputedStyle),
            h(TestQuerySelector),
            h(MockCodeMirror)
        );
    }

    console.log('Starting getComputedStyle Test...');
    render(h(App, null), document.body);
    console.log('Test rendered!');
})();
