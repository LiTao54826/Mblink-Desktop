/**
 * @file app.js
 * @brief 拖拽排序示例
 * 
 * 演示如何使用 HTML5 Drag and Drop API 实现列表项拖拽排序
 */

import { h, render } from 'preact';
import { useState } from 'preact/hooks';

// 样式定义
const styles = {
    app: {
        fontFamily: 'Arial, sans-serif',
        padding: '20px',
        maxWidth: '500px',
        margin: '0 auto',
        backgroundColor: '#f0f2f5',
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
    list: {
        backgroundColor: 'white',
        borderRadius: '12px',
        padding: '8px',
        boxShadow: '0 2px 8px rgba(0,0,0,0.1)'
    },
    item: {
        display: 'flex',
        alignItems: 'center',
        padding: '16px',
        marginBottom: '8px',
        backgroundColor: '#fff',
        border: '1px solid #e0e0e0',
        borderRadius: '8px',
        cursor: 'grab',
        transition: 'all 0.2s ease'
    },
    itemDragging: {
        opacity: '0.5',
        transform: 'scale(1.02)',
        boxShadow: '0 4px 12px rgba(0,0,0,0.15)'
    },
    itemDragOver: {
        borderColor: '#1890ff',
        backgroundColor: '#e6f7ff'
    },
    dragHandle: {
        marginRight: '12px',
        color: '#999',
        fontSize: '18px'
    },
    itemContent: {
        flex: 1
    },
    itemTitle: {
        fontWeight: 'bold',
        color: '#333',
        marginBottom: '4px'
    },
    itemDesc: {
        fontSize: '13px',
        color: '#666'
    },
    itemIndex: {
        width: '28px',
        height: '28px',
        borderRadius: '50%',
        backgroundColor: '#1890ff',
        color: 'white',
        display: 'flex',
        alignItems: 'center',
        justifyContent: 'center',
        fontSize: '14px',
        fontWeight: 'bold',
        marginLeft: '12px'
    },
    resetButton: {
        display: 'block',
        width: '100%',
        padding: '12px',
        marginTop: '16px',
        backgroundColor: '#1890ff',
        color: 'white',
        border: 'none',
        borderRadius: '8px',
        fontSize: '16px',
        cursor: 'pointer'
    }
};

// 初始数据
const initialItems = [
    { id: 1, title: '学习 JavaScript', desc: '掌握 ES6+ 新特性' },
    { id: 2, title: '学习 React/Preact', desc: '组件化开发思想' },
    { id: 3, title: '学习 CSS', desc: 'Flexbox 和 Grid 布局' },
    { id: 4, title: '学习 Node.js', desc: '服务端 JavaScript' },
    { id: 5, title: '学习 TypeScript', desc: '类型安全的 JavaScript' }
];

// 可排序列表项组件
function SortableItem({ item, index, onDragStart, onDragOver, onDrop, isDragging, isDragOver }) {
    const itemStyle = {
        ...styles.item,
        ...(isDragging ? styles.itemDragging : {}),
        ...(isDragOver ? styles.itemDragOver : {})
    };

    return h('div', {
        draggable: 'true',
        style: itemStyle,
        onDragStart: (e) => {
            e.dataTransfer.setData('text/plain', index.toString());
            e.dataTransfer.effectAllowed = 'move';
            onDragStart(index);
        },
        onDragOver: (e) => {
            e.preventDefault();
            e.dataTransfer.dropEffect = 'move';
            onDragOver(index);
        },
        onDragLeave: () => {
            // 可选：处理离开
        },
        onDrop: (e) => {
            e.preventDefault();
            const fromIndex = parseInt(e.dataTransfer.getData('text/plain'), 10);
            onDrop(fromIndex, index);
        },
        onDragEnd: () => {
            onDragStart(null);
        }
    }, [
        h('span', { style: styles.dragHandle }, '☰'),
        h('div', { style: styles.itemContent }, [
            h('div', { style: styles.itemTitle }, item.title),
            h('div', { style: styles.itemDesc }, item.desc)
        ]),
        h('div', { style: styles.itemIndex }, index + 1)
    ]);
}

// 主应用组件
function App() {
    const [items, setItems] = useState(initialItems);
    const [draggingIndex, setDraggingIndex] = useState(null);
    const [dragOverIndex, setDragOverIndex] = useState(null);

    const handleDragStart = (index) => {
        setDraggingIndex(index);
        setDragOverIndex(null);
    };

    const handleDragOver = (index) => {
        if (draggingIndex !== null && draggingIndex !== index) {
            setDragOverIndex(index);
        }
    };

    const handleDrop = (fromIndex, toIndex) => {
        if (fromIndex === toIndex) return;
        
        const newItems = [...items];
        const [movedItem] = newItems.splice(fromIndex, 1);
        newItems.splice(toIndex, 0, movedItem);
        
        setItems(newItems);
        setDraggingIndex(null);
        setDragOverIndex(null);
    };

    const resetOrder = () => {
        setItems([...initialItems]);
    };

    return h('div', { style: styles.app }, [
        h('h1', { style: styles.title }, '拖拽排序'),
        h('p', { style: styles.subtitle }, '拖动列表项来重新排序'),
        
        h('div', { style: styles.list },
            items.map((item, index) =>
                h(SortableItem, {
                    key: item.id,
                    item,
                    index,
                    onDragStart: handleDragStart,
                    onDragOver: handleDragOver,
                    onDrop: handleDrop,
                    isDragging: draggingIndex === index,
                    isDragOver: dragOverIndex === index
                })
            )
        ),
        
        h('button', { style: styles.resetButton, onClick: resetOrder }, '重置顺序')
    ]);
}

// 渲染应用
console.log('Starting Drag Sort Demo...');
render(h(App), document.body);
console.log('Drag Sort Demo rendered!');
