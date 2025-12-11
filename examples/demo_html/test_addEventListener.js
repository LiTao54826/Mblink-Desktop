/**
 * @file test_addEventListener.js  
 * @brief 测试 addEventListener 基本功能
 */

console.log('🧪 测试 addEventListener...');

// 创建一个按钮
var button = document.createElement('button');
button.textContent = '点击我';
button.style.setProperty('padding', '20px');
button.style.setProperty('font-size', '20px');

// 添加事件监听器
console.log('添加事件监听器...');
var result = button.addEventListener('click', function (e) {
    console.log('✅ 按钮被点击了！');
    console.log('事件对象:', e);
});

console.log('addEventListener 返回值:', result);

// 将按钮添加到文档
document.body.appendChild(button);

console.log('✅ 按钮已添加到文档，请点击测试');

// 手动触发点击测试
console.log('');
console.log('尝试手动触发 click...');
if (button.click) {
    button.click();
} else {
    console.log('⚠️  button.click() 不可用');
}
