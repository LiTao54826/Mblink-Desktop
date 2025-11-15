/**
 * @file dom_tests.js
 * @brief DOM 操作测试
 */

describe('DOM 操作', () => {
    
    // ========== appendChild ==========
    
    test('appendChild - 添加单个子节点', () => {
        const parent = document.createElement('div');
        const child = document.createElement('span');
        
        parent.appendChild(child);
        
        assertEqual(parent.childNodes.length, 1, '子节点数量应为 1');
        assertEqual(parent.firstChild, child, '第一个子节点应为 child');
        assertEqual(child.parentNode, parent, 'child 的父节点应为 parent');
    });
    
    test('appendChild - 添加多个子节点', () => {
        const parent = document.createElement('div');
        const child1 = document.createElement('span');
        const child2 = document.createElement('p');
        const child3 = document.createElement('a');
        
        parent.appendChild(child1);
        parent.appendChild(child2);
        parent.appendChild(child3);
        
        assertEqual(parent.childNodes.length, 3, '子节点数量应为 3');
        assertEqual(parent.firstChild, child1, '第一个子节点应为 child1');
        assertEqual(parent.lastChild, child3, '最后一个子节点应为 child3');
    });
    
    test('appendChild - 移动已存在的节点', () => {
        const parent1 = document.createElement('div');
        const parent2 = document.createElement('div');
        const child = document.createElement('span');
        
        parent1.appendChild(child);
        parent2.appendChild(child);
        
        assertEqual(parent1.childNodes.length, 0, 'parent1 应该没有子节点');
        assertEqual(parent2.childNodes.length, 1, 'parent2 应该有 1 个子节点');
        assertEqual(child.parentNode, parent2, 'child 的父节点应为 parent2');
    });
    
    // ========== removeChild ==========
    
    test('removeChild - 移除子节点', () => {
        const parent = document.createElement('div');
        const child = document.createElement('span');
        
        parent.appendChild(child);
        const removed = parent.removeChild(child);
        
        assertEqual(parent.childNodes.length, 0, '子节点数量应为 0');
        assertEqual(removed, child, '返回值应为被移除的节点');
        assertNull(child.parentNode, 'child 的父节点应为 null');
    });
    
    test('removeChild - 移除中间节点', () => {
        const parent = document.createElement('div');
        const child1 = document.createElement('span');
        const child2 = document.createElement('p');
        const child3 = document.createElement('a');
        
        parent.appendChild(child1);
        parent.appendChild(child2);
        parent.appendChild(child3);
        
        parent.removeChild(child2);
        
        assertEqual(parent.childNodes.length, 2, '子节点数量应为 2');
        assertEqual(parent.firstChild, child1, '第一个子节点应为 child1');
        assertEqual(parent.lastChild, child3, '最后一个子节点应为 child3');
    });
    
    // ========== insertBefore ==========
    
    test('insertBefore - 在第一个节点前插入', () => {
        const parent = document.createElement('div');
        const child1 = document.createElement('span');
        const child2 = document.createElement('p');
        
        parent.appendChild(child1);
        parent.insertBefore(child2, child1);
        
        assertEqual(parent.childNodes.length, 2, '子节点数量应为 2');
        assertEqual(parent.firstChild, child2, '第一个子节点应为 child2');
        assertEqual(parent.lastChild, child1, '最后一个子节点应为 child1');
    });
    
    test('insertBefore - 在中间节点前插入', () => {
        const parent = document.createElement('div');
        const child1 = document.createElement('span');
        const child2 = document.createElement('p');
        const child3 = document.createElement('a');
        
        parent.appendChild(child1);
        parent.appendChild(child3);
        parent.insertBefore(child2, child3);
        
        assertEqual(parent.childNodes.length, 3, '子节点数量应为 3');
        assertEqual(parent.childNodes[0], child1, '第 1 个子节点应为 child1');
        assertEqual(parent.childNodes[1], child2, '第 2 个子节点应为 child2');
        assertEqual(parent.childNodes[2], child3, '第 3 个子节点应为 child3');
    });
    
    test('insertBefore - 参考节点为 null（等同于 appendChild）', () => {
        const parent = document.createElement('div');
        const child1 = document.createElement('span');
        const child2 = document.createElement('p');
        
        parent.appendChild(child1);
        parent.insertBefore(child2, null);
        
        assertEqual(parent.childNodes.length, 2, '子节点数量应为 2');
        assertEqual(parent.lastChild, child2, '最后一个子节点应为 child2');
    });
    
    // ========== replaceChild ==========
    
    test('replaceChild - 替换子节点', () => {
        const parent = document.createElement('div');
        const oldChild = document.createElement('span');
        const newChild = document.createElement('p');
        
        parent.appendChild(oldChild);
        const replaced = parent.replaceChild(newChild, oldChild);
        
        assertEqual(parent.childNodes.length, 1, '子节点数量应为 1');
        assertEqual(parent.firstChild, newChild, '第一个子节点应为 newChild');
        assertEqual(replaced, oldChild, '返回值应为被替换的节点');
        assertNull(oldChild.parentNode, 'oldChild 的父节点应为 null');
    });
    
    test('replaceChild - 替换中间节点', () => {
        const parent = document.createElement('div');
        const child1 = document.createElement('span');
        const child2 = document.createElement('p');
        const child3 = document.createElement('a');
        const newChild = document.createElement('div');
        
        parent.appendChild(child1);
        parent.appendChild(child2);
        parent.appendChild(child3);
        
        parent.replaceChild(newChild, child2);
        
        assertEqual(parent.childNodes.length, 3, '子节点数量应为 3');
        assertEqual(parent.childNodes[0], child1, '第 1 个子节点应为 child1');
        assertEqual(parent.childNodes[1], newChild, '第 2 个子节点应为 newChild');
        assertEqual(parent.childNodes[2], child3, '第 3 个子节点应为 child3');
    });
    
    // ========== cloneNode ==========
    
    test('cloneNode - 浅克隆（不包含子节点）', () => {
        const original = document.createElement('div');
        original.id = 'test';
        original.className = 'container';
        original.setAttribute('data-value', '123');
        
        const child = document.createElement('span');
        original.appendChild(child);
        
        const clone = original.cloneNode(false);
        
        assertEqual(clone.id, 'test', 'ID 应被克隆');
        assertEqual(clone.className, 'container', 'className 应被克隆');
        assertEqual(clone.getAttribute('data-value'), '123', '属性应被克隆');
        assertEqual(clone.childNodes.length, 0, '不应包含子节点');
    });
    
    test('cloneNode - 深克隆（包含子节点）', () => {
        const original = document.createElement('div');
        original.id = 'test';
        original.innerHTML = '<span>Hello</span><p>World</p>';
        
        const clone = original.cloneNode(true);
        
        assertEqual(clone.id, 'test', 'ID 应被克隆');
        assertEqual(clone.childNodes.length, 2, '应包含 2 个子节点');
        assertEqual(clone.innerHTML, '<span>Hello</span><p>World</p>', 'innerHTML 应相同');
    });
    
    // ========== contains ==========
    
    test('contains - 包含直接子节点', () => {
        const parent = document.createElement('div');
        const child = document.createElement('span');
        
        parent.appendChild(child);
        
        assertTrue(parent.contains(child), 'parent 应包含 child');
    });
    
    test('contains - 包含后代节点', () => {
        const grandparent = document.createElement('div');
        const parent = document.createElement('div');
        const child = document.createElement('span');
        
        grandparent.appendChild(parent);
        parent.appendChild(child);
        
        assertTrue(grandparent.contains(child), 'grandparent 应包含 child');
    });
    
    test('contains - 不包含无关节点', () => {
        const parent = document.createElement('div');
        const other = document.createElement('span');
        
        assertFalse(parent.contains(other), 'parent 不应包含 other');
    });
    
    // ========== hasChildNodes ==========
    
    test('hasChildNodes - 有子节点', () => {
        const parent = document.createElement('div');
        const child = document.createElement('span');
        
        parent.appendChild(child);
        
        assertTrue(parent.hasChildNodes(), '应有子节点');
    });
    
    test('hasChildNodes - 无子节点', () => {
        const parent = document.createElement('div');
        
        assertFalse(parent.hasChildNodes(), '不应有子节点');
    });
    
    // ========== 节点遍历 ==========
    
    test('节点遍历 - firstChild/lastChild', () => {
        const parent = document.createElement('div');
        const child1 = document.createElement('span');
        const child2 = document.createElement('p');
        const child3 = document.createElement('a');
        
        parent.appendChild(child1);
        parent.appendChild(child2);
        parent.appendChild(child3);
        
        assertEqual(parent.firstChild, child1, 'firstChild 应为 child1');
        assertEqual(parent.lastChild, child3, 'lastChild 应为 child3');
    });
    
    test('节点遍历 - nextSibling/previousSibling', () => {
        const parent = document.createElement('div');
        const child1 = document.createElement('span');
        const child2 = document.createElement('p');
        const child3 = document.createElement('a');
        
        parent.appendChild(child1);
        parent.appendChild(child2);
        parent.appendChild(child3);
        
        assertEqual(child1.nextSibling, child2, 'child1.nextSibling 应为 child2');
        assertEqual(child2.nextSibling, child3, 'child2.nextSibling 应为 child3');
        assertEqual(child3.previousSibling, child2, 'child3.previousSibling 应为 child2');
        assertEqual(child2.previousSibling, child1, 'child2.previousSibling 应为 child1');
    });
});

