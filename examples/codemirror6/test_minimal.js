/**
 * @file test_minimal.js
 * @brief 最小化测试 - 不使用 basicSetup
 */

import { EditorView } from './codemirror6.bundle.js';

const container = document.createElement('div');
container.style.cssText = 'width: 600px; height: 400px;';
document.body.appendChild(container);

const view = new EditorView({
    doc: `Hello World
Line 2 here
Line 3 text`,
    parent: container
});

view.focus();

setTimeout(() => {
    console.log('=== Testing Select All (minimal) ===');
    
    view.dispatch({
        selection: { anchor: 0, head: 35 }
    });
    
    setTimeout(() => {
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
