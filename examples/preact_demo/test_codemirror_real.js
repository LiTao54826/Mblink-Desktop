/**
 * @file test_codemirror_real.js
 * @brief 真实 CodeMirror 5 测试
 * 
 * 使用方法: esm_loader.exe examples/preact_demo/test_codemirror_real.js
 */

(function () {
    'use strict';

    // 使用 fetch 加载脚本内容并执行
    function loadScriptSync(url) {
        return new Promise(function(resolve, reject) {
            fetch(url)
                .then(function(response) {
                    if (!response.ok) {
                        throw new Error('Failed to load: ' + url);
                    }
                    return response.text();
                })
                .then(function(code) {
                    try {
                        // 使用 Function 构造函数执行代码
                        var fn = new Function(code);
                        fn();
                        console.log('Loaded: ' + url);
                        resolve();
                    } catch (e) {
                        console.error('Error executing ' + url + ': ' + e.message);
                        reject(e);
                    }
                })
                .catch(reject);
        });
    }

    // 加载 CSS
    function loadCSS(url) {
        var link = document.createElement('link');
        link.rel = 'stylesheet';
        link.href = url;
        document.head.appendChild(link);
    }

    // 添加基础样式
    var style = document.createElement('style');
    style.textContent = [
        'body {',
        '    font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;',
        '    margin: 0;',
        '    padding: 20px;',
        '    background: #1e1e1e;',
        '    color: #d4d4d4;',
        '}',
        'h1 { color: #569cd6; }',
        '.CodeMirror {',
        '    height: 300px;',
        '    border: 1px solid #569cd6;',
        '}',
        '#log {',
        '    margin-top: 20px;',
        '    padding: 10px;',
        '    background: #252526;',
        '    border: 1px solid #3c3c3c;',
        '    font-family: monospace;',
        '    font-size: 12px;',
        '    max-height: 200px;',
        '    overflow-y: auto;',
        '}',
        '.log-success { color: #4ec9b0; }',
        '.log-error { color: #f14c4c; }',
        '.log-info { color: #569cd6; }'
    ].join('\n');
    document.head.appendChild(style);

    // 加载 CodeMirror CSS
    loadCSS('../codemirror_test/codemirror.css');

    // 创建页面结构
    var container = document.createElement('div');
    container.innerHTML = [
        '<h1>🔧 CodeMirror 5 测试 (ESM Loader)</h1>',
        '<p>测试 CodeMirror 在 ESM 模式下的样式应用</p>',
        '<div id="editor-container"></div>',
        '<div id="log"></div>'
    ].join('');
    document.body.appendChild(container);

    function log(type, msg) {
        console.log('[' + type + '] ' + msg);
        var logDiv = document.getElementById('log');
        if (logDiv) {
            var p = document.createElement('div');
            p.className = 'log-' + type;
            p.textContent = msg;
            logDiv.appendChild(p);
        }
    }

    // 加载 CodeMirror 并初始化
    log('info', '加载 CodeMirror...');

    loadScriptSync('../codemirror_test/codemirror.js')
        .then(function() {
            log('success', 'CodeMirror 核心已加载');
            return loadScriptSync('../codemirror_test/javascript.js');
        })
        .then(function() {
            log('success', 'JavaScript 模式已加载');
            initCodeMirror();
        })
        .catch(function(e) {
            log('error', '加载失败: ' + e.message);
        });

    function initCodeMirror() {
        log('info', '初始化 CodeMirror...');
        
        var editorContainer = document.getElementById('editor-container');
        if (!editorContainer) {
            log('error', '找不到编辑器容器');
            return;
        }

        // 创建 textarea
        var textarea = document.createElement('textarea');
        textarea.id = 'code';
        textarea.value = [
            '// CodeMirror 5 测试代码',
            'function hello(name) {',
            '    console.log("Hello, " + name + "!");',
            '}',
            '',
            'hello("World");',
            '',
            '// 测试样式是否正确应用',
            'var x = 42;',
            'var y = "string";',
            'var z = true;'
        ].join('\n');
        editorContainer.appendChild(textarea);

        // 检查 CodeMirror 是否可用
        if (typeof CodeMirror === 'undefined') {
            log('error', 'CodeMirror 未定义');
            return;
        }

        log('success', 'CodeMirror 库已加载');

        // 创建编辑器
        try {
            var editor = CodeMirror.fromTextArea(textarea, {
                mode: 'javascript',
                theme: 'default',
                lineNumbers: true,
                indentUnit: 4,
                tabSize: 4
            });

            log('success', 'CodeMirror 编辑器创建成功');

            // 测试 API
            setTimeout(function() {
                runTests(editor);
            }, 500);

        } catch (e) {
            log('error', '创建编辑器失败: ' + e.message);
            console.error(e);
        }
    }

    function runTests(editor) {
        log('info', '=== 运行 API 测试 ===');

        // 测试 getValue
        var value = editor.getValue();
        log('success', 'getValue(): ' + value.length + ' 字符');

        // 测试 lineCount
        var lines = editor.lineCount();
        log('success', 'lineCount(): ' + lines + ' 行');

        // 测试 getCursor
        var cursor = editor.getCursor();
        log('success', 'getCursor(): 行 ' + cursor.line + ', 列 ' + cursor.ch);

        // 测试 setCursor
        editor.setCursor(2, 5);
        cursor = editor.getCursor();
        log('success', 'setCursor(2, 5): 行 ' + cursor.line + ', 列 ' + cursor.ch);

        // 测试 getLine
        var line = editor.getLine(0);
        log('success', 'getLine(0): "' + line.substring(0, 30) + '..."');

        // 测试 getComputedStyle
        log('info', '=== 测试 getComputedStyle ===');
        
        var cmElement = document.querySelector('.CodeMirror');
        if (cmElement) {
            var style = window.getComputedStyle(cmElement);
            log('info', 'CodeMirror height: ' + style.height);
            log('info', 'CodeMirror fontFamily: ' + style.fontFamily);
            log('info', 'CodeMirror backgroundColor: ' + style.backgroundColor);
            
            if (style.height && style.height !== '') {
                log('success', 'getComputedStyle 正常工作');
            } else {
                log('error', 'getComputedStyle 返回空值');
            }
        } else {
            log('error', '找不到 .CodeMirror 元素');
        }

        log('info', '=== 测试完成 ===');
    }
})();
