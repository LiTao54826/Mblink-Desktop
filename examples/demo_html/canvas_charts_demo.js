// Canvas Charts Demo - 饼图、柱状图、折线图
console.log('=== Canvas Charts Demo ===');

// 创建canvas
const canvas = document.createElement('canvas');
canvas.width = 800;
canvas.height = 500;
document.body.appendChild(canvas);

const ctx = canvas.getContext('2d');

// 清空背景
ctx.fillStyle = '#f5f5f5';
ctx.fillRect(0, 0, 800, 500);

// ========== 饼图 (左侧) ==========
console.log('Drawing pie chart...');

const pieData = [
    { value: 30, color: '#FF6384', label: 'Red' },
    { value: 25, color: '#36A2EB', label: 'Blue' },
    { value: 20, color: '#FFCE56', label: 'Yellow' },
    { value: 15, color: '#4BC0C0', label: 'Teal' },
    { value: 10, color: '#9966FF', label: 'Purple' }
];

const pieX = 150;
const pieY = 180;
const pieRadius = 100;
let startAngle = 0;
const total = pieData.reduce((sum, d) => sum + d.value, 0);

for (const slice of pieData) {
    const sliceAngle = (slice.value / total) * 2 * Math.PI;

    ctx.beginPath();
    ctx.moveTo(pieX, pieY);
    ctx.arc(pieX, pieY, pieRadius, startAngle, startAngle + sliceAngle);
    ctx.closePath();

    ctx.fillStyle = slice.color;
    ctx.fill();

    ctx.strokeStyle = '#ffffff';
    ctx.lineWidth = 2;
    ctx.stroke();

    startAngle += sliceAngle;
}

// 饼图标题
ctx.fillStyle = '#333333';
ctx.font = '16px Arial';
ctx.textAlign = 'center';
ctx.fillText('Pie Chart', pieX, pieY + pieRadius + 30);

console.log('  ✓ Pie chart completed');

// ========== 柱状图 (中间) ==========
console.log('Drawing bar chart...');

const barData = [65, 85, 45, 75, 55, 90];
const barColors = ['#FF6384', '#36A2EB', '#FFCE56', '#4BC0C0', '#9966FF', '#FF9F40'];
const barX = 320;
const barY = 280;
const barWidth = 30;
const barGap = 10;
const maxBarHeight = 200;
const maxValue = Math.max(...barData);

// 绘制坐标轴
ctx.strokeStyle = '#333333';
ctx.lineWidth = 2;
ctx.beginPath();
ctx.moveTo(barX - 10, barY);
ctx.lineTo(barX + (barWidth + barGap) * barData.length, barY);
ctx.moveTo(barX - 10, barY);
ctx.lineTo(barX - 10, barY - maxBarHeight - 20);
ctx.stroke();

// 绘制柱子
for (let i = 0; i < barData.length; i++) {
    const height = (barData[i] / maxValue) * maxBarHeight;
    const x = barX + i * (barWidth + barGap);
    const y = barY - height;

    ctx.fillStyle = barColors[i];
    ctx.fillRect(x, y, barWidth, height);

    // 数值标签
    ctx.fillStyle = '#333333';
    ctx.font = '12px Arial';
    ctx.textAlign = 'center';
    ctx.fillText(barData[i].toString(), x + barWidth / 2, y - 5);
}

// 柱状图标题
ctx.fillStyle = '#333333';
ctx.font = '16px Arial';
ctx.textAlign = 'center';
ctx.fillText('Bar Chart', barX + (barWidth + barGap) * barData.length / 2, barY + 30);

console.log('  ✓ Bar chart completed');

// ========== 折线图 (右侧) ==========
console.log('Drawing line chart...');

const lineData = [30, 60, 45, 80, 55, 70, 90];
const lineX = 580;
const lineY = 280;
const lineWidth2 = 180;
const lineHeight = 200;
const pointGap = lineWidth2 / (lineData.length - 1);
const maxLineValue = Math.max(...lineData);

// 绘制坐标轴
ctx.strokeStyle = '#333333';
ctx.lineWidth = 2;
ctx.beginPath();
ctx.moveTo(lineX, lineY);
ctx.lineTo(lineX + lineWidth2 + 20, lineY);
ctx.moveTo(lineX, lineY);
ctx.lineTo(lineX, lineY - lineHeight - 20);
ctx.stroke();

// 绘制折线
ctx.strokeStyle = '#36A2EB';
ctx.lineWidth = 3;
ctx.beginPath();

for (let i = 0; i < lineData.length; i++) {
    const x = lineX + i * pointGap;
    const y = lineY - (lineData[i] / maxLineValue) * lineHeight;

    if (i === 0) {
        ctx.moveTo(x, y);
    } else {
        ctx.lineTo(x, y);
    }
}
ctx.stroke();

// 绘制数据点
for (let i = 0; i < lineData.length; i++) {
    const x = lineX + i * pointGap;
    const y = lineY - (lineData[i] / maxLineValue) * lineHeight;

    ctx.beginPath();
    ctx.arc(x, y, 5, 0, 2 * Math.PI);
    ctx.fillStyle = '#36A2EB';
    ctx.fill();
    ctx.strokeStyle = '#ffffff';
    ctx.lineWidth = 2;
    ctx.stroke();
}

// 折线图标题
ctx.fillStyle = '#333333';
ctx.font = '16px Arial';
ctx.textAlign = 'center';
ctx.fillText('Line Chart', lineX + lineWidth2 / 2, lineY + 30);

console.log('  ✓ Line chart completed');

// ========== 图例 ==========
console.log('Drawing legend...');

ctx.fillStyle = '#333333';
ctx.font = 'bold 20px Arial';
ctx.textAlign = 'center';
ctx.fillText('Canvas Charts Demo', 400, 30);

// 底部说明
ctx.font = '14px Arial';
ctx.fillStyle = '#666666';
ctx.fillText('Demonstrates: arc(), fillRect(), lineTo(), moveTo(), fill(), stroke()', 400, 480);

console.log('=== All charts rendered successfully! ===');
