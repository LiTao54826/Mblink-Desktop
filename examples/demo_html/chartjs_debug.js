// Chart.js Debug Test - 检查 canvas.getContext
console.log('=== Chart.js Debug Test ===');

// 检查Chart是否可用
if (typeof Chart === 'undefined') {
    console.log('ERROR: Chart.js not loaded');
} else {
    console.log('Chart.js version:', Chart.version);

    // 创建Canvas
    var canvas = document.createElement('canvas');
    canvas.id = 'myChart';
    canvas.width = 700;
    canvas.height = 400;
    document.body.appendChild(canvas);

    console.log('Canvas created:', canvas);
    console.log('Canvas id:', canvas.id);
    console.log('Canvas width:', canvas.width);
    console.log('Canvas height:', canvas.height);

    // 测试 getContext
    var ctx = canvas.getContext('2d');
    console.log('getContext("2d"):', ctx);
    console.log('ctx type:', typeof ctx);

    if (ctx) {
        console.log('Context is valid!');
        console.log('ctx.canvas:', ctx.canvas);

        // 测试直接绘制
        ctx.fillStyle = 'blue';
        ctx.fillRect(0, 0, 100, 100);
        console.log('Direct drawing test passed!');

        // 现在测试 Chart.js
        try {
            var chart = new Chart(canvas, {
                type: 'bar',
                data: {
                    labels: ['A', 'B', 'C'],
                    datasets: [{
                        label: 'Test',
                        data: [10, 20, 30],
                        backgroundColor: 'rgba(75, 192, 192, 0.7)'
                    }]
                },
                options: {
                    animation: false,
                    responsive: false
                }
            });
            console.log('Chart created successfully!');
        } catch (e) {
            console.log('Chart creation error:', e.message);
            console.log('Stack:', e.stack);
        }
    } else {
        console.log('ERROR: Could not get 2d context!');
    }
}

console.log('=== Debug Complete ===');
