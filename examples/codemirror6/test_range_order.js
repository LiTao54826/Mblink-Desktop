/**
 * @file test_range_order.js
 * @brief 测试 Range.setStart/setEnd 的顺序是否影响结果
 */

import { EditorView, basicSetup } from './codemirror6.bundle.js';

const container = document.createElement('div');
container.style.cssText = 'width: 600px; height: 400px;';
document.body.appendChild(container);

const view = new EditorView({
    doc: `Line 3 text`,
    extensions: [basicSetup],
    parent: container
});

setTimeout(() => {
    const lines = container.querySelectorAll('.cm-line');
    if (lines.length > 0) {
        const textNode = lines[0].firstChild;
        if (textNode && textNode.nodeType === 3) {
            console.log('Text node:', JSON.stringify(textNode.textContent));
            
            // 测试1: 先setStart，再setEnd（正常顺序）
            console.log('\n=== Test 1: setStart then setEnd ===');
            const range1 = document.createRange();
            range1.setStart(textNode, 10);
            range1.setEnd(textNode, 11);
            const rects1 = range1.getClientRects();
            console.log(`Range(10, 11): ${rects1.length} rects`);
            if (rects1.length > 0) {
                console.log(`  left=${rects1[0].left.toFixed(2)}, right=${rects1[0].right.toFixed(2)}, width=${rects1[0].width.toFixed(2)}`);
            }
            
            // 测试2: 先setEnd，再setStart（CodeMirror的顺序）
            console.log('\n=== Test 2: setEnd then setStart ===');
            const range2 = document.createRange();
            range2.setEnd(textNode, 11);
            range2.setStart(textNode, 10);
            const rects2 = range2.getClientRects();
            console.log(`Range(10, 11): ${rects2.length} rects`);
            if (rects2.length > 0) {
                console.log(`  left=${rects2[0].left.toFixed(2)}, right=${rects2[0].right.toFixed(2)}, width=${rects2[0].width.toFixed(2)}`);
            }
            
            // 测试3: 重用Range对象
            console.log('\n=== Test 3: Reuse range object ===');
            const range3 = document.createRange();
            // 第一次使用
            range3.setEnd(textNode, 1);
            range3.setStart(textNode, 0);
            const rects3a = range3.getClientRects();
            console.log(`First use Range(0, 1): width=${rects3a[0]?.width.toFixed(2)}`);
            // 第二次使用（重用）
            range3.setEnd(textNode, 11);
            range3.setStart(textNode, 10);
            const rects3b = range3.getClientRects();
            console.log(`Second use Range(10, 11): width=${rects3b[0]?.width.toFixed(2)}`);
        }
    }
}, 500);
