/**
 * Canvas Path Methods Test - Phase 1
 * Tests: rect, arcTo, quadraticCurveTo, bezierCurveTo, ellipse, clip
 */

console.log('=== Canvas Path Methods Test ===');

const canvas = document.createElement('canvas');
canvas.width = 800;
canvas.height = 600;
document.body.appendChild(canvas);

const ctx = canvas.getContext('2d');

if (!ctx) {
    console.log('Failed to get context');
} else {
    // Test 1: rect() method
    console.log('Test 1: rect()');
    ctx.fillStyle = '#FF0000';
    ctx.beginPath();
    ctx.rect(50, 50, 100, 80);
    ctx.fill();
    console.log('  ✓ rect() passed');

    // Test 2: arcTo() method - rounded corner
    console.log('Test 2: arcTo()');
    ctx.strokeStyle = '#0000FF';
    ctx.lineWidth = 3;
    ctx.beginPath();
    ctx.moveTo(200, 50);
    ctx.arcTo(300, 50, 300, 150, 50);
    ctx.lineTo(300, 150);
    ctx.stroke();
    console.log('  ✓ arcTo() passed');

    // Test 3: quadraticCurveTo() - quadratic bezier
    console.log('Test 3: quadraticCurveTo()');
    ctx.strokeStyle = '#00FF00';
    ctx.lineWidth = 3;
    ctx.beginPath();
    ctx.moveTo(350, 130);
    ctx.quadraticCurveTo(400, 30, 500, 130);
    ctx.stroke();
    console.log('  ✓ quadraticCurveTo() passed');

    // Test 4: bezierCurveTo() - cubic bezier
    console.log('Test 4: bezierCurveTo()');
    ctx.strokeStyle = '#FF00FF';
    ctx.lineWidth = 3;
    ctx.beginPath();
    ctx.moveTo(550, 130);
    ctx.bezierCurveTo(580, 30, 720, 30, 750, 130);
    ctx.stroke();
    console.log('  ✓ bezierCurveTo() passed');

    // Test 5: ellipse() - ellipse shape
    console.log('Test 5: ellipse()');
    ctx.fillStyle = '#FFFF00';
    ctx.beginPath();
    ctx.ellipse(150, 250, 80, 40, 0, 0, Math.PI * 2);
    ctx.fill();
    console.log('  ✓ ellipse() passed');

    // Test 6: ellipse() with rotation
    console.log('Test 6: ellipse() with rotation');
    ctx.fillStyle = '#00FFFF';
    ctx.beginPath();
    ctx.ellipse(350, 250, 80, 40, Math.PI / 4, 0, Math.PI * 2);
    ctx.fill();
    console.log('  ✓ ellipse() with rotation passed');

    // Test 7: clip() - clipping region
    console.log('Test 7: clip()');
    ctx.save();
    ctx.beginPath();
    ctx.arc(600, 250, 60, 0, Math.PI * 2);
    ctx.clip();
    // Draw a checkerboard that will be clipped
    for (let i = 0; i < 6; i++) {
        for (let j = 0; j < 6; j++) {
            ctx.fillStyle = (i + j) % 2 === 0 ? '#FF0000' : '#0000FF';
            ctx.fillRect(540 + i * 20, 190 + j * 20, 20, 20);
        }
    }
    ctx.restore();
    console.log('  ✓ clip() passed');

    // Draw labels
    ctx.fillStyle = '#000000';
    ctx.fillText('rect()', 70, 150);
    ctx.fillText('arcTo()', 220, 170);
    ctx.fillText('quadratic', 400, 170);
    ctx.fillText('bezier', 640, 170);
    ctx.fillText('ellipse', 120, 320);
    ctx.fillText('rotated ellipse', 310, 320);
    ctx.fillText('clip()', 580, 340);

    console.log('=== All Phase 1 tests passed! ===');
}
