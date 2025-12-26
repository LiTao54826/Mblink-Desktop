/**
 * @file app.js
 * @brief HTML5 拖拽 API 测试
 * 
 * 测试标准 HTML5 Drag and Drop API:
 * - draggable 属性
 * - dragstart, drag, dragend 事件
 * - dragenter, dragleave, dragover, drop 事件
 * - DataTransfer API
 */

import { h, render } from 'preact';
import { useState, useRef, useEffect } from 'preact/hooks';

// 样式定义
const styles = {
    app: {
        fontFamily: 'Arial, sans-serif',
        padding: '20px',
        maxWidth: '800px',
        margin: '0 auto',
        backgroundColor: '#f5f5f5',
        minHeight: '100vh'
    },
    title: {
        textAlign: 'center',
        color: '#333',
        marginBottom: '20px'
    },
    section: {
        backgroundColor: 'white',
        borderRadius: '8px',
        padding: '16px',
        marginBottom: '16px',
        boxShadow: '0 2px 4px rgba(0,0,0,0.1)'
    },
    sectionTitle: {
        fontSize: '18px',
        fontWeight: 'bold',
        marginBottom: '12px',
        color: '#333'
    },
    dragContainer: {
        display: 'flex',
        gap: '16px',
        flexWrap: 'wrap'
    },
    draggableItem: {
        width: '100px',
        height: '100px',
        backgroundColor: '#4CAF50',
        color: 'white',
        display: 'flex',
        alignItems: 'center',
        justifyContent: 'center',
        borderRadius: '8px',
        cursor: 'grab',
        userSelect: 'none',
        fontWeight: 'bold',
        transition: 'transform 0.2s, box-shadow 0.2s'
    },
    draggableItemDragging: {
        opacity: '0.5',
        transform: 'scale(1.05)',
        boxShadow: '0 4px 12px rgba(0,0,0,0.3)'
    },
    dropZone: {
        width: '200px',
        height: '150px',
        border: '2px dashed #ccc',
        borderRadius: '8px',
        display: 'flex',
        alignItems: 'center',
        justifyContent: 'center',
        color: '#999',
        transition: 'all 0.2s'
    },
    dropZoneActive: {
        borderColor: '#4CAF50',
        backgroundColor: 'rgba(76, 175, 80, 0.1)',
        color: '#4CAF50'
    },
    dropZoneHover: {
        borderColor: '#2196F3',
        backgroundColor: 'rgba(33, 150, 243, 0.1)',
        color: '#2196F3'
    },
    eventLog: {
        backgroundColor: '#1e1e1e',
        color: '#d4d4d4',
        padding: '12px',
        borderRadius: '4px',
        fontFamily: 'monospace',
        fontSize: '12px',
        maxHeight: '200px',
        overflowY: 'auto'
    },
    logEntry: {
        marginBottom: '4px'
    },
    logTime: {
        color: '#6a9955'
    },
    logEvent: {
        color: '#569cd6'
    },
    logData: {
        color: '#ce9178'
    },
    clearButton: {
        backgroundColor: '#f44336',
        color: 'white',
        border: 'none',
        padding: '8px 16px',
        borderRadius: '4px',
        cursor: 'pointer',
        marginTop: '8px'
    },
    droppedItems: {
        display: 'flex',
        gap: '8px',
        flexWrap: 'wrap',
        alignItems: 'center',
        justifyContent: 'center',
        width: '100%',
        height: '100%'
    },
    droppedItem: {
        padding: '8px 12px',
        backgroundColor: '#2196F3',
        color: 'white',
        borderRadius: '4px',
        fontSize: '14px'
    }
};

// 可拖拽项目组件
function DraggableItem({ id, label, color, onDragStart, onDragEnd, isDragging }) {
    const itemStyle = {
        ...styles.draggableItem,
        backgroundColor: color,
        ...(isDragging ? styles.draggableItemDragging : {})
    };

    return h('div', {
        draggable: 'true',
        style: itemStyle,
        onDragStart: (e) => {
            console.log('[DraggableItem] dragstart:', id);
            if (e.dataTransfer) {
                e.dataTransfer.setData('text/plain', id);
                e.dataTransfer.setData('application/json', JSON.stringify({ id, label, color }));
                e.dataTransfer.effectAllowed = 'move';
            }
            onDragStart && onDragStart(id);
        },
        onDrag: (e) => {
            // drag 事件在拖拽过程中持续触发
        },
        onDragEnd: (e) => {
            console.log('[DraggableItem] dragend:', id);
            onDragEnd && onDragEnd(id);
        }
    }, label);
}

// 放置区域组件
function DropZone({ id, label, onDrop, onDragEnter, onDragLeave, isActive, isHover, children }) {
    const zoneStyle = {
        ...styles.dropZone,
        ...(isActive ? styles.dropZoneActive : {}),
        ...(isHover ? styles.dropZoneHover : {})
    };

    return h('div', {
        style: zoneStyle,
        onDragEnter: (e) => {
            e.preventDefault();
            console.log('[DropZone] dragenter:', id);
            onDragEnter && onDragEnter(id);
        },
        onDragOver: (e) => {
            e.preventDefault();  // 必须阻止默认行为才能接收 drop
            if (e.dataTransfer) {
                e.dataTransfer.dropEffect = 'move';
            }
        },
        onDragLeave: (e) => {
            console.log('[DropZone] dragleave:', id);
            onDragLeave && onDragLeave(id);
        },
        onDrop: (e) => {
            e.preventDefault();
            console.log('[DropZone] drop:', id);
            
            let data = null;
            if (e.dataTransfer) {
                const textData = e.dataTransfer.getData('text/plain');
                const jsonData = e.dataTransfer.getData('application/json');
                console.log('[DropZone] getData text:', textData);
                console.log('[DropZone] getData json:', jsonData);
                
                try {
                    data = jsonData ? JSON.parse(jsonData) : { id: textData };
                } catch (err) {
                    data = { id: textData };
                }
            }
            
            onDrop && onDrop(id, data);
        }
    }, children || label);
}


// 事件日志组件
function EventLog({ logs, onClear }) {
    const logRef = useRef(null);

    useEffect(() => {
        // 自动滚动到底部
        if (logRef.current) {
            logRef.current.scrollTop = logRef.current.scrollHeight;
        }
    }, [logs]);

    return h('div', { style: styles.section }, [
        h('div', { style: styles.sectionTitle }, '事件日志'),
        h('div', { ref: logRef, style: styles.eventLog }, 
            logs.length === 0 
                ? h('div', { style: { color: '#666' } }, '拖拽元素以查看事件...')
                : logs.map((log, i) => 
                    h('div', { key: i, style: styles.logEntry }, [
                        h('span', { style: styles.logTime }, `[${log.time}] `),
                        h('span', { style: styles.logEvent }, log.event),
                        log.data ? h('span', { style: styles.logData }, ` - ${log.data}`) : null
                    ])
                )
        ),
        h('button', { 
            style: styles.clearButton, 
            onClick: onClear 
        }, '清除日志')
    ]);
}

// 主应用组件
function App() {
    const [draggingId, setDraggingId] = useState(null);
    const [activeZone, setActiveZone] = useState(null);
    const [hoverZone, setHoverZone] = useState(null);
    const [droppedItems, setDroppedItems] = useState({ zone1: [], zone2: [] });
    const [logs, setLogs] = useState([]);

    // 添加日志
    const addLog = (event, data) => {
        const now = new Date();
        const time = `${now.getHours().toString().padStart(2, '0')}:${now.getMinutes().toString().padStart(2, '0')}:${now.getSeconds().toString().padStart(2, '0')}.${now.getMilliseconds().toString().padStart(3, '0')}`;
        setLogs(prev => [...prev.slice(-50), { time, event, data }]);  // 保留最近50条
    };

    // 可拖拽项目数据
    const items = [
        { id: 'item1', label: 'Item 1', color: '#4CAF50' },
        { id: 'item2', label: 'Item 2', color: '#2196F3' },
        { id: 'item3', label: 'Item 3', color: '#FF9800' },
        { id: 'item4', label: 'Item 4', color: '#9C27B0' }
    ];

    // 事件处理
    const handleDragStart = (id) => {
        setDraggingId(id);
        setActiveZone('any');
        addLog('dragstart', `开始拖拽 ${id}`);
    };

    const handleDragEnd = (id) => {
        setDraggingId(null);
        setActiveZone(null);
        setHoverZone(null);
        addLog('dragend', `结束拖拽 ${id}`);
    };

    const handleDragEnter = (zoneId) => {
        setHoverZone(zoneId);
        addLog('dragenter', `进入 ${zoneId}`);
    };

    const handleDragLeave = (zoneId) => {
        if (hoverZone === zoneId) {
            setHoverZone(null);
        }
        addLog('dragleave', `离开 ${zoneId}`);
    };

    const handleDrop = (zoneId, data) => {
        addLog('drop', `放置到 ${zoneId}: ${JSON.stringify(data)}`);
        
        if (data && data.id) {
            setDroppedItems(prev => ({
                ...prev,
                [zoneId]: [...prev[zoneId], data]
            }));
        }
        
        setHoverZone(null);
        setActiveZone(null);
    };

    const clearLogs = () => {
        setLogs([]);
    };

    const clearDropped = () => {
        setDroppedItems({ zone1: [], zone2: [] });
    };

    return h('div', { style: styles.app }, [
        h('h1', { style: styles.title }, 'HTML5 拖拽 API 测试'),
        
        // 可拖拽项目区域
        h('div', { style: styles.section }, [
            h('div', { style: styles.sectionTitle }, '可拖拽项目'),
            h('p', { style: { color: '#666', marginBottom: '12px' } }, 
                '拖拽下面的方块到放置区域'),
            h('div', { style: styles.dragContainer },
                items.map(item => 
                    h(DraggableItem, {
                        key: item.id,
                        ...item,
                        isDragging: draggingId === item.id,
                        onDragStart: handleDragStart,
                        onDragEnd: handleDragEnd
                    })
                )
            )
        ]),

        // 放置区域
        h('div', { style: styles.section }, [
            h('div', { style: styles.sectionTitle }, '放置区域'),
            h('div', { style: { display: 'flex', gap: '16px' } }, [
                h('div', { style: { flex: 1 } }, [
                    h(DropZone, {
                        id: 'zone1',
                        label: '放置区域 1',
                        isActive: activeZone !== null,
                        isHover: hoverZone === 'zone1',
                        onDragEnter: handleDragEnter,
                        onDragLeave: handleDragLeave,
                        onDrop: handleDrop
                    }, droppedItems.zone1.length > 0 
                        ? h('div', { style: styles.droppedItems },
                            droppedItems.zone1.map((item, i) => 
                                h('div', { 
                                    key: i, 
                                    style: { ...styles.droppedItem, backgroundColor: item.color || '#2196F3' } 
                                }, item.label || item.id)
                            )
                        )
                        : '放置区域 1'
                    )
                ]),
                h('div', { style: { flex: 1 } }, [
                    h(DropZone, {
                        id: 'zone2',
                        label: '放置区域 2',
                        isActive: activeZone !== null,
                        isHover: hoverZone === 'zone2',
                        onDragEnter: handleDragEnter,
                        onDragLeave: handleDragLeave,
                        onDrop: handleDrop
                    }, droppedItems.zone2.length > 0 
                        ? h('div', { style: styles.droppedItems },
                            droppedItems.zone2.map((item, i) => 
                                h('div', { 
                                    key: i, 
                                    style: { ...styles.droppedItem, backgroundColor: item.color || '#2196F3' } 
                                }, item.label || item.id)
                            )
                        )
                        : '放置区域 2'
                    )
                ])
            ]),
            h('button', { 
                style: { ...styles.clearButton, backgroundColor: '#666', marginTop: '12px' }, 
                onClick: clearDropped 
            }, '清除放置的项目')
        ]),

        // 事件日志
        h(EventLog, { logs, onClear: clearLogs }),

        // DataTransfer API 测试说明
        h('div', { style: styles.section }, [
            h('div', { style: styles.sectionTitle }, 'DataTransfer API 测试'),
            h('ul', { style: { color: '#666', paddingLeft: '20px', margin: 0 } }, [
                h('li', {}, 'dragstart 时设置 dataTransfer.setData()'),
                h('li', {}, 'drop 时获取 dataTransfer.getData()'),
                h('li', {}, 'dragover 时设置 dataTransfer.dropEffect'),
                h('li', {}, 'dragstart 时设置 dataTransfer.effectAllowed'),
                h('li', {}, '支持多种数据格式 (text/plain, application/json)')
            ])
        ])
    ]);
}

// 渲染应用
console.log('Starting Drag and Drop Test App...');
render(h(App), document.body);
console.log('Drag and Drop Test App rendered!');
