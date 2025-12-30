/**
 * line-height: normal 测试
 */

console.log('=== line-height: normal Test ===');

// 测试1: 直接设置 style 属性
var div1 = document.createElement('div');
div1.setAttribute('style', 'line-height: normal; font-size: 14px; padding: 10px; background: #f0f0f0;');
div1.textContent = 'Test 1: style attribute with line-height: normal';
document.body.appendChild(div1);
console.log('div1 style attr:', div1.getAttribute('style'));

// 测试2: 通过 style 对象设置
var div2 = document.createElement('div');
div2.style.lineHeight = 'normal';
div2.style.fontSize = '14px';
div2.style.padding = '10px';
div2.style.background = '#e0e0e0';
div2.style.marginTop = '10px';
div2.textContent = 'Test 2: style.lineHeight = "normal"';
document.body.appendChild(div2);
console.log('div2 style attr:', div2.getAttribute('style'));
console.log('div2 style.lineHeight:', div2.style.lineHeight);

// 测试3: 使用数字 line-height
var div3 = document.createElement('div');
div3.style.lineHeight = '1.5';
div3.style.fontSize = '14px';
div3.style.padding = '10px';
div3.style.background = '#d0d0d0';
div3.style.marginTop = '10px';
div3.textContent = 'Test 3: style.lineHeight = "1.5"';
document.body.appendChild(div3);
console.log('div3 style attr:', div3.getAttribute('style'));

// 测试4: 不设置 line-height (使用默认值)
var div4 = document.createElement('div');
div4.style.fontSize = '14px';
div4.style.padding = '10px';
div4.style.background = '#c0c0c0';
div4.style.marginTop = '10px';
div4.textContent = 'Test 4: no line-height (default)';
document.body.appendChild(div4);
console.log('div4 style attr:', div4.getAttribute('style'));

console.log('Test complete');
