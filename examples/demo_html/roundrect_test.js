// roundRect API 测试
console.log('=== roundRect API Test ===');

var canvas = document.createElement('canvas');
canvas.width = 600;
canvas.height = 400;
document.body.appendChild(canvas);

var ctx = canvas.getContext('2d');

// 背景
ctx.fillStyle = '#f0f0f0';
ctx.fillRect(0, 0, 600, 400);

console.log('Testing roundRect API...');
console.log('roundRect exists:', typeof ctx.roundRect);

if (typeof ctx.roundRect === 'function') {
    console.log('roundRect is available!');

    // 测试1: 使用 roundRect 绘制圆角矩形
    ctx.beginPath();
    ctx.roundRect(50, 50, 100, 150, 10);
    ctx.fillStyle = 'rgba(255, 99, 132, 0.7)';
    ctx.fill();
    console.log('Test 1: roundRect with fill - DONE');

    // 测试2: 描边圆角矩形
    ctx.beginPath();
    ctx.roundRect(200, 50, 100, 150, 15);
    ctx.strokeStyle = 'rgb(54, 162, 235)';
    ctx.lineWidth = 3;
    ctx.stroke();
    console.log('Test 2: roundRect with stroke - DONE');

    // 测试3: 填充+描边圆角矩形
    ctx.beginPath();
    ctx.roundRect(350, 50, 100, 150, 20);
    ctx.fillStyle = 'rgba(255, 206, 86, 0.7)';
    ctx.fill();
    ctx.strokeStyle = 'rgb(255, 206, 86)';
    ctx.lineWidth = 2;
    ctx.stroke();
    console.log('Test 3: roundRect with fill and stroke - DONE');

    // 测试4: 大圆角
    ctx.beginPath();
    ctx.roundRect(50, 250, 100, 100, 50);
    ctx.fillStyle = 'rgba(75, 192, 192, 0.7)';
    ctx.fill();
    console.log('Test 4: roundRect with large radius - DONE');

    // 测试5: 模拟 Chart.js 柱状图
    var barY = 250;
    var barHeight = 120;
    var barWidth = 40;
    var barColors = [
        'rgba(255, 99, 132, 0.7)',
        'rgba(54, 162, 235, 0.7)',
        'rgba(255, 206, 86, 0.7)',
        'rgba(75, 192, 192, 0.7)',
        'rgba(153, 102, 255, 0.7)'
    ];

    for (var i = 0; i < 5; i++) {
        var x = 200 + i * 50;
        var y = barY + (100 - barHeight);

        ctx.beginPath();
        ctx.roundRect(x, y, barWidth, barHeight, 5);
        ctx.fillStyle = barColors[i];
        ctx.fill();
        console.log('Bar ' + i + ' drawn at x=' + x);
    }
    console.log('Test 5: Chart.js style bars - DONE');

} else {
    console.log('ERROR: roundRect is not available!');
    console.log('Available methods:', Object.keys(ctx).filter(k => typeof ctx[k] === 'function').join(', '));
}

console.log('=== Test Complete ===');
