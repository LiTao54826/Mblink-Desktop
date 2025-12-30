/**
 * @file test_fix_selection.js
 * @brief 尝试修复 Selection 问题
 */

import { EditorView, basicSetup } from './codemirror6.bundle.js';

console.log('=== Selection Fix Test ===');

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

// 在 capture phase 拦截 mousedown，手动设置光标
container.addEventListener('mousedown', (e) => {
    // 使用 posAtCoords 获取正确的位置
    const pos = view.posAtCoords({ x: e.clientX, y: e.clientY });
    console.log('\n=== MOUSEDOWN ===');
    console.log('posAtCoords:', pos);
    
    if (pos !== null) {
        // 阻止 CodeMirror 的默认处理
        e.preventDefault();
        e.stopPropagation();
        
        // 手动设置光标位置 - 使用 view.state.update
        view.dispatch({
            selection: { anchor: pos, head: pos }
        });
        
        console.log('Manually set cursor to:', pos);
        
        // 确保编辑器获得焦点
        view.focus();
    }
}, true);  // capture phase

// 验证结果
container.addEventListener('click', (e) => {
    setTimeout(() => {
        const cmSel = view.state.selection.main;
        console.log('FINAL CM cursor - anchor:', cmSel.anchor, 'head:', cmSel.head);
    }, 10);
}, false);

console.log('\nClick on editor to test...');
console.log('Expected positions:');
console.log('  Line 1: 0-11 (Hello World)');
console.log('  Line 2: 12-23 (Line 2 here)');
console.log('  Line 3: 24-35 (Line 3 text)');
