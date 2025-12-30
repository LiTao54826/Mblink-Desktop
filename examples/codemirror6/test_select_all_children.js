/**
 * @file test_select_all_children.js
 * @brief 测试 SelectAllChildren 的行为
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

setTimeout(() => {
    console.log('=== Testing SelectAllChildren ===');
    
    // 获取 contentDOM
    const contentDOM = view.contentDOM;
    console.log('contentDOM:', contentDOM.tagName, contentDOM.className);
    
    // 列出所有子节点
    console.log('\ncontentDOM children:');
    const children = contentDOM.childNodes;
    for (let i = 0; i < children.length; i++) {
        const child = children[i];
        console.log(`  [${i}] ${child.nodeName} (${child.nodeType})`);
        if (child.nodeType === 1) { // Element
            console.log(`      className: ${child.className}`);
            console.log(`      childNodes: ${child.childNodes.length}`);
            if (child.childNodes.length > 0) {
                for (let j = 0; j < child.childNodes.length; j++) {
                    const grandchild = child.childNodes[j];
                    console.log(`        [${j}] ${grandchild.nodeName} (${grandchild.nodeType})`);
                    if (grandchild.nodeType === 3) { // Text
                        console.log(`            text: "${grandchild.textContent}"`);
                    }
                }
            }
        }
    }
    
    // 调用 selectAllChildren
    console.log('\n=== Calling selectAllChildren ===');
    const sel = window.getSelection();
    sel.selectAllChildren(contentDOM);
    
    setTimeout(() => {
        console.log('\n=== After selectAllChildren ===');
        console.log('Selection rangeCount:', sel.rangeCount);
        
        // 直接读取 selection 的属性
        console.log('\nSelection properties:');
        console.log('  anchorNode:', sel.anchorNode ? sel.anchorNode.nodeName : 'null');
        console.log('  anchorOffset:', sel.anchorOffset);
        console.log('  focusNode:', sel.focusNode ? sel.focusNode.nodeName : 'null');
        console.log('  focusOffset:', sel.focusOffset);
        console.log('  isCollapsed:', sel.isCollapsed);
        console.log('  type:', sel.type);
        console.log('  direction:', sel.direction);
        
        if (sel.anchorNode && sel.anchorNode.nodeType === 3) {
            console.log('  anchorNode text:', JSON.stringify(sel.anchorNode.textContent));
        }
        if (sel.focusNode && sel.focusNode.nodeType === 3) {
            console.log('  focusNode text:', JSON.stringify(sel.focusNode.textContent));
        }
        
        if (sel.rangeCount > 0) {
            const range = sel.getRangeAt(0);
            console.log('\nRange:');
            console.log('  startContainer:', range.startContainer.nodeName, 'offset:', range.startOffset);
            console.log('  endContainer:', range.endContainer.nodeName, 'offset:', range.endOffset);
            
            const selectedText = range.toString();
            console.log('  Selected text length:', selectedText.length);
            console.log('  Selected text:', JSON.stringify(selectedText));
        }
        
        console.log('\nCodeMirror selection:', `from=${view.state.selection.main.from}, to=${view.state.selection.main.to}`);
    }, 100);
}, 500);
