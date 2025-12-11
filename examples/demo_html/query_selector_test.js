/**
 * @file query_selector_test.js
 * @brief 测试 querySelector 和 querySelectorAll API
 */

console.log('🔍 查询选择器测试开始...');

// ========== 创建测试 DOM 结构 ==========
const container = document.createElement('div');
container.setAttribute('id', 'container');
container.style.setProperty('padding', '20px');

// 标题
const title = document.createElement('h1');
title.textContent = '🔍 querySelector 测试';
title.style.setProperty('color', '#2c3e50');
container.appendChild(title);

// 创建测试列表
const list = document.createElement('ul');
list.setAttribute('class', 'test-list');

const items = ['测试项目 1', '测试项目 2', '测试项目 3'];
items.forEach((text, index) => {
    const li = document.createElement('li');
    li.setAttribute('class', 'list-item');
    li.setAttribute('data-index', index.toString());
    li.textContent = text;
    list.appendChild(li);
});

container.appendChild(list);

// 创建按钮容器
const buttonContainer = document.createElement('div');
buttonContainer.setAttribute('id', 'buttons');
buttonContainer.style.setProperty('margin-top', '20px');

const testBtn1 = document.createElement('button');
testBtn1.textContent = '测试 querySelector';
testBtn1.setAttribute('class', 'test-button');
buttonContainer.appendChild(testBtn1);

const testBtn2 = document.createElement('button');
testBtn2.textContent = '测试 querySelectorAll';
testBtn2.setAttribute('class', 'test-button');
testBtn2.style.setProperty('margin-left', '10px');
buttonContainer.appendChild(testBtn2);

container.appendChild(buttonContainer);

// 结果显示区
const resultDiv = document.createElement('div');
resultDiv.setAttribute('id', 'result');
resultDiv.style.setProperty('margin-top', '20px');
resultDiv.style.setProperty('padding', '15px');
resultDiv.style.setProperty('background', '#ecf0f1');
resultDiv.style.setProperty('border-radius', '5px');
container.appendChild(resultDiv);

document.body.appendChild(container);

// ========== 测试函数 ==========

function showResult(message, success) {
    resultDiv.textContent = '';
    const p = document.createElement('p');
    p.textContent = message;
    p.style.setProperty('color', success ? '#27ae60' : '#e74c3c');
    p.style.setProperty('font-weight', 'bold');
    resultDiv.appendChild(p);
}

// 测试 1: querySelector
testBtn1.addEventListener('click', function () {
    console.log('=== 测试 querySelector ===');

    try {
        // 测试 ID 选择器
        const elem1 = document.body.querySelector('#container');
        console.log('querySelector("#container"):', elem1 ? elem1.tagName : null);

        // 测试类选择器
        const elem2 = container.querySelector('.test-list');
        console.log('querySelector(".test-list"):', elem2 ? elem2.tagName : null);

        // 测试标签选择器
        const elem3 = container.querySelector('h1');
        console.log('querySelector("h1"):', elem3 ? elem3.textContent : null);

        // 测试复合选择器
        const elem4 = container.querySelector('.list-item');
        console.log('querySelector(".list-item"):', elem4 ? elem4.textContent : null);

        if (elem1 && elem2 && elem3 && elem4) {
            showResult('✅ querySelector 测试通过！', true);
            console.log('✅ querySelector 所有测试通过');
        } else {
            showResult('❌ querySelector 测试失败', false);
        }
    } catch (e) {
        console.log('❌ 错误:', e);
        showResult('❌ querySelector 出错: ' + e, false);
    }
});

// 测试 2: querySelectorAll
testBtn2.addEventListener('click', function () {
    console.log('=== 测试 querySelectorAll ===');

    try {
        // 测试选择所有列表项
        const items = container.querySelectorAll('.list-item');
        console.log('querySelectorAll(".list-item") 找到', items.length, '个元素');

        if (items.length === 3) {
            let allMatch = true;
            for (let i = 0; i < items.length; i++) {
                console.log('  项目', i, ':', items[i].textContent);
                if (!items[i].textContent) {
                    allMatch = false;
                }
            }

            if (allMatch) {
                showResult('✅ querySelectorAll 测试通过！找到 ' + items.length + ' 个元素', true);
                console.log('✅ querySelectorAll 所有测试通过');
            } else {
                showResult('❌ 某些元素内容为空', false);
            }
        } else {
            showResult('❌ querySelectorAll 应找到 3 个元素，实际找到 ' + items.length, false);
        }

        // 测试选择所有按钮
        const buttons = document.body.querySelectorAll('.test-button');
        console.log('querySelectorAll(".test-button") 找到', buttons.length, '个按钮');

    } catch (e) {
        console.log('❌ 错误:', e);
        showResult('❌ querySelectorAll 出错: ' + e, false);
    }
});

// 自动运行测试
console.log('📋 DOM 结构已创建');
console.log('  - #container');
console.log('  - .test-list with 3 .list-item');
console.log('  - 2 .test-button');
console.log('');
console.log('✅ 查询选择器测试准备完成！');
console.log('点击按钮开始测试...');
