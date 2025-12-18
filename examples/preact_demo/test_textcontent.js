/**
 * 测试 textContent 属性
 */

(function() {
    'use strict';
    
    console.log('=== 测试 textContent ===');
    
    // 测试1: 直接设置 textContent
    console.log('测试1: 直接设置 textContent');
    var div1 = document.createElement('div');
    div1.style.cssText = 'padding: 20px; margin: 10px; background: #f0f0f0; font-size: 24px;';
    div1.textContent = '直接设置的文本';
    document.body.appendChild(div1);
    console.log('div1.textContent = ' + div1.textContent);
    
    // 测试2: 使用 Preact h() 函数和 textContent 属性
    console.log('测试2: Preact h() + textContent');
    var h = Preact.h;
    var render = Preact.render;
    
    var vnode = h('div', {
        style: {
            padding: '20px',
            margin: '10px',
            background: '#e0e0e0',
            fontSize: '24px'
        },
        textContent: 'Preact textContent 测试'
    });
    
    console.log('vnode:', JSON.stringify(vnode, null, 2));
    
    var container = document.createElement('div');
    document.body.appendChild(container);
    render(vnode, container);
    
    // 测试3: 使用 Preact h() 函数和子元素
    console.log('测试3: Preact h() + 子元素');
    var vnode2 = h('div', {
        style: {
            padding: '20px',
            margin: '10px',
            background: '#d0d0d0',
            fontSize: '24px'
        }
    }, 'Preact 子元素测试');
    
    var container2 = document.createElement('div');
    document.body.appendChild(container2);
    render(vnode2, container2);
    
    // 测试4: 动态更新 textContent
    console.log('测试4: 动态更新');
    var div4 = document.createElement('div');
    div4.style.cssText = 'padding: 20px; margin: 10px; background: #c0c0c0; font-size: 32px;';
    div4.textContent = '计数: 0';
    document.body.appendChild(div4);
    
    var count = 0;
    setInterval(function() {
        count++;
        div4.textContent = '计数: ' + count;
        console.log('更新计数: ' + count);
    }, 1000);
    
    console.log('=== 测试完成 ===');
})();
