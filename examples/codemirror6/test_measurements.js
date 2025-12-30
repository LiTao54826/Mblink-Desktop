/**
 * @file test_measurements.js
 * @brief 检查 DOM 测量是否正确
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
    console.log('=== DOM Measurements ===');
    
    // 检查 contentDOM 的尺寸
    const contentRect = view.contentDOM.getBoundingClientRect();
    console.log('contentDOM rect:', {
        top: contentRect.top,
        bottom: contentRect.bottom,
        height: contentRect.height
    });
    
    // 检查每一行的位置
    const lines = view.contentDOM.querySelectorAll('.cm-line');
    console.log('\nLines positions:');
    lines.forEach((line, i) => {
        const rect = line.getBoundingClientRect();
        console.log(`Line ${i}: top=${rect.top.toFixed(2)}, bottom=${rect.bottom.toFixed(2)}, height=${rect.height.toFixed(2)}`);
    });
    
    // 检查 viewport
    console.log('\nViewport:', {
        from: view.viewport.from,
        to: view.viewport.to
    });
    
    // 检查 visibleRanges
    console.log('\nvisibleRanges:');
    view.visibleRanges.forEach((r, i) => {
        console.log(`  [${i}] from=${r.from}, to=${r.to}`);
    });
    
    // 检查 lineBlockAt
    console.log('\nlineBlockAt:');
    for (let pos of [0, 12, 24, 35]) {
        const block = view.lineBlockAt(pos);
        if (block) {
            console.log(`pos ${pos}: from=${block.from}, to=${block.to}, top=${block.top.toFixed(2)}, bottom=${block.bottom.toFixed(2)}`);
        }
    }
    
    // 检查 scrollDOM 的尺寸
    const scrollRect = view.scrollDOM.getBoundingClientRect();
    console.log('\nscrollDOM rect:', {
        top: scrollRect.top,
        bottom: scrollRect.bottom,
        height: scrollRect.height
    });
    
    // 检查第三行是否在可见区域内
    const line3Rect = lines[2].getBoundingClientRect();
    const isVisible = line3Rect.bottom <= scrollRect.bottom && line3Rect.top >= scrollRect.top;
    console.log('\nLine 3 visible?', isVisible);
    console.log('  line3.bottom:', line3Rect.bottom.toFixed(2));
    console.log('  scrollDOM.bottom:', scrollRect.bottom.toFixed(2));
}, 500);
