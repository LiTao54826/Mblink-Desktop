/**
 * @file test_dispatchEvent.js
 * @brief 测试 DispatchEvent 是否被正确触发
 */

console.log('🧪 测试 DispatchEvent...');

var button = document.createElement('button');
button.textContent = '测试按钮';
button.style.setProperty('padding', '20px');
button.style.setProperty('font-size', '20px');

// 设置 ID 以便在控制台中识别
button.setAttribute('id', 'test-button');

// 添加多层事件监听器
console.log('添加第1个事件监听器...');
var listener1 = button.addEventListener('click', function (e) {
    console.log('✅ 监听器1被触发！');
    console.log('  Event type:', e.type);
    console.log('  Event target:', e.target);
});
console.log('addEventListener #1 返回:', listener1);

console.log('添加第2个事件监听器...');
var listener2 = button.addEventListener('click', function (e) {
    console.log('✅ 监听器2被触发！');
});
console.log('addEventListener #2 返回:', listener2);

document.body.appendChild(button);

console.log('');
console.log('✅ 按钮已添加，请点击测试');
console.log('Button id:', button.getAttribute('id'));
