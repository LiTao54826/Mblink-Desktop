/**
 * Canvas集成测试 - 绘制多个形状
 */

console.log('Canvas Integration Test');

const canvas = document.createElement('canvas');
canvas.width = 800;
canvas.height = 600;
canvas.setAttribute('id', 'myCanvas');

// 先添加到文档
document.body.appendChild(canvas);

const ctx = canvas.getContext('2d');

if (ctx) {
    console.log('✓ Got context');

    // 测试1: 填充矩形（红色）
    console.log('Drawing red rectangle...');
    ctx.fillStyle = '#FF0000';
    ctx.fillRect(50, 50, 200, 150);

    // 测试2: 描边矩形（蓝色）
    console.log('Drawing blue stroke rectangle...');
    ctx.strokeStyle = '#0000FF';
    ctx.lineWidth = 5;
    ctx.strokeRect(300, 50, 200, 150);

    // 测试3: 路径绘制（绿色填充）
    console.log('Drawing green triangle...');
    ctx.fillStyle = '#00FF00';
    ctx.beginPath();
    ctx.moveTo(150, 300);
    ctx.lineTo(250, 300);
    ctx.lineTo(200, 400);
    ctx.closePath();
    ctx.fill();

    // 测试4: 圆形（黄色描边）
    console.log('Drawing yellow circle...');
    ctx.strokeStyle = '#FFFF00';
    ctx.lineWidth = 3;
    ctx.beginPath();
    ctx.arc(400, 350, 80, 0, Math.PI * 2);
    ctx.stroke();

    // 测试5: 文本
    console.log('Drawing text...');
    ctx.fillStyle = '#000000';
    ctx.fillText('Canvas Works!', 50, 500);

    console.log('✓ All drawings complete');
    console.log('Note: Canvas is offscreen - need DOM integration to display');

} else {
    console.log('✗ No context');
}

console.log('Test complete');
