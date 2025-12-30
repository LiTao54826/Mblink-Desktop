/**
 * @file test_codemirror6.js
 * @brief CodeMirror 6 测试 (ESM Loader)
 * 
 * 使用方法: esm_loader.exe examples/preact_demo/test_codemirror6.js
 */

(function () {
    'use strict';

    var h = Preact.h;
    var render = Preact.render;
    var useState = PreactHooks.useState;
    var useEffect = PreactHooks.useEffect;
    var useRef = PreactHooks.useRef;

    // 全局样式
    var globalStyles = [
        'body {',
        '    font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;',
        '    margin: 0;',
        '    padding: 20px;',
        '    background: #1e1e1e;',
        '    color: #d4d4d4;',
        '}',
        'h1 { color: #569cd6; margin-bottom: 10px; }',
        '.editor-container {',
        '    border: 1px solid #569cd6;',
        '    border-radius: 4px;',
        '    overflow: hidden;',
        '}',
        '.cm-editor {',
        '    height: 300px;',
        '}',
        '#log {',
        '    margin-top: 20px;',
        '    padding: 10px;',
        '    background: #252526;',
        '    border: 1px solid #3c3c3c;',
        '    border-radius: 4px;',
        '    font-family: monospace;',
        '    font-size: 12px;',
        '    max-height: 200px;',
        '    overflow-y: auto;',
        '}',
        '.log-success { color: #4ec9b0; }',
        '.log-error { color: #f14c4c; }',
        '.log-info { color: #569cd6; }'
    ].join('\n');

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

    // CodeMirror 编辑器组件
    function CodeMirrorEditor(props) {
        var containerRef = useRef(null);
        var editorRef = useRef(null);

        useEffect(function() {
            if (!containerRef.current) return;
            if (!window.CodeMirror6) {
                log('error', 'CodeMirror6 未加载');
                return;
            }

            log('info', '初始化 CodeMirror 6...');

            var CM = window.CodeMirror6;
            
            try {
                var view = new CM.EditorView({
                    doc: props.value || '// 在这里输入代码...',
                    extensions: [
                        CM.basicSetup,
                        CM.javascript(),
                        CM.oneDark
                    ],
                    parent: containerRef.current
                });

                editorRef.current = view;
                log('success', 'CodeMirror 6 编辑器创建成功');

                // 测试 getComputedStyle
                setTimeout(function() {
                    testComputedStyle();
                }, 500);

            } catch (e) {
                log('error', '创建编辑器失败: ' + e.message);
                console.error(e);
            }

            return function() {
                if (editorRef.current) {
                    editorRef.current.destroy();
                }
            };
        }, []);

        return h('div', { 
            ref: containerRef, 
            className: 'editor-container'
        });
    }

    function testComputedStyle() {
        log('info', '=== 测试 getComputedStyle ===');

        var cmEditor = document.querySelector('.cm-editor');
        if (cmEditor) {
            var style = window.getComputedStyle(cmEditor);
            log('info', 'cm-editor height: ' + style.height);
            log('info', 'cm-editor backgroundColor: ' + style.backgroundColor);
            log('info', 'cm-editor fontFamily: ' + style.fontFamily);

            if (style.height && style.height !== '') {
                log('success', 'getComputedStyle 正常工作');
            } else {
                log('error', 'getComputedStyle 返回空值');
            }
        } else {
            log('error', '找不到 .cm-editor 元素');
        }

        var cmContent = document.querySelector('.cm-content');
        if (cmContent) {
            var contentStyle = window.getComputedStyle(cmContent);
            log('info', 'cm-content color: ' + contentStyle.color);
            log('info', 'cm-content fontSize: ' + contentStyle.fontSize);
        }
    }

    // 主应用
    function App() {
        var initialCode = [
            '// CodeMirror 6 测试代码',
            'function hello(name) {',
            '    console.log("Hello, " + name + "!");',
            '}',
            '',
            'hello("World");',
            '',
            '// 测试语法高亮',
            'const x = 42;',
            'let y = "string";',
            'var z = true;'
        ].join('\n');

        return h('div', null,
            h('style', null, globalStyles),
            h('h1', null, '🔧 CodeMirror 6 测试'),
            h('p', null, '测试 CodeMirror 6 在 ESM Loader 中的样式应用'),
            h(CodeMirrorEditor, { value: initialCode }),
            h('div', { id: 'log' })
        );
    }

    // 先加载 CodeMirror 6 bundle
    log('info', '加载 CodeMirror 6 bundle...');
    
    var script = document.createElement('script');
    script.textContent = '/* CodeMirror 6 will be loaded here */';
    
    // 使用 XMLHttpRequest 同步加载（因为没有 fetch）
    var xhr = new XMLHttpRequest();
    xhr.open('GET', '../codemirror6/codemirror6.bundle.js', false); // 同步
    xhr.send();
    
    if (xhr.status === 200) {
        try {
            eval(xhr.responseText);
            log('success', 'CodeMirror 6 bundle 已加载');
            
            // 渲染应用
            render(h(App, null), document.body);
            log('success', '应用已渲染');
        } catch (e) {
            log('error', '执行 bundle 失败: ' + e.message);
            console.error(e);
        }
    } else {
        log('error', '加载 bundle 失败: ' + xhr.status);
    }
})();
