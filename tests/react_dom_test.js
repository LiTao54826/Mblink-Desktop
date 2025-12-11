/**
 * @file react_dom_test.js
 * @brief 测试 DOM 树绑定功能
 * 
 * 测试内容：
 * - document.createElement
 * - document.getElementById
 * - document.body
 * - element.appendChild
 * - element.setAttribute
 * - element.style
 */

console.log("=== React DOM Binding Test ===\n");

// 测试 1: createElement 和属性设置
console.log("Test 1: createElement and setAttribute");
let div = document.createElement('div');
console.log("  Created div:", div);
console.log("  div.tagName:", div.tagName);

div.setAttribute('id', 'test-div');
div.setAttribute('class', 'test-class');
console.log("  Set attributes: id=test-div, class=test-class");

let id = div.getAttribute('id');
console.log("  div.getAttribute('id'):", id);
console.log("  Test 1:", id === 'test-div' ? "PASS ✓" : "FAIL ✗");
console.log("");

// 测试 2: textContent
console.log("Test 2: textContent");
div.textContent = "Hello, World!";
console.log("  Set textContent:", div.textContent);
console.log("  Test 2:", div.textContent === "Hello, World!" ? "PASS ✓" : "FAIL ✗");
console.log("");

// 测试 3: appendChild
console.log("Test 3: appendChild");
let span = document.createElement('span');
span.textContent = "Child element";
div.appendChild(span);
console.log("  Appended span to div");
console.log("  div.firstChild:", div.firstChild);
console.log("  Test 3:", div.firstChild !== null ? "PASS ✓" : "FAIL ✗");
console.log("");

// 测试 4: style.setProperty
console.log("Test 4: style.setProperty");
div.style.setProperty('color', 'red');
div.style.setProperty('font-size', '16px');
console.log("  Set style properties");
let color = div.style.getPropertyValue('color');
console.log("  div.style.getPropertyValue('color'):", color);
console.log("  Test 4:", color === 'red' ? "PASS ✓" : "FAIL ✗");
console.log("");

// 测试 5: style.cssText
console.log("Test 5: style.cssText");
div.style.cssText = 'width: 100px; height: 50px;';
console.log("  Set cssText:", div.style.cssText);
let width = div.style.getPropertyValue('width');
console.log("  div.style.getPropertyValue('width'):", width);
console.log("  Test 5:", width === '100px' ? "PASS ✓" : "FAIL ✗");
console.log("");

// 测试 6: id 和 className 属性
console.log("Test 6: id and className properties");
div.id = 'new-id';
div.className = 'new-class another-class';
console.log("  Set id:", div.id);
console.log("  Set className:", div.className);
console.log("  Test 6:", (div.id === 'new-id' && div.className === 'new-class another-class') ? "PASS ✓" : "FAIL ✗");
console.log("");

// 测试 7: document.body 和 appendChild
console.log("Test 7: document.body");
console.log("  document.body:", document.body);
if (document.body) {
    document.body.appendChild(div);
    console.log("  Appended div to body");
    console.log("  Test 7: PASS ✓");
} else {
    console.log("  Test 7: FAIL ✗ (body is null)");
}
console.log("");

// 测试 8: 引用相等性（同一个 Node 应该返回同一个 JS 对象）
console.log("Test 8: Reference equality");
let div2 = document.getElementById('new-id');
console.log("  Retrieved element by id");
console.log("  div === div2:", div === div2);
console.log("  Test 8:", div === div2 ? "PASS ✓" : "FAIL ✗");
console.log("");

console.log("=== All DOM Tests Complete ===");
