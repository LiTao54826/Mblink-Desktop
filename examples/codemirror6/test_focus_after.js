/**
 * @file test_focus_after.js
 * @brief 先设置 Selection，再调用 focus()
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

setTimeout(() => {
    console.log('=== Setting Selection (before focus) ===');
    
    view.dispatch({
        selection: { anchor: 0, head: 35 }
    });
    
    console.log('selection.main:', `from=${view.state.selection.main.from}, to=${view.state.selection.main.to}`);
    
    // 延迟 focus
    setTimeout(() => {
        console.log('\n=== Calling focus() ===');
        view.focus();
        
        setTimeout(() => {
            console.log('\n=== After focus (200ms) ===');
            console.log('selection.main:', `from=${view.state.selection.main.from}, to=${view.state.selection.main.to}`);
            
            const backgrounds = container.querySelectorAll('.cm-selectionBackground');
            console.log('Selection backgrounds:', backgrounds.length);
            backgrounds.forEach((bg, i) => {
                const width = parseFloat(bg.style.width);
                const status = width > 0 ? '✓' : '✗';
                console.log(`  [${i}] ${status} width=${bg.style.width}`);
            });
            
            const allGood = Array.from(backgrounds).every(bg => parseFloat(bg.style.width) > 0);
            console.log('\n' + (allGood ? '✓ 全选正常！' : '✗ 全选仍有问题'));
        }, 200);
    }, 100);
}, 500);
