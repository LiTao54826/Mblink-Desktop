/**
 * 测试文本节点渲染
 */

console.log('=== 测试开始 ===');

// 测试1: 使用 innerHTML
console.log('测试1: innerHTML');
var div1 = document.createElement('div');
div1.style.cssText = 'padding: 20px; margin: 10px; background: #f0f0f0;';
div1.innerHTML = '测试1: innerHTML 文本';
document.body.appendChild(div1);
console.log('div1.innerHTML = ' + div1.innerHTML);

// 测试2: 使用 textContent
console.log('测试2: textContent');
var div2 = document.createElement('div');
div2.style.cssText = 'padding: 20px; margin: 10px; background: #e0e0e0;';
div2.textContent = '测试2: textContent 文本';
document.body.appendChild(div2);
console.log('div2.textContent = ' + div2.textContent);

// 测试3: 使用 createTextNode
console.log('测试3: createTextNode');
var div3 = document.createElement('div');
div3.style.cssText = 'padding: 20px; margin: 10px; background: #d0d0d0;';
var textNode = document.createTextNode('测试3: createTextNode 文本');
div3.appendChild(textNode);
document.body.appendChild(div3);
console.log('div3 childNodes.length = ' + div3.childNodes.length);

// 测试4: 嵌套元素中的文本
console.log('测试4: 嵌套元素');
var div4 = document.createElement('div');
div4.style.cssText = 'padding: 20px; margin: 10px; background: #c0c0c0;';
var span = document.createElement('span');
span.style.cssText = 'color: red; font-size: 24px;';
span.textContent = '测试4: span 中的文本';
div4.appendChild(span);
document.body.appendChild(div4);
console.log('span.textContent = ' + span.textContent);

// 测试5: 动态更新文本
console.log('测试5: 动态更新');
var div5 = document.createElement('div');
div5.style.cssText = 'padding: 20px; margin: 10px; background: #b0b0b0; font-size: 32px;';
div5.textContent = '计数: 0';
document.body.appendChild(div5);

var count = 0;
setInterval(function() {
    count++;
    div5.textContent = '计数: ' + count;
    console.log('更新计数: ' + count);
}, 1000);

console.log('=== 测试设置完成 ===');
