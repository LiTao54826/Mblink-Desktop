/**
 * @file test_dom_structure.js
 * @brief 检查 CodeMirror 的 DOM 结构
 */

import { EditorView, basicSetup } from './codemirror6.bundle.js';

console.log('=== DOM Structure Test ===');

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

setTimeout(() => {
    const contentDOM = view.contentDOM;
    console.log('\n=== contentDOM Structure ===');
    console.log('contentDOM:', contentDOM.tagName, contentDOM.className);
    console.log('children count:', contentDOM.childNodes.length);
    
    for (let i = 0; i < contentDOM.childNodes.length; i++) {
        const child = contentDOM.childNodes[i];
        console.log(`\nChild ${i}:`);
        console.log('  nodeType:', child.nodeType, child.nodeType === 1 ? '(ELEMENT)' : child.nodeType === 3 ? '(TEXT)' : '');
        console.log('  nodeName:', child.nodeName);
        if (child.nodeType === 1) {
            console.log('  className:', child.className);
            console.log('  childNodes count:', child.childNodes.length);
            for (let j = 0; j < child.childNodes.length; j++) {
                const grandchild = child.childNodes[j];
                console.log(`    Grandchild ${j}:`);
                console.log('      nodeType:', grandchild.nodeType, grandchild.nodeType === 3 ? '(TEXT)' : '');
                console.log('      nodeName:', grandchild.nodeName);
                if (grandchild.nodeType === 3) {
                    console.log('      nodeValue:', JSON.stringify(grandchild.nodeValue));
                    console.log('      length:', grandchild.nodeValue ? grandchild.nodeValue.length : 0);
                }
            }
        }
    }
}, 500);
