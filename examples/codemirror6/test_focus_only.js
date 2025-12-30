/**
 * @file test_focus_only.js
 * @brief 测试只调用 focus() 的情况
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

console.log('=== Before focus ===');
console.log('selection.main:', `from=${view.state.selection.main.from}, to=${view.state.selection.main.to}`);

view.focus();

setTimeout(() => {
    console.log('\n=== After focus (500ms) ===');
    console.log('selection.main:', `from=${view.state.selection.main.from}, to=${view.state.selection.main.to}`);
    
    // 现在设置全选
    console.log('\n=== Setting Selection ===');
    view.dispatch({
        selection: { anchor: 0, head: 35 }
    });
    
    setTimeout(() => {
        console.log('\n=== After dispatch (200ms) ===');
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
}, 500);
