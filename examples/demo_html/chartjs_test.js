// Chart.js Integration Test - ES6 Import
import Chart from '../js/chart.js';

console.log('=== Chart.js Integration Test ===');

// 检查Chart是否可用
if (typeof Chart === 'undefined') {
    console.log('ERROR: Chart.js not loaded');
} else {
    console.log('Chart.js loaded successfully!');
    console.log('Chart version:', Chart.version);

    // 创建Canvas
    const canvas = document.createElement('canvas');
    canvas.id = 'myChart';
    canvas.width = 700;
    canvas.height = 400;
    document.body.appendChild(canvas);

    try {
        // 创建Chart.js图表
        const chart = new Chart(canvas, {
            type: 'bar',
            data: {
                labels: ['January', 'February', 'March', 'April', 'May', 'June'],
                datasets: [{
                    label: 'Sales 2024',
                    data: [65, 59, 80, 81, 56, 55],
                    backgroundColor: [
                        'rgba(255, 99, 132, 0.7)',
                        'rgba(54, 162, 235, 0.7)',
                        'rgba(255, 206, 86, 0.7)',
                        'rgba(75, 192, 192, 0.7)',
                        'rgba(153, 102, 255, 0.7)',
                        'rgba(255, 159, 64, 0.7)'
                    ],
                    borderColor: [
                        'rgba(255, 99, 132, 1)',
                        'rgba(54, 162, 235, 1)',
                        'rgba(255, 206, 86, 1)',
                        'rgba(75, 192, 192, 1)',
                        'rgba(153, 102, 255, 1)',
                        'rgba(255, 159, 64, 1)'
                    ],
                    borderWidth: 2
                }]
            },
            options: {
                animation: false,
                responsive: false,
                plugins: {
                    title: {
                        display: true,
                        text: 'Monthly Sales Report'
                    }
                }
            }
        });

        console.log('Chart.js chart created successfully!');
    } catch (e) {
        console.log('Error creating Chart.js chart:', e.message);
        console.log('Stack:', e.stack);
    }
}

console.log('=== Test Complete ===');
