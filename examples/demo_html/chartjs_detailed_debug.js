// Chart.js 详细调试测试
console.log('=== Chart.js Detailed Debug ===');

// 重写 console.error 来捕获所有错误
var originalError = console.error;
console.error = function () {
    originalError.apply(console, arguments);
    console.log('[CAPTURED ERROR]', Array.prototype.slice.call(arguments).join(' '));
};

// 全局错误处理
globalThis.addEventListener('error', function (e) {
    console.log('[GLOBAL ERROR]', e.message);
});

var canvas = document.createElement('canvas');
canvas.width = 600;
canvas.height = 400;
document.body.appendChild(canvas);

console.log('Chart version:', Chart.version);
console.log('Creating chart...');

try {
    var chart = new Chart(canvas, {
        type: 'bar',
        data: {
            labels: ['A', 'B', 'C'],
            datasets: [{
                label: 'Test',
                data: [65, 59, 80],
                backgroundColor: [
                    'rgba(255, 99, 132, 0.7)',
                    'rgba(54, 162, 235, 0.7)',
                    'rgba(255, 206, 86, 0.7)'
                ]
            }]
        },
        options: {
            animation: false,
            responsive: false,
            plugins: {
                title: {
                    display: true,
                    text: 'Debug Test'
                }
            }
        }
    });

    console.log('Chart created!');
    console.log('Chart config:', JSON.stringify(chart.config.type));
    console.log('Chart data:', chart.data);

    // 等待一下看是否有延迟错误
    setTimeout(function () {
        console.log('After 100ms - checking chart status');
    }, 100);

} catch (e) {
    console.log('Chart creation FAILED:', e.message);
    console.log('Stack:', e.stack);
}

console.log('=== Debug Complete ===');
