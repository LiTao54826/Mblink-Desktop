/**
 * Canvas 最小测试 - 仅测试fillRect
 */

console.log('=== Minimal Canvas Test ===');

// 创建Canvas
const canvas = document.createElement('canvas');
canvas.width = 400;
canvas.height = 300;
document.body.appendChild(canvas);

console.log('Canvas created');

// 获取context
const ctx = canvas.getContext('2d');
console.log('Context obtained:', ctx);

if (ctx) {
    try {
        console.log('Setting fillStyle...');
        ctx.fillStyle = '#ff0000';
        console.log('fillStyle set to:', ctx.fillStyle);

        console.log('Calling fillRect(50, 50, 100, 100)...');
        ctx.fillRect(50, 50, 100, 100);
        console.log('fillRect completed!');

        console.log('✓ Test passed!');
    } catch (e) {
        console.log('✗ Error:', e.toString());
    }
} else {
    console.log('✗ No context');
}

console.log('=== Test End ===');
