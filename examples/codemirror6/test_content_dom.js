/**
 * @file test_content_dom.js
 * @brief 检查 contentDOM 的内容
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
    console.log('=== contentDOM Info ===');
    console.log('contentDOM.textContent:', JSON.stringify(view.contentDOM.textContent));
    console.log('contentDOM.textContent.length:', view.contentDOM.textContent.length);
    console.log('contentDOM.innerText:', JSON.stringify(view.contentDOM.innerText));
    
    console.log('\n=== Document Info ===');
    console.log('view.state.doc.length:', view.state.doc.length);
    console.log('view.state.doc.toString():', JSON.stringify(view.state.doc.toString()));
    
    console.log('\n=== DOM Structure ===');
    const lines = view.contentDOM.querySelectorAll('.cm-line');
    console.log('Lines count:', lines.length);
    lines.forEach((line, i) => {
        console.log(`Line ${i}:`, JSON.stringify(line.textContent));
    });
}, 500);
