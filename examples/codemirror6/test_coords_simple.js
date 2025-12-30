/**
 * @file test_coords_simple.js
 * @brief 简化的 coordsAtPos 测试 - 添加 textRange 调试
 */

import { EditorView, basicSetup } from './codemirror6.bundle.js';

// 拦截 textRange 调用
const originalCreateRange = document.createRange;
let rangeCallCount = 0;
document.createRange = function() {
    const range = originalCreateRange.call(document);
    const originalSetStart = range.setStart;
    const originalSetEnd = range.setEnd;
    const originalGetClientRects = range.getClientRects;
    
    let startNode, startOffset, endNode, endOffset;
    
    range.setStart = function(node, offset) {
        startNode = node;
        startOffset = offset;
        return originalSetStart.call(this, node, offset);
    };
    
    range.setEnd = function(node, offset) {
        endNode = node;
        endOffset = offset;
        return originalSetEnd.call(this, node, offset);
    };
    
    range.getClientRects = function() {
        const rects = originalGetClientRects.call(this);
        if (rangeCallCount < 30) {  // 只记录前30次调用
            console.log(`[textRange #${rangeCallCount}] setStart(${startOffset}), setEnd(${endOffset}), rects=${rects.length}, width=${rects[0]?.width.toFixed(2)}`);
            rangeCallCount++;
        }
        return rects;
    };
    
    return range;
};

const container = document.createElement('div');
container.style.cssText = 'width: 600px; height: 400px;';
document.body.appendChild(container);

const view = new EditorView({
    doc: `Line 3 text`,
    extensions: [basicSetup],
    parent: container
});

setTimeout(() => {
    console.log('\n=== Testing coordsAtPos(11, -2) ===');
    rangeCallCount = 0;
    const coords = view.coordsAtPos(11, -2);
    console.log(`Result: left=${coords.left.toFixed(2)}, right=${coords.right.toFixed(2)}, width=${(coords.right - coords.left).toFixed(2)}`);
}, 500);
