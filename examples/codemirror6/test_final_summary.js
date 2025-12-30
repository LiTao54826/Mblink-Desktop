/**
 * @file test_final_summary.js
 * @brief 最终测试总结
 */

import { EditorView, basicSetup } from './codemirror6.bundle.js';

console.log('=== Final Summary Test ===\n');

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

setTimeout(() => {
    const contentDOM = view.contentDOM;
    
    console.log('\n=== DOM Analysis ===');
    console.log('contentDOM.textContent:', JSON.stringify(contentDOM.textContent));
    console.log('contentDOM.textContent.length:', contentDOM.textContent.length);
    
    console.log('\n=== Manual Selection Test ===');
    const sel = window.getSelection();
    sel.selectAllChildren(contentDOM);
    
    setTimeout(() => {
        if (sel.rangeCount > 0) {
            const range = sel.getRangeAt(0);
            const selectedText = range.toString();
            console.log('Range.toString():', JSON.stringify(selectedText));
            console.log('Range.toString().length:', selectedText.length);
            console.log('Expected length:', view.state.doc.length);
            console.log('Match:', selectedText.length === view.state.doc.length ? '✓ PASS' : '✗ FAIL');
        }
    }, 100);
}, 500);
