/**
 * @file test_selectall_direct.js
 * @brief 直接测试 CodeMirror 的 selectAll 命令
 */

import { EditorView, basicSetup } from './codemirror6.bundle.js';

console.log('=== Direct selectAll Test ===');

const container = document.createElement('div');
container.style.cssText = 'width: 600px; height: 400px;';
document.body.appendChild(container);

const view = new EditorView({
    doc: `Line 1
Line 2
Line 3`,
    extensions: [basicSetup],
    parent: container
});

console.log('Document length:', view.state.doc.length);
console.log('Document text:', JSON.stringify(view.state.doc.toString()));

view.focus();

setTimeout(() => {
    console.log('\n=== Before selectAll ===');
    console.log('Selection:', `from=${view.state.selection.main.from}, to=${view.state.selection.main.to}`);
    
    // 直接调用 selectAll 命令
    console.log('\n=== Calling selectAll command directly ===');
    view.dispatch(view.state.update({ 
        selection: { anchor: 0, head: view.state.doc.length }, 
        userEvent: "select" 
    }));
    
    setTimeout(() => {
        console.log('\n=== After selectAll ===');
        console.log('Selection:', `from=${view.state.selection.main.from}, to=${view.state.selection.main.to}`);
        console.log('Expected:', `from=0, to=${view.state.doc.length}`);
        
        if (view.state.selection.main.from === 0 && view.state.selection.main.to === view.state.doc.length) {
            console.log('✓ selectAll works correctly!');
        } else {
            console.log('✗ selectAll failed - selection range is incorrect');
        }
        
        // 检查 DOM Selection
        const domSel = window.getSelection();
        console.log('\n=== DOM Selection ===');
        console.log('rangeCount:', domSel.rangeCount);
        if (domSel.rangeCount > 0) {
            const range = domSel.getRangeAt(0);
            console.log('DOM range:', range.toString().length, 'chars');
            console.log('DOM text:', JSON.stringify(range.toString()));
        }
    }, 100);
}, 500);
