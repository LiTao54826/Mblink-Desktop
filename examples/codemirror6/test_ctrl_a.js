/**
 * @file test_ctrl_a.js
 * @brief 测试 Ctrl+A 选择全部功能
 */

import { EditorView, basicSetup } from './codemirror6.bundle.js';

console.log('=== Ctrl+A Test ===');

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
    console.log('\n=== Before Ctrl+A ===');
    console.log('Selection:', `from=${view.state.selection.main.from}, to=${view.state.selection.main.to}`);
    
    // 模拟 Ctrl+A
    console.log('\n=== Simulating Ctrl+A ===');
    const event = new KeyboardEvent('keydown', {
        key: 'a',
        code: 'KeyA',
        keyCode: 65,
        ctrlKey: true,
        bubbles: true,
        cancelable: true
    });
    view.contentDOM.dispatchEvent(event);
    
    setTimeout(() => {
        console.log('\n=== After Ctrl+A ===');
        console.log('Selection:', `from=${view.state.selection.main.from}, to=${view.state.selection.main.to}`);
        console.log('Expected:', `from=0, to=${view.state.doc.length}`);
        
        if (view.state.selection.main.from === 0 && view.state.selection.main.to === view.state.doc.length) {
            console.log('✓ Ctrl+A works correctly!');
        } else {
            console.log('✗ Ctrl+A failed - selection range is incorrect');
        }
        
        // 测试复制
        console.log('\n=== Testing Copy ===');
        const selectedText = view.state.doc.sliceString(
            view.state.selection.main.from,
            view.state.selection.main.to
        );
        console.log('Selected text length:', selectedText.length);
        console.log('Selected text:', JSON.stringify(selectedText));
    }, 100);
}, 500);
