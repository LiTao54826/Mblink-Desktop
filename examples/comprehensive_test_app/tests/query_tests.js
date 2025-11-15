/**
 * @file query_tests.js
 * @brief 查询选择器测试
 */

describe('查询选择器', () => {
    
    // ========== querySelector ==========
    
    test('querySelector - 通过标签名查询', () => {
        const container = document.createElement('div');
        container.innerHTML = '<span>Hello</span><p>World</p>';
        
        const span = container.querySelector('span');
        
        assertNotNull(span, '应找到 span 元素');
        assertEqual(span.tagName.toLowerCase(), 'span', '标签名应为 span');
        assertEqual(span.textContent, 'Hello', '内容应为 Hello');
    });
    
    test('querySelector - 通过 class 查询', () => {
        const container = document.createElement('div');
        container.innerHTML = '<div class="test">Found</div><div>Not Found</div>';
        
        const element = container.querySelector('.test');
        
        assertNotNull(element, '应找到元素');
        assertEqual(element.textContent, 'Found', '内容应为 Found');
    });
    
    test('querySelector - 通过 ID 查询', () => {
        const container = document.createElement('div');
        container.innerHTML = '<div id="test">Found</div><div>Not Found</div>';
        
        const element = container.querySelector('#test');
        
        assertNotNull(element, '应找到元素');
        assertEqual(element.textContent, 'Found', '内容应为 Found');
    });
    
    test('querySelector - 复杂选择器', () => {
        const container = document.createElement('div');
        container.innerHTML = '<div class="container"><span class="text">Found</span></div>';
        
        const element = container.querySelector('.container .text');
        
        assertNotNull(element, '应找到元素');
        assertEqual(element.textContent, 'Found', '内容应为 Found');
    });
    
    test('querySelector - 未找到元素', () => {
        const container = document.createElement('div');
        container.innerHTML = '<div>Test</div>';
        
        const element = container.querySelector('.nonexistent');
        
        assertNull(element, '不应找到元素');
    });
    
    // ========== querySelectorAll ==========
    
    test('querySelectorAll - 查询所有匹配元素', () => {
        const container = document.createElement('div');
        container.innerHTML = '<span>1</span><span>2</span><span>3</span>';
        
        const elements = container.querySelectorAll('span');
        
        assertEqual(elements.length, 3, '应找到 3 个元素');
        assertEqual(elements[0].textContent, '1', '第 1 个元素内容应为 1');
        assertEqual(elements[1].textContent, '2', '第 2 个元素内容应为 2');
        assertEqual(elements[2].textContent, '3', '第 3 个元素内容应为 3');
    });
    
    test('querySelectorAll - 通过 class 查询', () => {
        const container = document.createElement('div');
        container.innerHTML = '<div class="item">1</div><div>2</div><div class="item">3</div>';
        
        const elements = container.querySelectorAll('.item');
        
        assertEqual(elements.length, 2, '应找到 2 个元素');
        assertEqual(elements[0].textContent, '1', '第 1 个元素内容应为 1');
        assertEqual(elements[1].textContent, '3', '第 2 个元素内容应为 3');
    });
    
    test('querySelectorAll - 未找到元素', () => {
        const container = document.createElement('div');
        container.innerHTML = '<div>Test</div>';
        
        const elements = container.querySelectorAll('.nonexistent');
        
        assertEqual(elements.length, 0, '不应找到元素');
    });
    
    // ========== getElementById ==========
    
    test('getElementById - 查询元素', () => {
        const element = document.createElement('div');
        element.id = 'test-id';
        document.body.appendChild(element);
        
        const found = document.getElementById('test-id');
        
        assertNotNull(found, '应找到元素');
        assertEqual(found.id, 'test-id', 'ID 应为 test-id');
        
        // 清理
        document.body.removeChild(element);
    });
    
    test('getElementById - 未找到元素', () => {
        const found = document.getElementById('nonexistent-id');
        
        assertNull(found, '不应找到元素');
    });
    
    // ========== getElementsByClassName ==========
    
    test('getElementsByClassName - 查询元素', () => {
        const container = document.createElement('div');
        container.innerHTML = '<div class="test">1</div><div>2</div><div class="test">3</div>';
        document.body.appendChild(container);
        
        const elements = document.getElementsByClassName('test');
        
        assertEqual(elements.length, 2, '应找到 2 个元素');
        
        // 清理
        document.body.removeChild(container);
    });
    
    // ========== getElementsByTagName ==========
    
    test('getElementsByTagName - 查询元素', () => {
        const container = document.createElement('div');
        container.innerHTML = '<span>1</span><p>2</p><span>3</span>';
        document.body.appendChild(container);
        
        const elements = document.getElementsByTagName('span');
        
        assertEqual(elements.length, 2, '应找到 2 个元素');
        
        // 清理
        document.body.removeChild(container);
    });
    
    // ========== matches ==========
    
    test('matches - 匹配标签名', () => {
        const element = document.createElement('div');
        
        assertTrue(element.matches('div'), '应匹配 div');
        assertFalse(element.matches('span'), '不应匹配 span');
    });
    
    test('matches - 匹配 class', () => {
        const element = document.createElement('div');
        element.className = 'container main';
        
        assertTrue(element.matches('.container'), '应匹配 .container');
        assertTrue(element.matches('.main'), '应匹配 .main');
        assertFalse(element.matches('.other'), '不应匹配 .other');
    });
    
    test('matches - 匹配 ID', () => {
        const element = document.createElement('div');
        element.id = 'test';
        
        assertTrue(element.matches('#test'), '应匹配 #test');
        assertFalse(element.matches('#other'), '不应匹配 #other');
    });
    
    test('matches - 复杂选择器', () => {
        const element = document.createElement('div');
        element.className = 'container';
        element.id = 'main';
        
        assertTrue(element.matches('div.container'), '应匹配 div.container');
        assertTrue(element.matches('div#main'), '应匹配 div#main');
        assertTrue(element.matches('#main.container'), '应匹配 #main.container');
    });
    
    // ========== closest ==========
    
    test('closest - 查找直接父元素', () => {
        const parent = document.createElement('div');
        parent.className = 'parent';
        const child = document.createElement('span');
        parent.appendChild(child);
        
        const found = child.closest('.parent');
        
        assertNotNull(found, '应找到父元素');
        assertEqual(found, parent, '应为 parent 元素');
    });
    
    test('closest - 查找祖先元素', () => {
        const grandparent = document.createElement('div');
        grandparent.className = 'grandparent';
        const parent = document.createElement('div');
        const child = document.createElement('span');
        
        grandparent.appendChild(parent);
        parent.appendChild(child);
        
        const found = child.closest('.grandparent');
        
        assertNotNull(found, '应找到祖先元素');
        assertEqual(found, grandparent, '应为 grandparent 元素');
    });
    
    test('closest - 查找自身', () => {
        const element = document.createElement('div');
        element.className = 'test';
        
        const found = element.closest('.test');
        
        assertNotNull(found, '应找到自身');
        assertEqual(found, element, '应为自身元素');
    });
    
    test('closest - 未找到元素', () => {
        const parent = document.createElement('div');
        const child = document.createElement('span');
        parent.appendChild(child);
        
        const found = child.closest('.nonexistent');
        
        assertNull(found, '不应找到元素');
    });
});

