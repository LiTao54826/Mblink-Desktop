/**
 * Terminal 原生元素演示
 * 
 * 直接使用 <terminal> 元素，由 C++ 层渲染
 * 使用方法: esm_loader.exe examples/terminal_logview_demo/terminal_native.js
 */

import { h, render } from 'preact';
import { useRef, useEffect } from 'preact/hooks';

console.log('=== Terminal 原生元素演示 ===');

// 样式 - 使用 100% 确保铺满窗口
const style = document.createElement('style');
style.textContent = `
    html, body {
        margin: 0;
        padding: 0;
        overflow: auto;
        width: 100%;
        height: 100%;
        background: #000000ff;
    }

    .app-container {
        width: 100%;
        height: 100%;
    }
`;
document.head.appendChild(style);

function App() {
    const terminalRef = useRef(null);

    useEffect(() => {
        // 启动 shell
        console.log('useEffect called, terminalRef.current:', terminalRef.current);
        if (terminalRef.current) {
            console.log('Terminal methods:', Object.keys(terminalRef.current));
            console.log('startShell exists:', typeof terminalRef.current.startShell);
            if (terminalRef.current.startShell) {
                console.log('Starting shell...');
                terminalRef.current.startShell('cmd.exe');
            } else {
                console.log('startShell method not found!');
            }
        } else {
            console.log('terminalRef.current is null');
        }
    }, []);

    return h('div', { className: 'app-container' },
        h('terminal', {
            ref: terminalRef,
            rows: 24,
            cols: 80,
            style: { 
                width: '100%',
                height: '100%'
            }
        })
    );
}

console.log('开始渲染...');
render(h(App), document.body);
console.log('渲染完成');
