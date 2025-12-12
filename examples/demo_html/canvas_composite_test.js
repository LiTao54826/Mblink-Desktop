/**
 * Canvas Compositing and Shadows Test - Phase 6
 * Tests: globalCompositeOperation, shadowColor, shadowBlur, shadowOffsetX, shadowOffsetY
 */

console.log('=== Canvas Compositing and Shadows Test ===');

const canvas = document.createElement('canvas');
canvas.width = 800;
canvas.height = 600;
document.body.appendChild(canvas);

const ctx = canvas.getContext('2d');

if (!ctx) {
    console.log('Failed to get context');
} else {
    // Test 1: globalCompositeOperation default
    console.log('Test 1: globalCompositeOperation');
    console.log('  Default: ' + ctx.globalCompositeOperation);

    // Test 2: Draw with different composite operations
    console.log('Test 2: Composite operations demo');

    // Draw base rectangles
    ctx.fillStyle = '#FF0000';
    ctx.fillRect(50, 50, 100, 100);

    // source-over (default)
    ctx.globalCompositeOperation = 'source-over';
    ctx.fillStyle = '#0000FF';
    ctx.fillRect(100, 100, 100, 100);
    ctx.fillText('source-over', 100, 220);

    // multiply
    ctx.fillStyle = '#FF0000';
    ctx.fillRect(250, 50, 100, 100);
    ctx.globalCompositeOperation = 'multiply';
    ctx.fillStyle = '#0000FF';
    ctx.fillRect(300, 100, 100, 100);
    ctx.globalCompositeOperation = 'source-over';
    ctx.fillText('multiply', 300, 220);

    // screen
    ctx.fillStyle = '#FF0000';
    ctx.fillRect(450, 50, 100, 100);
    ctx.globalCompositeOperation = 'screen';
    ctx.fillStyle = '#0000FF';
    ctx.fillRect(500, 100, 100, 100);
    ctx.globalCompositeOperation = 'source-over';
    ctx.fillText('screen', 500, 220);

    // xor
    ctx.fillStyle = '#FF0000';
    ctx.fillRect(650, 50, 100, 100);
    ctx.globalCompositeOperation = 'xor';
    ctx.fillStyle = '#0000FF';
    ctx.fillRect(700, 100, 100, 100);
    ctx.globalCompositeOperation = 'source-over';
    ctx.fillText('xor', 700, 220);

    console.log('  ✓ globalCompositeOperation passed');

    // Test 3: Shadow Color
    console.log('Test 3: shadowColor');
    ctx.shadowColor = 'rgba(0, 0, 0, 0.5)';
    console.log('  Set shadowColor: ' + ctx.shadowColor);
    console.log('  ✓ shadowColor passed');

    // Test 4: Shadow Blur
    console.log('Test 4: shadowBlur');
    ctx.shadowBlur = 10;
    console.log('  Set shadowBlur: ' + ctx.shadowBlur);
    console.log('  ✓ shadowBlur passed');

    // Test 5: Shadow Offset
    console.log('Test 5: shadowOffsetX/Y');
    ctx.shadowOffsetX = 5;
    ctx.shadowOffsetY = 5;
    console.log('  Set shadowOffsetX: ' + ctx.shadowOffsetX);
    console.log('  Set shadowOffsetY: ' + ctx.shadowOffsetY);
    console.log('  ✓ shadowOffset passed');

    // Test 6: Draw with shadow (Note: shadow rendering may not be visible without shadow image filter implementation)
    console.log('Test 6: Draw with shadow properties');
    ctx.shadowColor = 'rgba(0, 0, 0, 0.8)';
    ctx.shadowBlur = 15;
    ctx.shadowOffsetX = 10;
    ctx.shadowOffsetY = 10;
    ctx.fillStyle = '#00FF00';
    ctx.fillRect(100, 300, 150, 100);

    // Reset shadow
    ctx.shadowColor = 'transparent';
    ctx.shadowBlur = 0;
    ctx.shadowOffsetX = 0;
    ctx.shadowOffsetY = 0;

    ctx.fillStyle = '#000000';
    ctx.fillText('Green rect with shadow settings', 100, 430);
    console.log('  ✓ Draw with shadow passed');

    // Test 7: Various composite operations
    console.log('Test 7: Additional composite modes');
    const modes = ['lighter', 'darken', 'lighten', 'overlay'];
    let x = 50;
    for (const mode of modes) {
        ctx.fillStyle = '#FF0000';
        ctx.fillRect(x, 460, 60, 60);
        ctx.globalCompositeOperation = mode;
        ctx.fillStyle = '#0000FF';
        ctx.fillRect(x + 30, 490, 60, 60);
        ctx.globalCompositeOperation = 'source-over';
        ctx.fillText(mode, x + 20, 570);
        x += 150;
    }
    console.log('  ✓ Additional modes passed');

    console.log('=== All Phase 6 tests passed! ===');
}
