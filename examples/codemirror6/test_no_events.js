/**
 * @file test_no_events.js
 * @brief 测试不触发任何事件的情况下设置 Selection
 */

import { EditorView, basicSetup } from './codemirror6.bundle.js';

const container = document.createElement('div');
container.style.cssText = 'width: 600px; height: 400px;';
document.body.appendChild(container);

const view = new EditorView({
    doc: `Hello World
Line 2 here
Line 3 text`,
    extensions: [basicSetup],
    parent: container
});

// 不调用 focus()，避免触发事件

setTimeout(() => {
    console.log('=== Setting Selection (no focus) ===');
    
    view.dispatch({
        selection: { anchor: 0, head: 35 }
    });
    
    console.log('Dispatched');
    
    setTimeout(() => {
        console.log('\n=== After 500ms ===');
        console.log('selection.main:', `from=${view.state.selection.main.from}, to=${view.state.selection.main.to}`);
        
        const backgrounds = container.querySelectorAll('.cm-selectionBackground');
        console.log('Selection backgrounds:', backgrounds.length);
        backgrounds.forEach((bg, i) => {
            const width = parseFloat(bg.style.width);
            const status = width > 0 ? '✓' : '✗';
            console.log(`  [${i}] ${status} width=${bg.style.width}`);
        });
    }, 500);
}, 500);
