// Chart.js 简化调试
console.log('=== Chart.js Simple Debug ===');

var canvas = document.createElement('canvas');
canvas.width = 600;
canvas.height = 400;
document.body.appendChild(canvas);

console.log('Chart available:', typeof Chart);
console.log('Chart version:', Chart.version);

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
            responsive: false
        }
    });

    console.log('SUCCESS: Chart created');
    console.log('Chart type:', chart.config.type);

} catch (e) {
    console.log('FAILED:', e.message);
    if (e.stack) console.log('Stack:', e.stack);
}

console.log('=== Test Complete ===');
