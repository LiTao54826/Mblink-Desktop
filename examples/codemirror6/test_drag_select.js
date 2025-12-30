/**
 * @file test_drag_select.js
 * @brief 拦截 style 设置查看选择矩形的创建
 */

import { EditorView, basicSetup } from './codemirror6.bundle.js';

console.log('=== Selection Highlight Test ===');

const container = document.createElement('div');
container.style.cssText = 'width: 600px; height: 400px; background: #f0f0f0;';
document.body.appendChild(container);

const view = new EditorView({
    doc: `Hello World
Line 2 here
Line 3 text`,
    extensions: [basicSetup],
    parent: container
});

view.focus();

// 拦截 DOM 元素的 style 设置
setTimeout(() => {
    const observer = new MutationObserver((mutations) => {
        for (const mutation of mutations) {
            if (mutation.type === 'attributes' && mutation.attributeName === 'style') {
                const target = mutation.target;
                if (target.classList.contains('cm-selectionBackground')) {
                    const style = target.style;
                    console.log(`[SelectionBackground] left=${style.left}, top=${style.top}, width=${style.width}, height=${style.height}`);
                }
            }
        }
    });
    
    observer.observe(container, {
        attributes: true,
        attributeFilter: ['style'],
        subtree: true
    });
    
    // 设置全选
    console.log('\n=== Setting selection ===');
    view.dispatch({
        selection: { anchor: 0, head: 35 }
    });
    
    setTimeout(() => {
        observer.disconnect();
        console.log('\n=== Final check ===');
        const backgrounds = container.querySelectorAll('.cm-selectionBackground');
        console.log('Total backgrounds:', backgrounds.length);
        backgrounds.forEach((bg, i) => {
            console.log(`  [${i}] width=${bg.style.width}`);
        });
    }, 300);
}, 500);
