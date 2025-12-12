/**
 * Canvas 超级最小测试 - 只获取context
 */

console.log('Getting canvas...');
const canvas = document.createElement('canvas');
canvas.width = 400;
canvas.height = 300;
document.body.appendChild(canvas);

console.log('Getting context...');
const ctx = canvas.getContext('2d');

console.log('Got context:', ctx);
console.log('typeof ctx:', typeof ctx);

// 不做任何操作，只是成功获取context
if (ctx) {
    console.log('✓ SUCCESS! Context obtained.');
} else {
    console.log('✗ FAIL! No context.');
}

console.log('Test complete - staying open');

// 保持窗口打开
setTimeout(() => {
    console.log('Still alive after 1 second');
}, 1000);
