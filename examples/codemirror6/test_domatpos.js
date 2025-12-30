/**
 * @file test_domatpos.js
 * @brief 测试 CodeMirror 的 domAtPos 方法
 */

import { EditorView, basicSetup } from './codemirror6.bundle.js';

console.log('=== domAtPos Test ===');

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
    console.log('\n=== Testing domAtPos ===');
    
    // 测试每个位置
    for (let pos = 0; pos <= view.state.doc.length; pos++) {
        try {
            const domPos = view.domAtPos(pos);
            const nodeType = domPos.node.nodeType;
            const nodeValue = nodeType === 3 ? JSON.stringify(domPos.node.nodeValue) : domPos.node.nodeName;
            console.log(`pos ${pos}: node=${nodeValue}, offset=${domPos.offset}`);
        } catch (e) {
            console.log(`pos ${pos}: ERROR - ${e.message}`);
        }
    }
}, 500);
