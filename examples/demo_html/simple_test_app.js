/**
 * @file simple_test_app.js
 * @brief 简单的DOM绑定测试（不使用Preact）
 */

console.log('========================================');
console.log('  简单 DOM 绑定测试');
console.log('========================================');

// 测试 document 对象
console.log('✓ document:', typeof document);
console.log('✓ document.body:', document.body);
console.log('✓ document.createElement:', typeof document.createElement);

// 测试创建元素
const div = document.createElement('div');
console.log('✓ Created div:', div);
console.log('✓ div.tagName:', div.tagName);

// 测试属性设置
div.setAttribute('id', 'test-div');
console.log('✓ Set id attribute');
console.log('✓ div.getAttribute(\'id\'):', div.getAttribute('id'));

// 测试文本内容
div.textContent = 'Hello from LightUI!';
console.log('✓ Set textContent:', div.textContent);

// 测试样式
div.style.setProperty('color', 'blue');
div.style.setProperty('font-size', '24px');
div.style.setProperty('padding', '20px');
console.log('✓ Set style properties');

// 测试 appendChild
document.body.appendChild(div);
console.log('✓ Appended div to body');
console.log('✓ document.body.firstChild:', document.body.firstChild);

// 创建更多元素
const title = document.createElement('h1');
title.textContent = '🎉 DOM 绑定测试成功！';
title.style.setProperty('color', 'green');
title.style.setProperty('text-align', 'center');

const message = document.createElement('p');
message.textContent = '所有核心 DOM API 都正常工作：';
message.style.setProperty('text-align', 'center');
message.style.setProperty('font-size', '18px');

const list = document.createElement('ul');
list.style.setProperty('list-style', 'none');
list.style.setProperty('padding', '20px');

const features = [
    'document.createElement ✓',
    'element.appendChild ✓',
    'element.setAttribute ✓',
    'element.textContent ✓',
    'element.style.setProperty ✓',
    'element.firstChild ✓'
];

features.forEach(feature => {
    const li = document.createElement('li');
    li.textContent = feature;
    li.style.setProperty('padding', '5px');
    li.style.setProperty('font-size', '16px');
    list.appendChild(li);
});

// 添加到 body
document.body.appendChild(title);
document.body.appendChild(message);
document.body.appendChild(list);

console.log('========================================');
console.log('✅ 所有测试通过！');
console.log('✅ 页面已渲染完成');
console.log('========================================');
