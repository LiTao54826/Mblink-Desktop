/**
 * Canvas API 验证测试
 * 
 * 测试所有已实现的Canvas 2D API功能
 */

console.log('Canvas Test App Starting...');

// 创建Canvas元素
const canvas = document.createElement('canvas');
canvas.width = 800;
canvas.height = 600;
canvas.style = 'border: 2px solid #333; display: block; margin: 20px auto;';
document.body.appendChild(canvas);

// 获取2D渲染上下文
const ctx = canvas.getContext('2d');

if (!ctx) {
    console.error('Failed to get 2D context!');
    document.body.innerHTML = '<h1 style="color: red; text-align: center;">Canvas 2D context not supported!</h1>';
} else {
    console.log('Canvas 2D context obtained successfully!');

    // 创建标题
    const title = document.createElement('h1');
    title.textContent = 'Canvas API 测试';
    title.style = 'text-align: center; color: #333; font-family: Arial;';
    document.body.insertBefore(title, canvas);

    // 测试1: 清除画布
    console.log('Test 1: Clear canvas');
    ctx.fillStyle = '#f0f0f0';
    ctx.fillRect(0, 0, 800, 600);

    // 测试2: 绘制矩形
    console.log('Test 2: Draw rectangles');
    ctx.fillStyle = '#ff6b6b';
    ctx.fillRect(50, 50, 150, 100);

    ctx.strokeStyle = '#4ecdc4';
    ctx.lineWidth = 3;
    ctx.strokeRect(220, 50, 150, 100);

    // 测试3: 路径和形状
    console.log('Test 3: Draw paths');
    ctx.fillStyle = '#95e1d3';
    ctx.beginPath();
    ctx.moveTo(450, 50);
    ctx.lineTo(550, 50);
    ctx.lineTo(500, 150);
    ctx.closePath();
    ctx.fill();

    // 测试4: 圆弧
    console.log('Test 4: Draw arcs');
    ctx.fillStyle = '#f38181';
    ctx.beginPath();
    ctx.arc(650, 100, 50, 0, Math.PI * 2, false);
    ctx.fill();

    // 测试5: 描边圆形
    ctx.strokeStyle = '#aa96da';
    ctx.lineWidth = 4;
    ctx.beginPath();
    ctx.arc(100, 250, 40, 0, Math.PI * 2);
    ctx.stroke();

    // 测试6: 半圆
    ctx.fillStyle = '#fcbad3';
    ctx.beginPath();
    ctx.arc(250, 250, 50, 0, Math.PI, false);
    ctx.fill();

    // 测试7: 文本绘制
    console.log('Test 7: Draw text');
    ctx.fillStyle = '#2c3e50';
    ctx.fillText('Hello Canvas!', 400, 250);

    // 测试8: 变换 - 平移和旋转
    console.log('Test 8: Transformations');
    ctx.save();
    ctx.translate(600, 250);
    ctx.rotate(Math.PI / 4);
    ctx.fillStyle = '#e74c3c';
    ctx.fillRect(-25, -25, 50, 50);
    ctx.restore();

    // 测试9: 缩放
    ctx.save();
    ctx.scale(1.5, 1.5);
    ctx.fillStyle = '#3498db';
    ctx.fillRect(50, 200, 30, 30);
    ctx.restore();

    // 测试10: 线条样式
    console.log('Test 10: Line styles');
    ctx.strokeStyle = '#16a085';
    ctx.lineWidth = 5;
    ctx.beginPath();
    ctx.moveTo(50, 400);
    ctx.lineTo(200, 400);
    ctx.lineTo(200, 500);
    ctx.lineTo(50, 500);
    ctx.stroke();

    // 测试11: 透明度
    console.log('Test 11: Global alpha');
    ctx.globalAlpha = 0.5;
    ctx.fillStyle = '#e67e22';
    ctx.fillRect(220, 400, 100, 100);
    ctx.globalAlpha = 1.0;

    // 测试12: 组合图形
    console.log('Test 12: Complex shapes');
    ctx.fillStyle = '#9b59b6';
    ctx.beginPath();
    ctx.arc(400, 450, 60, 0, Math.PI * 2);
    ctx.arc(450, 450, 40, 0, Math.PI * 2);
    ctx.fill();

    // 测试13: 清除矩形
    ctx.clearRect(390, 440, 20, 20);
    ctx.clearRect(445, 440, 15, 15);

    // 测试14: 多个状态保存/恢复
    console.log('Test 14: Save/Restore state');
    ctx.save();
    ctx.fillStyle = '#1abc9c';
    ctx.save();
    ctx.fillStyle = '#f39c12';
    ctx.fillRect(550, 400, 50, 50);
    ctx.restore();
    ctx.fillRect(620, 400, 50, 50);
    ctx.restore();

    // 测试15: 路径组合
    console.log('Test 15: Path combinations');
    ctx.strokeStyle = '#c0392b';
    ctx.lineWidth = 2;
    ctx.beginPath();
    ctx.moveTo(700, 400);
    ctx.lineTo(750, 400);
    ctx.lineTo(750, 450);
    ctx.lineTo(700, 450);
    ctx.lineTo(700, 400);
    ctx.lineTo(725, 425);
    ctx.lineTo(750, 400);
    ctx.moveTo(725, 425);
    ctx.lineTo(750, 450);
    ctx.stroke();

    // 添加测试结果文本
    const result = document.createElement('div');
    result.style = 'text-align: center; margin: 20px; font-family: Arial;';
    result.innerHTML = `
        <h2 style="color: #27ae60;">✓ All Canvas Tests Completed!</h2>
        <p style="color: #555;">
            测试项目: 矩形、路径、圆弧、文本、变换(平移/旋转/缩放)、透明度、状态管理
        </p>
        <p style="color: #7f8c8d; font-size: 14px;">
            如果看到上面的彩色图形，说明Canvas API工作正常！
        </p>
    `;
    document.body.appendChild(result);

    console.log('Canvas test completed successfully!');
}
