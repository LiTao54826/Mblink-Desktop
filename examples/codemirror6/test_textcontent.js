/**
 * @file test_textcontent.js
 * @brief 测试 contentDOM 的 textContent 长度
 */

import { EditorView, basicSetup } from './codemirror6.bundle.js';

console.log('=== TextContent Test ===');

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
    console.log('\n=== contentDOM Analysis ===');
    console.log('textContent length:', contentDOM.textContent.length);
    console.log('textContent:', JSON.stringify(contentDOM.textContent));
    
    console.log('\n=== Children Analysis ===');
    for (let i = 0; i < contentDOM.children.length; i++) {
        const child = contentDOM.children[i];
        console.log(`Child ${i}:`, child.tagName, child.className);
        console.log(`  textContent:`, JSON.stringify(child.textContent));
        console.log(`  textContent.length:`, child.textContent.length);
    }
}, 500);
