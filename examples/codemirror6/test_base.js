import { EditorView, basicSetup } from './codemirror6.bundle.js';

console.log('=== Base Test ===');

const container = document.createElement('div');
container.style.cssText = 'width: 600px; height: 400px;';
document.body.appendChild(container);

const view = new EditorView({
    doc: `Line 1: Hello World`,
    extensions: [basicSetup],
    parent: container
});

view.focus();

setTimeout(() => {
    // 获取 scrollDOM 的 rect
    const scrollRect = view.scrollDOM.getBoundingClientRect();
    console.log('scrollDOM rect:', JSON.stringify(scrollRect));
    
    // 计算 base
    const baseLeft = scrollRect.left - view.scrollDOM.scrollLeft;
    const baseTop = scrollRect.top - view.scrollDOM.scrollTop;
    console.log('base left:', baseLeft, 'top:', baseTop);
    
    // 获取 pos 0 的坐标
    const coords0 = view.coordsAtPos(0);
    console.log('coordsAtPos(0):', coords0 ? JSON.stringify(coords0) : 'null');
    
    // 计算光标位置
    if (coords0) {
        const cursorLeft = coords0.left - baseLeft;
        const cursorTop = coords0.top - baseTop;
        console.log('calculated cursor left:', cursorLeft, 'top:', cursorTop);
    }
    
    // 获取 pos 5 的坐标
    const coords5 = view.coordsAtPos(5);
    console.log('coordsAtPos(5):', coords5 ? JSON.stringify(coords5) : 'null');
    
    if (coords5) {
        const cursorLeft = coords5.left - baseLeft;
        const cursorTop = coords5.top - baseTop;
        console.log('calculated cursor left for pos 5:', cursorLeft, 'top:', cursorTop);
    }
    
    // 设置选择到 pos 5 并检查光标
    view.dispatch({ selection: { anchor: 5 } });
    
    setTimeout(() => {
        const cursor = container.querySelector('.cm-cursor');
        if (cursor) {
            console.log('\nActual cursor style:');
            console.log('  left:', cursor.style.left);
            console.log('  top:', cursor.style.top);
            console.log('  rect:', JSON.stringify(cursor.getBoundingClientRect()));
        }
    }, 100);
    
}, 300);
