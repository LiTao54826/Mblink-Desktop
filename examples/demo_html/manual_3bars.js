// 手动模拟 Chart.js 绘制3个柱子
console.log('=== Manual 3-Bar Test ===');

var canvas = document.createElement('canvas');
canvas.width = 600;
canvas.height = 400;
document.body.appendChild(canvas);

var ctx = canvas.getContext('2d');

// 背景
ctx.fillStyle = '#ffffff';
ctx.fillRect(0, 0, 600, 400);

// 绘制网格
ctx.strokeStyle = '#e0e0e0';
ctx.lineWidth = 1;
for (var i = 0; i < 5; i++) {
    var y = 80 + i * 60;
    ctx.beginPath();
    ctx.moveTo(50, y);
    ctx.lineTo(550, y);
    ctx.stroke();
}

// 柱子数据
var bars = [
    { x: 100, height: 150, color: 'rgba(255, 99, 132, 0.7)', label: 'A' },
    { x: 250, height: 120, color: 'rgba(54, 162, 235, 0.7)', label: 'B' },
    { x: 400, height: 180, color: 'rgba(255, 206, 86, 0.7)', label: 'C' }
];

var barWidth = 80;
var baseY = 320;

console.log('Drawing 3 bars with roundRect...');

for (var i = 0; i < bars.length; i++) {
    var bar = bars[i];
    var y = baseY - bar.height;

    console.log('Bar ' + i + ': ' + bar.label + ' at x=' + bar.x + ', y=' + y + ', height=' + bar.height);

    try {
        ctx.beginPath();
        ctx.roundRect(bar.x, y, barWidth, bar.height, 8);
        ctx.fillStyle = bar.color;
        ctx.fill();
        console.log('  ✓ Bar ' + i + ' drawn successfully');
    } catch (e) {
        console.log('  ✗ Bar ' + i + ' FAILED:', e.message);
    }
}

console.log('=== Test Complete ===');
