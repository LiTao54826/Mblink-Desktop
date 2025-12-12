// Canvas Transform Test - 测试变换方法
console.log('=== Canvas Transform Test ===');

const canvas = document.createElement('canvas');
canvas.width = 600;
canvas.height = 400;
document.body.appendChild(canvas);

const ctx = canvas.getContext('2d');

// 背景
ctx.fillStyle = '#f0f0f0';
ctx.fillRect(0, 0, 600, 400);

// 1. 测试 save/restore
console.log('Test 1: save/restore');
ctx.save();
ctx.fillStyle = 'red';
ctx.fillRect(20, 20, 50, 50);
ctx.restore();
ctx.fillRect(80, 20, 50, 50);  // 应该是灰色背景色

// 2. 测试 translate
console.log('Test 2: translate');
ctx.save();
ctx.translate(200, 20);
ctx.fillStyle = 'blue';
ctx.fillRect(0, 0, 50, 50);  // 实际位置是(200, 20)
ctx.restore();

// 3. 测试 rotate
console.log('Test 3: rotate');
ctx.save();
ctx.translate(350, 45);
ctx.rotate(Math.PI / 4);  // 45度
ctx.fillStyle = 'green';
ctx.fillRect(-25, -25, 50, 50);  // 居中旋转
ctx.restore();

// 4. 测试 scale
console.log('Test 4: scale');
ctx.save();
ctx.translate(470, 20);
ctx.scale(0.5, 0.5);
ctx.fillStyle = 'purple';
ctx.fillRect(0, 0, 100, 100);  // 实际大小是50x50
ctx.restore();

// 5. 测试 transform (组合变换)
console.log('Test 5: transform');
ctx.save();
ctx.translate(50, 150);
ctx.transform(1, 0.2, 0.2, 1, 0, 0);  // 倾斜
ctx.fillStyle = 'orange';
ctx.fillRect(0, 0, 80, 60);
ctx.restore();

// 6. 测试 setTransform (重置并设置)
console.log('Test 6: setTransform');
ctx.save();
ctx.translate(100, 100);  // 这会被覆盖
ctx.setTransform(1, 0, 0, 1, 200, 150);  // 重置为新变换
ctx.fillStyle = 'cyan';
ctx.fillRect(0, 0, 80, 60);
ctx.restore();

// 7. 测试 resetTransform
console.log('Test 7: resetTransform');
ctx.save();
ctx.translate(500, 500);  // 设置一个变换
ctx.resetTransform();  // 重置
ctx.fillStyle = 'magenta';
ctx.fillRect(350, 150, 80, 60);  // 应该在原始坐标
ctx.restore();

// 8. 测试 clip
console.log('Test 8: clip');
ctx.save();
ctx.beginPath();
ctx.arc(100, 300, 50, 0, Math.PI * 2);
ctx.clip();
ctx.fillStyle = 'yellow';
ctx.fillRect(50, 250, 100, 100);  // 只显示圆形区域
ctx.restore();

// 标签
ctx.fillStyle = '#333';
ctx.font = '14px Arial';
ctx.fillText('save/restore', 20, 90);
ctx.fillText('translate', 200, 90);
ctx.fillText('rotate', 330, 90);
ctx.fillText('scale', 460, 90);
ctx.fillText('transform', 45, 230);
ctx.fillText('setTransform', 195, 230);
ctx.fillText('resetTransform', 345, 230);
ctx.fillText('clip', 85, 370);

console.log('=== All transform tests completed! ===');
