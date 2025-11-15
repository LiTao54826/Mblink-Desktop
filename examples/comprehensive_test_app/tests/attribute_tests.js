/**
 * @file attribute_tests.js
 * @brief 属性和样式测试
 */

describe('属性和样式', () => {
    
    // ========== setAttribute/getAttribute ==========
    
    test('setAttribute - 设置属性', () => {
        const element = document.createElement('div');
        
        element.setAttribute('id', 'test');
        element.setAttribute('data-value', '123');
        element.setAttribute('title', 'Hello');
        
        assertEqual(element.getAttribute('id'), 'test', 'id 应为 test');
        assertEqual(element.getAttribute('data-value'), '123', 'data-value 应为 123');
        assertEqual(element.getAttribute('title'), 'Hello', 'title 应为 Hello');
    });
    
    test('getAttribute - 获取不存在的属性', () => {
        const element = document.createElement('div');
        
        const value = element.getAttribute('nonexistent');
        
        assertEqual(value, '', '不存在的属性应返回空字符串');
    });
    
    test('hasAttribute - 检查属性是否存在', () => {
        const element = document.createElement('div');
        element.setAttribute('id', 'test');
        
        assertTrue(element.hasAttribute('id'), '应有 id 属性');
        assertFalse(element.hasAttribute('class'), '不应有 class 属性');
    });
    
    test('removeAttribute - 移除属性', () => {
        const element = document.createElement('div');
        element.setAttribute('id', 'test');
        element.setAttribute('class', 'container');
        
        element.removeAttribute('id');
        
        assertFalse(element.hasAttribute('id'), '不应有 id 属性');
        assertTrue(element.hasAttribute('class'), '应有 class 属性');
    });
    
    // ========== id 属性 ==========
    
    test('id - 设置和获取', () => {
        const element = document.createElement('div');
        
        element.id = 'test-id';
        
        assertEqual(element.id, 'test-id', 'id 应为 test-id');
        assertEqual(element.getAttribute('id'), 'test-id', 'getAttribute 应返回 test-id');
    });
    
    // ========== className ==========
    
    test('className - 设置和获取', () => {
        const element = document.createElement('div');
        
        element.className = 'container main';
        
        assertEqual(element.className, 'container main', 'className 应为 container main');
        assertEqual(element.getAttribute('class'), 'container main', 'getAttribute 应返回 container main');
    });
    
    test('className - 修改', () => {
        const element = document.createElement('div');
        element.className = 'old-class';
        
        element.className = 'new-class';
        
        assertEqual(element.className, 'new-class', 'className 应为 new-class');
    });
    
    // ========== classList ==========
    
    test('classList.add - 添加单个 class', () => {
        const element = document.createElement('div');
        
        element.classList.add('active');
        
        assertTrue(element.classList.contains('active'), '应包含 active');
        assertEqual(element.className, 'active', 'className 应为 active');
    });
    
    test('classList.add - 添加多个 class', () => {
        const element = document.createElement('div');
        
        element.classList.add('active');
        element.classList.add('highlight');
        element.classList.add('selected');
        
        assertTrue(element.classList.contains('active'), '应包含 active');
        assertTrue(element.classList.contains('highlight'), '应包含 highlight');
        assertTrue(element.classList.contains('selected'), '应包含 selected');
    });
    
    test('classList.remove - 移除 class', () => {
        const element = document.createElement('div');
        element.className = 'active highlight selected';
        
        element.classList.remove('highlight');
        
        assertTrue(element.classList.contains('active'), '应包含 active');
        assertFalse(element.classList.contains('highlight'), '不应包含 highlight');
        assertTrue(element.classList.contains('selected'), '应包含 selected');
    });
    
    test('classList.toggle - 切换 class（添加）', () => {
        const element = document.createElement('div');
        
        const result = element.classList.toggle('active');
        
        assertTrue(result, '应返回 true');
        assertTrue(element.classList.contains('active'), '应包含 active');
    });
    
    test('classList.toggle - 切换 class（移除）', () => {
        const element = document.createElement('div');
        element.className = 'active';
        
        const result = element.classList.toggle('active');
        
        assertFalse(result, '应返回 false');
        assertFalse(element.classList.contains('active'), '不应包含 active');
    });
    
    test('classList.contains - 检查 class', () => {
        const element = document.createElement('div');
        element.className = 'container main';
        
        assertTrue(element.classList.contains('container'), '应包含 container');
        assertTrue(element.classList.contains('main'), '应包含 main');
        assertFalse(element.classList.contains('other'), '不应包含 other');
    });
    
    test('classList.length - 获取 class 数量', () => {
        const element = document.createElement('div');
        element.className = 'one two three';
        
        assertEqual(element.classList.length, 3, 'class 数量应为 3');
    });
    
    test('classList.item - 获取指定索引的 class', () => {
        const element = document.createElement('div');
        element.className = 'first second third';
        
        assertEqual(element.classList.item(0), 'first', '第 0 个 class 应为 first');
        assertEqual(element.classList.item(1), 'second', '第 1 个 class 应为 second');
        assertEqual(element.classList.item(2), 'third', '第 2 个 class 应为 third');
    });
    
    // ========== style ==========
    
    test('style.setProperty - 设置样式', () => {
        const element = document.createElement('div');
        
        element.style.setProperty('color', 'red');
        element.style.setProperty('font-size', '16px');
        element.style.setProperty('background-color', 'blue');
        
        assertEqual(element.style.getPropertyValue('color'), 'red', 'color 应为 red');
        assertEqual(element.style.getPropertyValue('font-size'), '16px', 'font-size 应为 16px');
        assertEqual(element.style.getPropertyValue('background-color'), 'blue', 'background-color 应为 blue');
    });
    
    test('style.getPropertyValue - 获取不存在的样式', () => {
        const element = document.createElement('div');
        
        const value = element.style.getPropertyValue('nonexistent');
        
        assertEqual(value, '', '不存在的样式应返回空字符串');
    });
    
    test('style.removeProperty - 移除样式', () => {
        const element = document.createElement('div');
        element.style.setProperty('color', 'red');
        element.style.setProperty('font-size', '16px');
        
        element.style.removeProperty('color');
        
        assertEqual(element.style.getPropertyValue('color'), '', 'color 应为空');
        assertEqual(element.style.getPropertyValue('font-size'), '16px', 'font-size 应为 16px');
    });
    
    test('style.cssText - 设置完整样式', () => {
        const element = document.createElement('div');
        
        element.style.cssText = 'color: red; font-size: 16px;';
        
        assertEqual(element.style.getPropertyValue('color'), 'red', 'color 应为 red');
        assertEqual(element.style.getPropertyValue('font-size'), '16px', 'font-size 应为 16px');
    });
    
    test('style.cssText - 获取完整样式', () => {
        const element = document.createElement('div');
        element.style.setProperty('color', 'red');
        element.style.setProperty('font-size', '16px');
        
        const cssText = element.style.cssText;
        
        assertNotEqual(cssText, '', 'cssText 不应为空');
        assertTrue(cssText.includes('color'), 'cssText 应包含 color');
        assertTrue(cssText.includes('font-size'), 'cssText 应包含 font-size');
    });
    
    test('style.length - 获取样式数量', () => {
        const element = document.createElement('div');
        element.style.setProperty('color', 'red');
        element.style.setProperty('font-size', '16px');
        element.style.setProperty('background-color', 'blue');
        
        assertEqual(element.style.length, 3, '样式数量应为 3');
    });
    
    // ========== dataset ==========
    
    test('dataset.set - 设置 data 属性', () => {
        const element = document.createElement('div');
        
        element.dataset.set('userId', '123');
        element.dataset.set('userName', 'John');
        
        assertEqual(element.dataset.get('userId'), '123', 'userId 应为 123');
        assertEqual(element.dataset.get('userName'), 'John', 'userName 应为 John');
        assertEqual(element.getAttribute('data-user-id'), '123', 'data-user-id 应为 123');
        assertEqual(element.getAttribute('data-user-name'), 'John', 'data-user-name 应为 John');
    });
    
    test('dataset.get - 获取不存在的 data 属性', () => {
        const element = document.createElement('div');
        
        const value = element.dataset.get('nonexistent');
        
        assertEqual(value, '', '不存在的 data 属性应返回空字符串');
    });
    
    test('dataset.has - 检查 data 属性是否存在', () => {
        const element = document.createElement('div');
        element.dataset.set('userId', '123');
        
        assertTrue(element.dataset.has('userId'), '应有 userId');
        assertFalse(element.dataset.has('userName'), '不应有 userName');
    });
    
    test('dataset.remove - 移除 data 属性', () => {
        const element = document.createElement('div');
        element.dataset.set('userId', '123');
        element.dataset.set('userName', 'John');
        
        element.dataset.remove('userId');
        
        assertFalse(element.dataset.has('userId'), '不应有 userId');
        assertTrue(element.dataset.has('userName'), '应有 userName');
    });
    
    test('dataset - 驼峰命名转换', () => {
        const element = document.createElement('div');
        
        element.dataset.set('userFullName', 'John Doe');
        
        assertEqual(element.getAttribute('data-user-full-name'), 'John Doe', 'data-user-full-name 应为 John Doe');
        assertEqual(element.dataset.get('userFullName'), 'John Doe', 'userFullName 应为 John Doe');
    });
});

