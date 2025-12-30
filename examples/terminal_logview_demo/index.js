/**
 * Terminal 和 LogView 元素演示
 * 
 * 使用方法: esm_loader.exe examples/terminal_logview_demo/index.js
 */

import { h, render } from 'preact';
import { useState, useEffect, useRef } from 'preact/hooks';
import { Input } from '../../js/components/input.js';

console.log('=== Terminal & LogView 演示 ===');

// ========== 样式定义 ==========
const style = document.createElement('style');
style.textContent = `
    .app-container {
        padding: 20px;
        background: #1e1e1e;
        color: #d4d4d4;
        font-family: 'Consolas', 'Monaco', monospace;
        min-height: 100vh;
    }

    .section {
        margin-bottom: 20px;
        padding: 16px;
        background: #252526;
        border-radius: 8px;
    }

    .title {
        font-size: 16px;
        font-weight: bold;
        color: #569cd6;
        margin-bottom: 12px;
    }

    .btn {
        padding: 8px 16px;
        margin-right: 8px;
        border: none;
        border-radius: 4px;
        cursor: pointer;
        font-size: 13px;
        background: #0e639c;
        color: white;
    }

    .btn:hover {
        background: #1177bb;
    }

    .terminal-output {
        background: #0c0c0c;
        color: #cccccc;
        padding: 12px;
        font-family: 'Consolas', monospace;
        font-size: 13px;
        white-space: pre-wrap;
        min-height: 200px;
        max-height: 300px;
        overflow-y: auto;
        border-radius: 4px;
        margin-bottom: 12px;
    }

    .log-entry {
        padding: 4px 8px;
        font-size: 12px;
        border-bottom: 1px solid #333;
    }

    .log-DEBUG { color: #808080; }
    .log-INFO { color: #4fc1ff; }
    .log-WARN { color: #dcdcaa; }
    .log-ERROR { color: #f14c4c; }

    .input-row {
        display: flex;
        gap: 8px;
        align-items: center;
    }

    .mode-toggle {
        font-size: 12px;
        color: #888;
        margin-bottom: 8px;
    }

    .mode-toggle label {
        cursor: pointer;
        margin-right: 16px;
    }
`;
document.head.appendChild(style);

// ========== 日志管理器 ==========
const logManager = {
    listeners: [],
    addListener(cb) { this.listeners.push(cb); },
    log(level, source, message) {
        const entry = { timestamp: Date.now(), level, source, message };
        this.listeners.forEach(cb => cb(entry));
    },
    info(source, msg) { this.log('INFO', source, msg); },
    debug(source, msg) { this.log('DEBUG', source, msg); },
    warn(source, msg) { this.log('WARN', source, msg); },
    error(source, msg) { this.log('ERROR', source, msg); }
};

// ========== Terminal 组件 ==========
function TerminalDemo() {
    const terminalRef = useRef(null);
    const [input, setInput] = useState('');
    const [output, setOutput] = useState('$ ');
    const [useRealShell, setUseRealShell] = useState(false);
    const [realOutput, setRealOutput] = useState('');

    // 定时获取真实终端输出
    useEffect(() => {
        if (!useRealShell) return;
        
        const interval = setInterval(() => {
            if (terminalRef.current && terminalRef.current.serialize) {
                const content = terminalRef.current.serialize();
                if (content && content !== realOutput) {
                    setRealOutput(content);
                }
            }
        }, 500);

        return () => clearInterval(interval);
    }, [useRealShell, realOutput]);

    function executeCommand(cmd) {
        if (!cmd.trim()) return;

        if (useRealShell) {
            // 使用真正的 shell 执行命令
            if (terminalRef.current && terminalRef.current.execute) {
                setInput('');
                try {
                    terminalRef.current.execute(cmd);
                } catch (e) {
                    setRealOutput(realOutput + '\nError: ' + e.message);
                }
            } else {
                setRealOutput('Error: terminal.execute not available');
            }
        } else {
            // 模拟终端命令
            let newOutput = output + cmd + '\n';
            const command = cmd.trim().toLowerCase();

            if (command === 'help') {
                newOutput += 'Built-in commands: help, clear, echo <text>, date, log <n>\n';
                newOutput += 'Toggle "Real Shell" to execute actual CMD commands.\n';
            } else if (command === 'clear') {
                newOutput = '';
            } else if (command.startsWith('echo ')) {
                newOutput += cmd.slice(5) + '\n';
            } else if (command === 'date') {
                newOutput += new Date().toString() + '\n';
            } else if (command.startsWith('log')) {
                const n = parseInt(command.split(' ')[1]) || 5;
                for (let i = 0; i < n; i++) {
                    const levels = ['DEBUG', 'INFO', 'WARN', 'ERROR'];
                    const level = levels[Math.floor(Math.random() * levels.length)];
                    logManager.log(level, 'Test', 'Log message ' + (i + 1));
                }
                newOutput += 'Generated ' + n + ' log entries\n';
            } else {
                newOutput += 'Unknown command: ' + command + '\n';
                newOutput += 'Type "help" for available commands.\n';
            }

            newOutput += '$ ';
            setOutput(newOutput);
            setInput('');

            if (terminalRef.current && terminalRef.current.write) {
                terminalRef.current.clear();
                terminalRef.current.write(newOutput);
            }
        }
    }

    return h('div', { className: 'section' },
        h('div', { className: 'title' }, '🖥️ Terminal Demo'),
        h('div', { className: 'mode-toggle' },
            h('label', null,
                h('input', {
                    type: 'checkbox',
                    checked: useRealShell,
                    onChange: (e) => setUseRealShell(e.target.checked)
                }),
                ' Real Shell (execute CMD commands)'
            )
        ),
        h('div', { className: 'terminal-output' }, useRealShell ? (realOutput || '(Waiting for output...)') : output),
        h('terminal', { ref: terminalRef, style: { display: 'none' } }),
        h('div', { className: 'input-row' },
            h(Input, {
                placeholder: useRealShell ? 'Enter CMD command (e.g., dir, echo hello)' : 'Enter command (try: help)',
                value: input,
                onChange: setInput,
                onEnter: () => executeCommand(input),
                style: { flex: 1 }
            }),
            h('button', { className: 'btn', onClick: () => executeCommand(input) }, 'Run')
        )
    );
}

// ========== LogView 组件 ==========
function LogViewDemo() {
    const logviewRef = useRef(null);
    const [logs, setLogs] = useState([]);

    useEffect(() => {
        function handleLog(entry) {
            setLogs(prev => [...prev.slice(-49), entry]);
            if (logviewRef.current && logviewRef.current.append) {
                logviewRef.current.append(entry.level, entry.source, entry.message);
            }
        }
        logManager.addListener(handleLog);
        
        // 初始日志
        logManager.info('System', 'Demo started');
        logManager.debug('Config', 'Loading...');
        logManager.info('Config', 'Ready');
    }, []);

    function clearLogs() {
        setLogs([]);
        if (logviewRef.current && logviewRef.current.clear) {
            logviewRef.current.clear();
        }
    }

    return h('div', { className: 'section' },
        h('div', { className: 'title' }, '📋 LogView Demo (' + logs.length + ' entries)'),
        h('logview', { ref: logviewRef, style: { display: 'none' } }),
        h('div', { style: { maxHeight: '200px', overflowY: 'auto', marginBottom: '12px' } },
            logs.map((log, i) => 
                h('div', { key: i, className: 'log-entry log-' + log.level },
                    '[' + new Date(log.timestamp).toLocaleTimeString() + '] ',
                    '[' + log.level + '] [' + log.source + '] ' + log.message
                )
            )
        ),
        h('button', { className: 'btn', onClick: clearLogs }, '🗑️ Clear'),
        h('button', { className: 'btn', onClick: () => logManager.error('Test', 'Test error!') }, '⚠️ Add Error')
    );
}

// ========== 主应用 ==========
function App() {
    return h('div', { className: 'app-container' },
        h('h1', { style: { color: '#569cd6', marginBottom: '20px' } }, '🚀 Terminal & LogView Demo'),
        h(TerminalDemo),
        h(LogViewDemo)
    );
}

console.log('开始渲染...');
render(h(App), document.body);
console.log('渲染完成');
