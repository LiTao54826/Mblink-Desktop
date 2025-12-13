// Chart.js 调试 - 检查 fillRect 是否正常工作
console.log('=== Chart.js Fill Debug ===');

var canvas = document.createElement('canvas');
canvas.width = 400;
canvas.height = 300;
document.body.appendChild(canvas);

var ctx = canvas.getContext('2d');

// 测试1: 纯色 fillRect
console.log('Test 1: Solid color fillRect');
ctx.fillStyle = '#FF0000';  // 红色
ctx.fillRect(10, 10, 80, 80);

// 测试2: rgba 颜色
console.log('Test 2: RGBA color');
ctx.fillStyle = 'rgba(0, 255, 0, 0.7)';  // 绿色半透明
ctx.fillRect(100, 10, 80, 80);

// 测试3: Chart.js 使用的格式
console.log('Test 3: Chart.js style RGBA');
ctx.fillStyle = 'rgba(54, 162, 235, 0.7)';  // 蓝色
ctx.fillRect(200, 10, 80, 80);

// 测试4: 多个柱子
console.log('Test 4: Multiple bars');
var colors = [
    'rgba(255, 99, 132, 0.7)',
    'rgba(54, 162, 235, 0.7)',
    'rgba(255, 206, 86, 0.7)',
    'rgba(75, 192, 192, 0.7)'
];
var data = [65, 59, 80, 81];
var maxVal = 100;
var barWidth = 40;
var barGap = 10;
var chartHeight = 150;
var startX = 50;
var startY = 250;

for (var i = 0; i < data.length; i++) {
    var barHeight = (data[i] / maxVal) * chartHeight;
    var x = startX + i * (barWidth + barGap);
    var y = startY - barHeight;

    ctx.fillStyle = colors[i];
    ctx.fillRect(x, y, barWidth, barHeight);
    console.log('Bar ' + i + ': x=' + x + ', y=' + y + ', w=' + barWidth + ', h=' + barHeight + ', color=' + colors[i]);
}

console.log('=== Debug Complete ===');
