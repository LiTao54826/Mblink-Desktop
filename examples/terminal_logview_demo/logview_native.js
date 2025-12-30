/**
 * LogView 原生元素演示
 * 
 * 直接使用 <logview> 元素，由 C++ 层渲染
 * 使用方法: esm_loader.exe examples/terminal_logview_demo/logview_native.js
 */

import { h, render } from 'preact';
import { useRef, useEffect, useState } from 'preact/hooks';

console.log('=== LogView 原生元素演示 ===');

// 样式
const style = document.createElement('style');
style.textContent = `
    html, body {
        margin: 0;
        padding: 0;
        overflow: hidden;
        width: 100%;
        height: 100%;
        background: #1e1e1e;
        font-family: 'Segoe UI', sans-serif;
    }

    .app-container {
        display: flex;
        flex-direction: column;
        width: 100%;
        height: 100%;
    }

    .toolbar {
        display: flex;
        padding: 8px;
        background: #2d2d2d;
        gap: 8px;
        align-items: center;
    }

    .toolbar button {
        padding: 6px 12px;
        background: #0e639c;
        color: white;
        border: none;
        border-radius: 4px;
        cursor: pointer;
    }

    .toolbar button:hover {
        background: #1177bb;
    }

    .toolbar input {
        padding: 6px 8px;
        background: #3c3c3c;
        color: white;
        border: 1px solid #555;
        border-radius: 4px;
        width: 200px;
    }

    .toolbar select {
        padding: 6px 8px;
        background: #3c3c3c;
        color: white;
        border: 1px solid #555;
        border-radius: 4px;
    }

    .toolbar .stats {
        margin-left: auto;
        color: #888;
        font-size: 12px;
    }

    .logview-container {
        flex: 1;
        overflow: hidden;
    }
`;
document.head.appendChild(style);

// 日志级别
const LOG_LEVELS = ['DEBUG', 'INFO', 'WARN', 'ERROR', 'FATAL'];

// 模拟日志源
const LOG_SOURCES = ['app', 'network', 'database', 'ui', 'system'];

// 生成随机日志消息
function generateLogMessage(level, source) {
    const messages = {
        DEBUG: [
            'Variable x = 42',
            'Entering function processData()',
            'Cache hit for key: user_123',
            'Memory usage: 256MB',
        ],
        INFO: [
            'Application started successfully',
            'User logged in: john@example.com',
            'Request completed in 150ms',
            'Configuration loaded from config.json',
        ],
        WARN: [
            'Deprecated API usage detected',
            'Connection pool running low',
            'Retry attempt 2 of 3',
            'Response time exceeded threshold',
        ],
        ERROR: [
            'Failed to connect to database',
            'Invalid JSON in request body',
            'Authentication failed for user',
            'File not found: data.csv',
        ],
        FATAL: [
            'Out of memory exception',
            'Critical system failure',
            'Unrecoverable error in main loop',
            'Database corruption detected',
        ],
    };
    
    const levelMessages = messages[level] || messages.INFO;
    const msg = levelMessages[Math.floor(Math.random() * levelMessages.length)];
    return `[${source}] ${msg}`;
}

function App() {
    const logviewRef = useRef(null);
    const [logCount, setLogCount] = useState(0);
    const [searchQuery, setSearchQuery] = useState('');
    const [matchCount, setMatchCount] = useState(0);
    const [levelFilter, setLevelFilter] = useState('ALL');
    const [autoScroll, setAutoScroll] = useState(true);
    const [isGenerating, setIsGenerating] = useState(false);
    const intervalRef = useRef(null);

    // 添加单条日志
    const addLog = (level, source, message) => {
        if (logviewRef.current && logviewRef.current.append) {
            logviewRef.current.append(level, source, message);
            setLogCount(prev => prev + 1);
        }
    };

    // 添加随机日志
    const addRandomLog = () => {
        const level = LOG_LEVELS[Math.floor(Math.random() * LOG_LEVELS.length)];
        const source = LOG_SOURCES[Math.floor(Math.random() * LOG_SOURCES.length)];
        const message = generateLogMessage(level, source);
        addLog(level, source, message);
    };

    // 批量添加日志
    const addBatchLogs = (count) => {
        for (let i = 0; i < count; i++) {
            addRandomLog();
        }
    };

    // 开始/停止自动生成
    const toggleAutoGenerate = () => {
        if (isGenerating) {
            clearInterval(intervalRef.current);
            setIsGenerating(false);
        } else {
            intervalRef.current = setInterval(addRandomLog, 100);
            setIsGenerating(true);
        }
    };

    // 清空日志
    const clearLogs = () => {
        if (logviewRef.current && logviewRef.current.clear) {
            logviewRef.current.clear();
            setLogCount(0);
            setMatchCount(0);
        }
    };

    // 搜索
    const doSearch = () => {
        if (logviewRef.current && logviewRef.current.search) {
            const count = logviewRef.current.search(searchQuery);
            setMatchCount(count);
        }
    };

    // 清除搜索
    const clearSearch = () => {
        if (logviewRef.current && logviewRef.current.clearSearch) {
            logviewRef.current.clearSearch();
            setSearchQuery('');
            setMatchCount(0);
        }
    };

    // 下一个匹配
    const nextMatch = () => {
        if (logviewRef.current && logviewRef.current.nextMatch) {
            logviewRef.current.nextMatch();
        }
    };

    // 上一个匹配
    const prevMatch = () => {
        if (logviewRef.current && logviewRef.current.prevMatch) {
            logviewRef.current.prevMatch();
        }
    };

    // 设置级别过滤
    const handleLevelFilter = (e) => {
        const level = e.target.value;
        setLevelFilter(level);
        
        if (logviewRef.current && logviewRef.current.setLevelFilter) {
            if (level === 'ALL') {
                logviewRef.current.clearFilter();
            } else {
                logviewRef.current.setLevelFilter([level]);
            }
        }
    };

    // 导出日志
    const exportLogs = () => {
        if (logviewRef.current && logviewRef.current.export) {
            const text = logviewRef.current.export('text');
            console.log('=== Exported Logs ===');
            console.log(text);
        }
    };

    // 初始化
    useEffect(() => {
        console.log('LogView mounted, ref:', logviewRef.current);
        if (logviewRef.current) {
            console.log('LogView methods:', Object.keys(logviewRef.current));
            
            // 添加一些初始日志
            addLog('INFO', 'app', 'LogView demo started');
            addLog('DEBUG', 'system', 'Initializing components...');
            addLog('INFO', 'network', 'Connected to server');
            addLog('WARN', 'database', 'Connection pool size is low');
            addLog('ERROR', 'app', 'Failed to load user preferences');
            addLog('INFO', 'ui', 'UI rendering complete');
        }

        return () => {
            if (intervalRef.current) {
                clearInterval(intervalRef.current);
            }
        };
    }, []);

    return h('div', { class: 'app-container' },
        // 工具栏
        h('div', { class: 'toolbar' },
            h('button', { onClick: addRandomLog }, '+ Add Log'),
            h('button', { onClick: () => addBatchLogs(100) }, '+ 100 Logs'),
            h('button', { onClick: () => addBatchLogs(1000) }, '+ 1000 Logs'),
            h('button', { 
                onClick: toggleAutoGenerate,
                style: { background: isGenerating ? '#c42b1c' : '#0e639c' }
            }, isGenerating ? 'Stop Auto' : 'Auto Generate'),
            h('button', { onClick: clearLogs }, 'Clear'),
            
            // 分隔
            h('span', { style: { width: '1px', height: '20px', background: '#555' } }),
            
            // 搜索
            h('input', {
                type: 'text',
                placeholder: 'Search...',
                value: searchQuery,
                onInput: (e) => setSearchQuery(e.target.value),
                onKeyDown: (e) => e.key === 'Enter' && doSearch()
            }),
            h('button', { onClick: doSearch }, 'Search'),
            h('button', { onClick: prevMatch }, '◀'),
            h('button', { onClick: nextMatch }, '▶'),
            h('button', { onClick: clearSearch }, '✕'),
            
            // 级别过滤
            h('select', { value: levelFilter, onChange: handleLevelFilter },
                h('option', { value: 'ALL' }, 'All Levels'),
                ...LOG_LEVELS.map(level => 
                    h('option', { value: level }, level)
                )
            ),
            
            // 导出
            h('button', { onClick: exportLogs }, 'Export'),
            
            // 统计
            h('span', { class: 'stats' }, 
                `Logs: ${logCount}` + (matchCount > 0 ? ` | Matches: ${matchCount}` : '')
            )
        ),
        
        // LogView 容器
        h('div', { class: 'logview-container' },
            h('logview', {
                ref: logviewRef,
                'max-entries': 100000,
                'auto-scroll': autoScroll,
                'show-timestamp': true,
                'show-level': true,
                'show-source': true,
                style: {
                    width: '100%',
                    height: '100%'
                }
            })
        )
    );
}

console.log('开始渲染...');
render(h(App), document.body);
console.log('渲染完成');
