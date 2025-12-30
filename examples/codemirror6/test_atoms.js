/**
 * @file test_atoms.js
 * @brief 检查 atomicRanges
 */

import { EditorView, basicSetup } from './codemirror6.bundle.js';

console.log('=== Atomic Ranges Debug Test ===');

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

setTimeout(() => {
    // 检查 atomicRanges facet
    console.log('\n--- Checking atomicRanges ---');
    
    // 获取 atoms
    const atomicRangesFacet = view.state.facet;
    console.log('view.state.facet:', typeof atomicRangesFacet);
    
    // 尝试获取 inputState 中的 atoms
    console.log('view.inputState:', view.inputState);
    
    // 模拟 MouseSelection 构造函数中的 atoms 获取
    // this.atoms = view.state.facet(atomicRanges).map((f) => f(view));
    
    // 我们需要找到 atomicRanges facet
    // 在 bundle 中它是一个内部变量，我们无法直接访问
    
    // 让我们检查 view 的属性
    console.log('\nview properties:');
    for (const key of Object.keys(view)) {
        console.log('  ' + key + ':', typeof view[key]);
    }
    
    console.log('\nview.inputState properties:');
    for (const key of Object.keys(view.inputState)) {
        console.log('  ' + key + ':', typeof view.inputState[key]);
    }
    
    // 检查 mouseSelection
    if (view.inputState.mouseSelection) {
        console.log('\nmouseSelection.atoms:', view.inputState.mouseSelection.atoms);
    }
    
    // 手动测试 posAndSideAtCoords
    console.log('\n--- Manual test ---');
    const testCoords = { x: 100, y: 30 };
    const result = view.posAndSideAtCoords(testCoords, false);
    console.log('posAndSideAtCoords(' + JSON.stringify(testCoords) + '):', result);
    
    // 测试 EditorSelection.cursor
    console.log('\n--- Testing EditorSelection.cursor ---');
    // 我们需要访问 EditorSelection，但它是 bundle 内部的
    // 让我们通过 view.state.selection 来检查
    console.log('Current selection:', view.state.selection);
    console.log('  main.anchor:', view.state.selection.main.anchor);
    console.log('  main.head:', view.state.selection.main.head);
    
}, 500);

// 监听选择变化
let lastHead = -1;
setInterval(() => {
    const head = view.state.selection.main.head;
    if (head !== lastHead) {
        console.log('\n=== Selection changed to:', head, '===');
        lastHead = head;
    }
}, 100);

console.log('\nClick to test...');
