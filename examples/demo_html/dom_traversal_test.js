/**
 * @file dom_traversal_test.js
 * @brief 测试 DOM 遍历属性
 */

console.log('🌳 DOM 遍历属性测试开始...');

// 创建测试 DOM 结构
const container = document.createElement('div');
container.setAttribute('id', 'container');

const title = document.createElement('h1');
title.textContent = '🌳 DOM 遍历属性测试';
container.appendChild(title);

// 创建一个带有多个子元素的容器
const parent = document.createElement('div');
parent.setAttribute('id', 'parent');
parent.setAttribute('class', 'test-parent');

// 添加5个子元素
for (let i = 1; i <= 5; i++) {
    const child = document.createElement('div');
    child.setAttribute('id', 'child' + i);
    child.setAttribute('class', 'test-child');
    child.textContent = '子元素 ' + i;
    parent.appendChild(child);
}

container.appendChild(parent);

// 结果显示区
const output = document.createElement('pre');
output.setAttribute('id', 'output');
output.style.setProperty('background', '#f5f5f5');
output.style.setProperty('padding', '15px');
output.style.setProperty('border-radius', '5px');
output.style.setProperty('margin-top', '20px');
container.appendChild(output);

document.body.appendChild(container);

// 测试函数
function log(message) {
    console.log(message);
    output.textContent += message + '\n';
}

log('=== DOM结构 ===');
log('#parent 有 5 个子元素: child1, child2, child3, child4, child5');
log('');

// 测试 firstChild 和 lastChild
log('=== 测试 firstChild 和 lastChild ===');
const firstChild = parent.firstChild;
const lastChild = parent.lastChild;
log('parent.firstChild.id: ' + (firstChild ? firstChild.getAttribute('id') : 'null'));
log('parent.lastChild.id: ' + (lastChild ? lastChild.getAttribute('id') : 'null'));
log('');

// 测试 nextSibling 和 previousSibling
log('=== 测试 nextSibling 和 previousSibling ===');
const child3 = document.body.querySelector('#child3');
if (child3) {
    const next = child3.nextSibling;
    const prev = child3.previousSibling;
    log('child3.nextSibling.id: ' + (next ? next.getAttribute('id') : 'null'));
    log('child3.previousSibling.id: ' + (prev ? prev.getAttribute('id') : 'null'));
}
log('');

// 测试 parentNode
log('=== 测试 parentNode ===');
if (child3) {
    const parentNode = child3.parentNode;
    log('child3.parentNode.id: ' + (parentNode ? parentNode.getAttribute('id') : 'null'));
}
log('');

// 测试 Element.children
log('=== 测试 Element.children（HTMLCollection）===');
const children = parent.children;
log('parent.children.length: ' + children.length);
for (let i = 0; i < children.length; i++) {
    log('  children[' + i + '].id: ' + children[i].getAttribute('id'));
}
log('');

// 遍历测试
log('=== 遍历测试：从 firstChild 到 lastChild ===');
let current = parent.firstChild;
let index = 0;
while (current) {
    log('  [' + index + '] ' + current.getAttribute('id'));
    current = current.nextSibling;
    index++;
}
log('');

log('=== 反向遍历：从 lastChild 到 firstChild ===');
current = parent.lastChild;
index = 0;
while (current) {
    log('  [' + index + '] ' + current.getAttribute('id'));
    current = current.previousSibling;
    index++;
}
log('');

// 验证结果
log('=== 验证结果 ===');
let passed = true;
let tests = [];

// 测试1: firstChild
if (!firstChild || firstChild.getAttribute('id') !== 'child1') {
    passed = false;
    tests.push('❌ firstChild 失败');
} else {
    tests.push('✅ firstChild 正确');
}

// 测试2: lastChild
if (!lastChild || lastChild.getAttribute('id') !== 'child5') {
    passed = false;
    tests.push('❌ lastChild 失败');
} else {
    tests.push('✅ lastChild 正确');
}

// 测试3: nextSibling
if (child3) {
    const next = child3.nextSibling;
    if (!next || next.getAttribute('id') !== 'child4') {
        passed = false;
        tests.push('❌ nextSibling 失败');
    } else {
        tests.push('✅ nextSibling 正确');
    }
}

// 测试4: previousSibling
if (child3) {
    const prev = child3.previousSibling;
    if (!prev || prev.getAttribute('id') !== 'child2') {
        passed = false;
        tests.push('❌ previousSibling 失败');
    } else {
        tests.push('✅ previousSibling 正确');
    }
}

// 测试5: parentNode
if (child3) {
    const parentNode = child3.parentNode;
    if (!parentNode || parentNode.getAttribute('id') !== 'parent') {
        passed = false;
        tests.push('❌ parentNode 失败');
    } else {
        tests.push('✅ parentNode 正确');
    }
}

// 测试6: children
if (children.length !== 5) {
    passed = false;
    tests.push('❌ children.length 失败');
} else {
    tests.push('✅ children.length 正确');
}

tests.forEach(function (test) {
    log(test);
});

log('');
if (passed) {
    log('🎉 所有测试通过！');
} else {
    log('❌ 部分测试失败');
}
