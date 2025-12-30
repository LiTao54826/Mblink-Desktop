/**
 * @file test_dom_selection.js
 * @brief 测试 DOM Selection 在 Ctrl+A 后的状态
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

// 监听 selectionchange 事件
document.addEventListener('selectionchange', () => {
    const sel = window.getSelection();
    console.log('[selectionchange] DOM Selection:', {
        rangeCount: sel.rangeCount,
        isCollapsed: sel.isCollapsed
    });
    if (sel.rangeCount > 0) {
        const range = sel.getRangeAt(0);
        console.log('  Range:', {
            startContainer: range.startContainer.nodeName,
            startOffset: range.startOffset,
            endContainer: range.endContainer.nodeName,
            endOffset: range.endOffset
        });
    }
});

setTimeout(() => {
    console.log('\n=== Before Ctrl+A ===');
    const sel1 = window.getSelection();
    console.log('DOM Selection rangeCount:', sel1.rangeCount);
    
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
        console.log('\n=== After Ctrl+A (100ms) ===');
        const sel2 = window.getSelection();
        console.log('DOM Selection:', {
            rangeCount: sel2.rangeCount,
            isCollapsed: sel2.isCollapsed
        });
        if (sel2.rangeCount > 0) {
            const range = sel2.getRangeAt(0);
            console.log('Range:', {
                startContainer: range.startContainer.nodeName,
                startOffset: range.startOffset,
                endContainer: range.endContainer.nodeName,
                endOffset: range.endOffset
            });
            
            // 计算选择的文本长度
            const selectedText = range.toString();
            console.log('Selected text length:', selectedText.length);
            console.log('Selected text:', JSON.stringify(selectedText));
        }
        
        console.log('\n=== CodeMirror Selection ===');
        console.log('view.state.selection.main:', {
            from: view.state.selection.main.from,
            to: view.state.selection.main.to
        });
    }, 100);
}, 500);
