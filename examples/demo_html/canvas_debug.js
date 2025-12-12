/**
 * Canvas API 简单测试 - 调试版本
 */

console.log('=== Canvas Debug Test ===');

// 测试1: 创建canvas元素
console.log('Step 1: Creating canvas element...');
const canvas = document.createElement('canvas');
console.log('Canvas element created:', canvas);
console.log('Canvas tagName:', canvas.tagName);

// 测试2: 设置属性
console.log('Step 2: Setting canvas attributes...');
canvas.width = 400;
canvas.height = 300;
console.log('Width:', canvas.width, 'Height:', canvas.height);

// 测试3: 检查getContext方法
console.log('Step 3: Checking getContext method...');
console.log('typeof canvas.getContext:', typeof canvas.getContext);
console.log('canvas.getContext:', canvas.getContext);

// 测试4: 添加到DOM
console.log('Step 4: Adding canvas to document...');
document.body.appendChild(canvas);
console.log('Canvas added to body');

// 测试5: 获取2D上下文
console.log('Step 5: Getting 2D context...');
try {
    const ctx = canvas.getContext('2d');
    console.log('Context obtained:', ctx);
    console.log('Context type:', typeof ctx);

    if (ctx) {
        console.log('✓ SUCCESS: Canvas 2D context works!');

        // 测试简单绘制
        console.log('Step 6: Testing simple drawing...');
        console.log('typeof ctx.fillStyle:', typeof ctx.fillStyle);
        console.log('typeof ctx.fillRect:', typeof ctx.fillRect);

        if (typeof ctx.fillRect === 'function') {
            ctx.fillStyle = '#ff0000';
            ctx.fillRect(10, 10, 100, 100);
            console.log('✓ fillRect executed');
        } else {
            console.log('✗ fillRect is not a function');
        }
    } else {
        console.log('✗ FAIL: Context is null');
    }
} catch (e) {
    console.log('✗ ERROR:', e.toString());
    console.log('Stack:', e.stack);
}

console.log('=== Test Complete ===');
