/**
 * @file test_final.js
 * @brief 最终测试 - 检查全选是否正常
 */

import { EditorView, basicSetup } from './codemirror6.bundle.js';

const container = document.createElement('div');
container.style.cssText = 'width: 600px; height: 400px; background: #f0f0f0;';
document.body.appendChild(container);

const view = new EditorView({
    doc: `Hello World
Line 2 here
Line 3 text`,
    extensions: [basicSetup],
    parent: container
});

view.focus();

setTimeout(() => {
    console.log('=== Testing Select All ===');
    console.log('navigator.userAgent:', navigator.userAgent);
    
    // 检查初始 Selection
    console.log('\n=== Before dispatch ===');
    console.log('selection.main:', `from=${view.state.selection.main.from}, to=${view.state.selection.main.to}`);
    
    // 设置全选
    view.dispatch({
        selection: { anchor: 0, head: 35 }
    });
    
    // 立即检查 Selection 状态
    console.log('\n=== Immediately after dispatch ===');
    console.log('selection.main:', `from=${view.state.selection.main.from}, to=${view.state.selection.main.to}`);
    console.log('selection.ranges.length:', view.state.selection.ranges.length);
    view.state.selection.ranges.forEach((r, i) => {
        console.log(`  Range ${i}: from=${r.from}, to=${r.to}`);
    });
    
    setTimeout(() => {
        console.log('\n=== After 200ms ===');
        console.log('selection.main:', `from=${view.state.selection.main.from}, to=${view.state.selection.main.to}`);
        
        const backgrounds = container.querySelectorAll('.cm-selectionBackground');
        console.log('\nSelection backgrounds:', backgrounds.length);
        backgrounds.forEach((bg, i) => {
            const width = parseFloat(bg.style.width);
            const status = width > 0 ? '✓' : '✗';
            console.log(`  [${i}] ${status} width=${bg.style.width}`);
        });
        
        const allGood = Array.from(backgrounds).every(bg => parseFloat(bg.style.width) > 0);
        console.log('\n' + (allGood ? '✓ 全选正常！' : '✗ 全选仍有问题'));
    }, 200);
}, 500);
