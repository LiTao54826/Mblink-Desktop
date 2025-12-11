/**
 * @file classlist_test.js
 * @brief 测试 Element.classList API
 */

console.log('🎨 classList 测试开始...');

// 创建测试元素
const container = document.createElement('div');
container.setAttribute('id', 'container');
container.style.setProperty('padding', '20px');

const title = document.createElement('h1');
title.textContent = '🎨 classList API 测试';
title.style.setProperty('color', '#2c3e50');
container.appendChild(title);

// 创建测试目标元素
const testDiv = document.createElement('div');
testDiv.setAttribute('id', 'test-element');
testDiv.setAttribute('class', 'initial-class');
testDiv.textContent = '这是测试元素';
testDiv.style.setProperty('padding', '15px');
testDiv.style.setProperty('margin', '20px 0');
testDiv.style.setProperty('border', '2px solid #3498db');
testDiv.style.setProperty('border-radius', '5px');
container.appendChild(testDiv);

// 按钮容器
const btnContainer = document.createElement('div');
btnContainer.style.setProperty('margin-top', '20px');

function createButton(text, onClick) {
    const btn = document.createElement('button');
    btn.textContent = text;
    btn.style.setProperty('margin-right', '10px');
    btn.style.setProperty('margin-bottom', '10px');
    btn.style.setProperty('padding', '10px 15px');
    btn.addEventListener('click', onClick);
    return btn;
}

// 测试按钮
const addBtn = createButton('添加 "highlight" 类', function () {
    testDiv.classList.add('highlight');
    updateDisplay();
    console.log('✅ 添加 highlight 类');
});

const removeBtn = createButton('移除 "highlight" 类', function () {
    testDiv.classList.remove('highlight');
    updateDisplay();
    console.log('✅ 移除 highlight 类');
});

const toggleBtn = createButton('切换 "active" 类', function () {
    const added = testDiv.classList.toggle('active');
    updateDisplay();
    console.log('✅ 切换 active 类，结果:', added ? '添加' : '移除');
});

const containsBtn = createButton('检查是否包含 "highlight"', function () {
    const has = testDiv.classList.contains('highlight');
    updateDisplay();
    showResult(has ? '✅ 包含 "highlight" 类' : '❌ 不包含 "highlight" 类');
    console.log('检查结果:', has);
});

const multiAddBtn = createButton('添加多个类', function () {
    testDiv.classList.add('class1');
    testDiv.classList.add('class2');
    testDiv.classList.add('class3');
    updateDisplay();
    console.log('✅ 添加多个类');
});

const clearBtn = createButton('清空所有类', function () {
    testDiv.setAttribute('class', '');
    updateDisplay();
    console.log('✅ 清空所有类');
});

btnContainer.appendChild(addBtn);
btnContainer.appendChild(removeBtn);
btnContainer.appendChild(toggleBtn);
btnContainer.appendChild(containsBtn);
btnContainer.appendChild(multiAddBtn);
btnContainer.appendChild(clearBtn);

container.appendChild(btnContainer);

// 显示当前 className
const displayDiv = document.createElement('div');
displayDiv.setAttribute('id', 'display');
displayDiv.style.setProperty('margin-top', '20px');
displayDiv.style.setProperty('padding', '15px');
displayDiv.style.setProperty('background', '#ecf0f1');
displayDiv.style.setProperty('border-radius', '5px');
container.appendChild(displayDiv);

// 结果显示
const resultDiv = document.createElement('div');
resultDiv.setAttribute('id', 'result');
resultDiv.style.setProperty('margin-top', '10px');
resultDiv.style.setProperty('padding', '10px');
resultDiv.style.setProperty('background', '#d5dade');
resultDiv.style.setProperty('border-radius', '5px');
resultDiv.style.setProperty('min-height', '30px');
container.appendChild(resultDiv);

document.body.appendChild(container);

// 更新显示函数
function updateDisplay() {
    const currentClass = testDiv.getAttribute('class') || '';
    displayDiv.textContent = '当前 className: "' + (currentClass || '(空)') + '"';

    // 更新测试元素样式
    if (currentClass.indexOf('highlight') >= 0) {
        testDiv.style.setProperty('background', '#fff3cd');
        testDiv.style.setProperty('border', '2px solid #ffc107');
    } else if (currentClass.indexOf('active') >= 0) {
        testDiv.style.setProperty('background', '#d4edda');
        testDiv.style.setProperty('border', '2px solid #28a745');
    } else {
        testDiv.style.setProperty('background', 'white');
        testDiv.style.setProperty('border', '2px solid #3498db');
    }
}

function showResult(message) {
    resultDiv.textContent = message;
}

// 初始化显示
updateDisplay();

// 运行自动化测试
console.log('=== 自动化测试开始 ===');
console.log('初始 className:', testDiv.getAttribute('class'));

// 测试 add
testDiv.classList.add('test1');
console.log('add("test1"):', testDiv.getAttribute('class'));

// 测试 contains
console.log('contains("test1"):', testDiv.classList.contains('test1'));
console.log('contains("not-exist"):', testDiv.classList.contains('not-exist'));

// 测试 toggle
const toggled1 = testDiv.classList.toggle('test2');
console.log('toggle("test2"):', toggled1, '→', testDiv.getAttribute('class'));

const toggled2 = testDiv.classList.toggle('test2');
console.log('toggle("test2") again:', toggled2, '→', testDiv.getAttribute('class'));

// 测试 remove
testDiv.classList.remove('test1');
console.log('remove("test1"):', testDiv.getAttribute('class'));

console.log('=== 自动化测试完成 ===');
console.log('');
console.log('✅ classList API 准备完成！');
console.log('点击按钮进行交互测试...');

updateDisplay();
