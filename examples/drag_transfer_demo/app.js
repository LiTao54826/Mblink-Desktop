/**
 * @file app.js
 * @brief 左右列表拖放示例
 * 
 * 演示如何使用 HTML5 Drag and Drop API 实现两个列表之间的项目转移
 */

import { h, render } from 'preact';
import { useState } from 'preact/hooks';

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
        marginBottom: '8px'
    },
    subtitle: {
        textAlign: 'center',
        color: '#666',
        fontSize: '14px',
        marginBottom: '24px'
    },
    container: {
        display: 'flex',
        gap: '24px'
    },
    panel: {
        flex: 1,
        backgroundColor: 'white',
        borderRadius: '12px',
        padding: '16px',
        boxShadow: '0 2px 8px rgba(0,0,0,0.1)'
    },
    panelHeader: {
        display: 'flex',
        justifyContent: 'space-between',
        alignItems: 'center',
        marginBottom: '16px',
        paddingBottom: '12px',
        borderBottom: '1px solid #eee'
    },
    panelTitle: {
        fontSize: '16px',
        fontWeight: 'bold',
        color: '#333'
    },
    panelCount: {
        fontSize: '13px',
        color: '#999',
        backgroundColor: '#f0f0f0',
        padding: '4px 10px',
        borderRadius: '12px'
    },
    list: {
        minHeight: '300px',
        padding: '8px',
        backgroundColor: '#fafafa',
        borderRadius: '8px',
        border: '2px dashed transparent',
        transition: 'all 0.2s'
    },
    listDragOver: {
        borderColor: '#52c41a',
        backgroundColor: '#f6ffed'
    },
    item: {
        display: 'flex',
        alignItems: 'center',
        padding: '12px',
        marginBottom: '8px',
        backgroundColor: 'white',
        border: '1px solid #e8e8e8',
        borderRadius: '6px',
        cursor: 'grab',
        transition: 'all 0.2s'
    },
    itemDragging: {
        opacity: '0.5',
        transform: 'rotate(2deg)'
    },
    itemIcon: {
        width: '36px',
        height: '36px',
        borderRadius: '8px',
        display: 'flex',
        alignItems: 'center',
        justifyContent: 'center',
        marginRight: '12px',
        fontSize: '18px',
        color: 'white'
    },
    itemContent: {
        flex: 1
    },
    itemName: {
        fontWeight: '500',
        color: '#333',
        marginBottom: '2px'
    },
    itemCategory: {
        fontSize: '12px',
        color: '#999'
    },
    emptyState: {
        textAlign: 'center',
        padding: '40px 20px',
        color: '#999'
    },
    emptyIcon: {
        fontSize: '48px',
        marginBottom: '12px'
    },
    actions: {
        display: 'flex',
        gap: '12px',
        marginTop: '20px',
        justifyContent: 'center'
    },
    button: {
        padding: '10px 20px',
        border: 'none',
        borderRadius: '6px',
        fontSize: '14px',
        cursor: 'pointer'
    },
    primaryButton: {
        backgroundColor: '#1890ff',
        color: 'white'
    },
    secondaryButton: {
        backgroundColor: '#f0f0f0',
        color: '#666'
    }
};

// 初始数据
const allItems = [
    { id: 1, name: 'JavaScript', category: '编程语言', icon: '📜', color: '#f7df1e' },
    { id: 2, name: 'Python', category: '编程语言', icon: '🐍', color: '#3776ab' },
    { id: 3, name: 'React', category: '前端框架', icon: '⚛️', color: '#61dafb' },
    { id: 4, name: 'Vue', category: '前端框架', icon: '💚', color: '#42b883' },
    { id: 5, name: 'Node.js', category: '运行时', icon: '🟢', color: '#339933' },
    { id: 6, name: 'Docker', category: '容器', icon: '🐳', color: '#2496ed' },
    { id: 7, name: 'Git', category: '版本控制', icon: '📦', color: '#f05032' },
    { id: 8, name: 'MongoDB', category: '数据库', icon: '🍃', color: '#47a248' }
];

// 列表项组件
function ListItem({ item, listId, onDragStart, isDragging }) {
    const itemStyle = {
        ...styles.item,
        ...(isDragging ? styles.itemDragging : {})
    };

    return h('div', {
        draggable: 'true',
        style: itemStyle,
        onDragStart: (e) => {
            e.dataTransfer.setData('application/json', JSON.stringify({ item, fromList: listId }));
            e.dataTransfer.effectAllowed = 'move';
            onDragStart(item.id);
        },
        onDragEnd: () => {
            onDragStart(null);
        }
    }, [
        h('div', { style: { ...styles.itemIcon, backgroundColor: item.color } }, item.icon),
        h('div', { style: styles.itemContent }, [
            h('div', { style: styles.itemName }, item.name),
            h('div', { style: styles.itemCategory }, item.category)
        ])
    ]);
}

// 列表面板组件
function ListPanel({ title, listId, items, onDrop, onDragStart, draggingId, isDragOver, onDragOver, onDragLeave }) {
    const listStyle = {
        ...styles.list,
        ...(isDragOver ? styles.listDragOver : {})
    };

    return h('div', { style: styles.panel }, [
        h('div', { style: styles.panelHeader }, [
            h('span', { style: styles.panelTitle }, title),
            h('span', { style: styles.panelCount }, `${items.length} 项`)
        ]),
        h('div', {
            style: listStyle,
            onDragOver: (e) => {
                e.preventDefault();
                e.dataTransfer.dropEffect = 'move';
                onDragOver(listId);
            },
            onDragLeave: () => {
                onDragLeave(listId);
            },
            onDrop: (e) => {
                e.preventDefault();
                try {
                    const data = JSON.parse(e.dataTransfer.getData('application/json'));
                    onDrop(data.item, data.fromList, listId);
                } catch (err) {
                    console.error('Drop error:', err);
                }
            }
        }, items.length > 0
            ? items.map(item =>
                h(ListItem, {
                    key: item.id,
                    item,
                    listId,
                    onDragStart,
                    isDragging: draggingId === item.id
                })
            )
            : h('div', { style: styles.emptyState }, [
                h('div', { style: styles.emptyIcon }, '📭'),
                h('div', {}, '拖放项目到这里')
            ])
        )
    ]);
}

// 主应用组件
function App() {
    const [leftItems, setLeftItems] = useState(allItems.slice(0, 4));
    const [rightItems, setRightItems] = useState(allItems.slice(4));
    const [draggingId, setDraggingId] = useState(null);
    const [dragOverList, setDragOverList] = useState(null);

    const handleDrop = (item, fromList, toList) => {
        if (fromList === toList) return;

        if (fromList === 'left' && toList === 'right') {
            setLeftItems(prev => prev.filter(i => i.id !== item.id));
            setRightItems(prev => [...prev, item]);
        } else if (fromList === 'right' && toList === 'left') {
            setRightItems(prev => prev.filter(i => i.id !== item.id));
            setLeftItems(prev => [...prev, item]);
        }

        setDragOverList(null);
    };

    const handleDragOver = (listId) => {
        setDragOverList(listId);
    };

    const handleDragLeave = (listId) => {
        if (dragOverList === listId) {
            setDragOverList(null);
        }
    };

    const moveAllRight = () => {
        setRightItems(prev => [...prev, ...leftItems]);
        setLeftItems([]);
    };

    const moveAllLeft = () => {
        setLeftItems(prev => [...prev, ...rightItems]);
        setRightItems([]);
    };

    const reset = () => {
        setLeftItems(allItems.slice(0, 4));
        setRightItems(allItems.slice(4));
    };

    return h('div', { style: styles.app }, [
        h('h1', { style: styles.title }, '左右列表拖放'),
        h('p', { style: styles.subtitle }, '在两个列表之间拖放项目'),

        h('div', { style: styles.container }, [
            h(ListPanel, {
                title: '可用技术',
                listId: 'left',
                items: leftItems,
                onDrop: handleDrop,
                onDragStart: setDraggingId,
                draggingId,
                isDragOver: dragOverList === 'left',
                onDragOver: handleDragOver,
                onDragLeave: handleDragLeave
            }),
            h(ListPanel, {
                title: '已选技术',
                listId: 'right',
                items: rightItems,
                onDrop: handleDrop,
                onDragStart: setDraggingId,
                draggingId,
                isDragOver: dragOverList === 'right',
                onDragOver: handleDragOver,
                onDragLeave: handleDragLeave
            })
        ]),

        h('div', { style: styles.actions }, [
            h('button', {
                style: { ...styles.button, ...styles.secondaryButton },
                onClick: moveAllLeft
            }, '← 全部移左'),
            h('button', {
                style: { ...styles.button, ...styles.primaryButton },
                onClick: reset
            }, '重置'),
            h('button', {
                style: { ...styles.button, ...styles.secondaryButton },
                onClick: moveAllRight
            }, '全部移右 →')
        ])
    ]);
}

// 渲染应用
console.log('Starting Drag Transfer Demo...');
render(h(App), document.body);
console.log('Drag Transfer Demo rendered!');
