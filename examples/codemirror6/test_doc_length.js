/**
 * @file test_doc_length.js
 * @brief 检查文档长度和内容
 */

import { EditorView, basicSetup } from './codemirror6.bundle.js';

const container = document.createElement('div');
container.style.cssText = 'width: 600px; height: 400px;';
document.body.appendChild(container);

const docText = `Hello World
Line 2 here
Line 3 text`;

console.log('Input text:', JSON.stringify(docText));
console.log('Input length:', docText.length);

const view = new EditorView({
    doc: docText,
    extensions: [basicSetup],
    parent: container
});

setTimeout(() => {
    console.log('\n=== Document Info ===');
    console.log('view.state.doc.length:', view.state.doc.length);
    console.log('view.state.doc.toString():', JSON.stringify(view.state.doc.toString()));
    console.log('view.state.doc.lines:', view.state.doc.lines);
    
    console.log('\n=== Line Info ===');
    for (let i = 1; i <= view.state.doc.lines; i++) {
        const line = view.state.doc.line(i);
        console.log(`Line ${i}: from=${line.from}, to=${line.to}, text="${line.text}"`);
    }
    
    console.log('\n=== DOM Content ===');
    const lines = container.querySelectorAll('.cm-line');
    console.log('DOM lines count:', lines.length);
    lines.forEach((line, i) => {
        console.log(`DOM line ${i}: "${line.textContent}"`);
    });
}, 500);
