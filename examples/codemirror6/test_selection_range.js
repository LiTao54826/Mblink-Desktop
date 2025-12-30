/**
 * @file test_selection_range.js
 * @brief 测试手动拖选后的 Selection 状态
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

view.focus();

console.log('=== Document Info ===');
console.log('Document length:', view.state.doc.length);
console.log('Document text:', JSON.stringify(view.state.doc.toString()));

// 等待用户手动拖选
console.log('\n请手动拖选所有文本，然后按 Ctrl+C 复制');
console.log('程序将每秒检查 Selection 状态...\n');

let checkCount = 0;
const interval = setInterval(() => {
    checkCount++;
    
    const sel = window.getSelection();
    if (sel && sel.rangeCount > 0) {
        const range = sel.getRangeAt(0);
        
        console.log(`[Check ${checkCount}] DOM Selection:`);
        console.log('  startContainer:', range.startContainer.nodeName, 'offset:', range.startOffset);
        console.log('  endContainer:', range.endContainer.nodeName, 'offset:', range.endOffset);
        
        const selectedText = range.toString();
        console.log('  Selected text length:', selectedText.length);
        console.log('  Selected text:', JSON.stringify(selectedText.substring(0, 50) + (selectedText.length > 50 ? '...' : '')));
        
        // 检查 CodeMirror 的 Selection
        console.log('  CodeMirror selection:', `from=${view.state.selection.main.from}, to=${view.state.selection.main.to}`);
        console.log('');
    }
    
    if (checkCount >= 20) {
        clearInterval(interval);
        console.log('测试结束');
    }
}, 1000);
