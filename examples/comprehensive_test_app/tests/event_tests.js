/**
 * @file event_tests.js
 * @brief 事件系统测试
 */

describe('事件系统', () => {
    
    // ========== addEventListener ==========
    
    test('addEventListener - 基础事件监听', () => {
        const button = document.createElement('button');
        let clicked = false;
        
        button.addEventListener('click', () => {
            clicked = true;
        });
        
        button.dispatchEvent(new Event('click'));
        
        assertTrue(clicked, '事件应被触发');
    });
    
    test('addEventListener - 多个监听器', () => {
        const button = document.createElement('button');
        let count = 0;
        
        button.addEventListener('click', () => { count++; });
        button.addEventListener('click', () => { count++; });
        button.addEventListener('click', () => { count++; });
        
        button.dispatchEvent(new Event('click'));
        
        assertEqual(count, 3, '所有监听器都应被触发');
    });
    
    test('addEventListener - once 选项（只执行一次）', () => {
        const button = document.createElement('button');
        let count = 0;
        
        button.addEventListener('click', () => { count++; }, false, true);
        
        button.dispatchEvent(new Event('click'));
        button.dispatchEvent(new Event('click'));
        button.dispatchEvent(new Event('click'));
        
        assertEqual(count, 1, '事件应只触发一次');
    });
    
    test('addEventListener - 不同事件类型', () => {
        const element = document.createElement('div');
        let clickCount = 0;
        let mouseoverCount = 0;
        
        element.addEventListener('click', () => { clickCount++; });
        element.addEventListener('mouseover', () => { mouseoverCount++; });
        
        element.dispatchEvent(new Event('click'));
        element.dispatchEvent(new Event('mouseover'));
        element.dispatchEvent(new Event('click'));
        
        assertEqual(clickCount, 2, 'click 应触发 2 次');
        assertEqual(mouseoverCount, 1, 'mouseover 应触发 1 次');
    });
    
    // ========== removeEventListener ==========
    
    test('removeEventListener - 移除事件监听器', () => {
        const button = document.createElement('button');
        let count = 0;
        
        const listenerId = button.addEventListener('click', () => { count++; });
        
        button.dispatchEvent(new Event('click'));
        button.removeEventListener('click', listenerId);
        button.dispatchEvent(new Event('click'));
        
        assertEqual(count, 1, '移除后事件不应再触发');
    });
    
    test('removeEventListener - 移除不存在的监听器', () => {
        const button = document.createElement('button');
        
        const result = button.removeEventListener('click', 99999);
        
        assertFalse(result, '移除不存在的监听器应返回 false');
    });
    
    // ========== dispatchEvent ==========
    
    test('dispatchEvent - 分发自定义事件', () => {
        const element = document.createElement('div');
        let triggered = false;
        
        element.addEventListener('custom-event', () => {
            triggered = true;
        });
        
        element.dispatchEvent(new Event('custom-event'));
        
        assertTrue(triggered, '自定义事件应被触发');
    });
    
    // ========== 事件冒泡 ==========
    
    test('事件冒泡 - 从子元素到父元素', () => {
        const parent = document.createElement('div');
        const child = document.createElement('button');
        parent.appendChild(child);
        
        let parentClicked = false;
        let childClicked = false;
        
        parent.addEventListener('click', () => { parentClicked = true; });
        child.addEventListener('click', () => { childClicked = true; });
        
        child.dispatchEvent(new Event('click', { bubbles: true }));
        
        assertTrue(childClicked, 'child 事件应被触发');
        assertTrue(parentClicked, 'parent 事件应被触发（冒泡）');
    });
    
    test('事件冒泡 - 多层嵌套', () => {
        const grandparent = document.createElement('div');
        const parent = document.createElement('div');
        const child = document.createElement('button');
        
        grandparent.appendChild(parent);
        parent.appendChild(child);
        
        const order = [];
        
        grandparent.addEventListener('click', () => { order.push('grandparent'); });
        parent.addEventListener('click', () => { order.push('parent'); });
        child.addEventListener('click', () => { order.push('child'); });
        
        child.dispatchEvent(new Event('click', { bubbles: true }));
        
        assertEqual(order.length, 3, '应触发 3 个事件');
        assertEqual(order[0], 'child', '第一个应为 child');
        assertEqual(order[1], 'parent', '第二个应为 parent');
        assertEqual(order[2], 'grandparent', '第三个应为 grandparent');
    });
    
    // ========== 事件捕获 ==========
    
    test('事件捕获 - 从父元素到子元素', () => {
        const parent = document.createElement('div');
        const child = document.createElement('button');
        parent.appendChild(child);
        
        const order = [];
        
        parent.addEventListener('click', () => { order.push('parent-capture'); }, true);
        child.addEventListener('click', () => { order.push('child'); });
        parent.addEventListener('click', () => { order.push('parent-bubble'); });
        
        child.dispatchEvent(new Event('click', { bubbles: true }));
        
        assertEqual(order.length, 3, '应触发 3 个事件');
        assertEqual(order[0], 'parent-capture', '第一个应为 parent-capture');
        assertEqual(order[1], 'child', '第二个应为 child');
        assertEqual(order[2], 'parent-bubble', '第三个应为 parent-bubble');
    });
    
    // ========== stopPropagation ==========
    
    test('stopPropagation - 停止事件传播', () => {
        const parent = document.createElement('div');
        const child = document.createElement('button');
        parent.appendChild(child);
        
        let parentClicked = false;
        
        child.addEventListener('click', (e) => {
            e.stopPropagation();
        });
        parent.addEventListener('click', () => { parentClicked = true; });
        
        child.dispatchEvent(new Event('click', { bubbles: true }));
        
        assertFalse(parentClicked, 'parent 事件不应被触发（已停止传播）');
    });
    
    // ========== preventDefault ==========
    
    test('preventDefault - 阻止默认行为', () => {
        const link = document.createElement('a');
        let defaultPrevented = false;
        
        link.addEventListener('click', (e) => {
            e.preventDefault();
            defaultPrevented = e.defaultPrevented;
        });
        
        link.dispatchEvent(new Event('click', { cancelable: true }));
        
        assertTrue(defaultPrevented, '默认行为应被阻止');
    });
    
    // ========== Event 对象属性 ==========
    
    test('Event.type - 事件类型', () => {
        const element = document.createElement('div');
        let eventType = '';
        
        element.addEventListener('custom-event', (e) => {
            eventType = e.type;
        });
        
        element.dispatchEvent(new Event('custom-event'));
        
        assertEqual(eventType, 'custom-event', '事件类型应为 custom-event');
    });
    
    test('Event.target - 事件目标', () => {
        const element = document.createElement('div');
        let eventTarget = null;
        
        element.addEventListener('click', (e) => {
            eventTarget = e.target;
        });
        
        element.dispatchEvent(new Event('click'));
        
        assertEqual(eventTarget, element, '事件目标应为 element');
    });
    
    test('Event.currentTarget - 当前目标', () => {
        const parent = document.createElement('div');
        const child = document.createElement('button');
        parent.appendChild(child);
        
        let currentTarget = null;
        
        parent.addEventListener('click', (e) => {
            currentTarget = e.currentTarget;
        });
        
        child.dispatchEvent(new Event('click', { bubbles: true }));
        
        assertEqual(currentTarget, parent, '当前目标应为 parent');
    });
    
    test('Event.bubbles - 是否冒泡', () => {
        const element = document.createElement('div');
        let bubbles = false;
        
        element.addEventListener('click', (e) => {
            bubbles = e.bubbles;
        });
        
        element.dispatchEvent(new Event('click', { bubbles: true }));
        
        assertTrue(bubbles, '事件应冒泡');
    });
    
    test('Event.cancelable - 是否可取消', () => {
        const element = document.createElement('div');
        let cancelable = false;
        
        element.addEventListener('click', (e) => {
            cancelable = e.cancelable;
        });
        
        element.dispatchEvent(new Event('click', { cancelable: true }));
        
        assertTrue(cancelable, '事件应可取消');
    });
});

