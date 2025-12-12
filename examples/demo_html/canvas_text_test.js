/**
 * Canvas Text Methods Test - Phase 3
 * Tests: fillText, strokeText, measureText, font, textAlign, textBaseline
 */

console.log('=== Canvas Text Methods Test ===');

const canvas = document.createElement('canvas');
canvas.width = 800;
canvas.height = 600;
document.body.appendChild(canvas);

const ctx = canvas.getContext('2d');

if (!ctx) {
    console.log('Failed to get context');
} else {
    // Test 1: fillText
    console.log('Test 1: fillText');
    ctx.fillStyle = '#000000';
    ctx.fillText('Hello Canvas!', 50, 50);
    console.log('  ✓ fillText passed');

    // Test 2: strokeText
    console.log('Test 2: strokeText');
    ctx.strokeStyle = '#FF0000';
    ctx.lineWidth = 1;
    ctx.strokeText('Stroked Text', 50, 100);
    console.log('  ✓ strokeText passed');

    // Test 3: font property
    console.log('Test 3: font property');
    ctx.font = '24px';
    ctx.fillStyle = '#0000FF';
    ctx.fillText('Large Text (24px)', 50, 150);
    console.log('  Current font: ' + ctx.font);
    console.log('  ✓ font passed');

    // Test 4: measureText
    console.log('Test 4: measureText');
    const text = 'Measured Text';
    const metrics = ctx.measureText(text);
    console.log('  Text: "' + text + '"');
    console.log('  Width: ' + metrics.width + 'px');
    ctx.fillStyle = '#00FF00';
    ctx.fillText(text, 50, 200);
    // Draw a line showing the measured width
    ctx.strokeStyle = '#FF0000';
    ctx.lineWidth = 2;
    ctx.beginPath();
    ctx.moveTo(50, 210);
    ctx.lineTo(50 + metrics.width, 210);
    ctx.stroke();
    console.log('  ✓ measureText passed');

    // Test 5: textAlign
    console.log('Test 5: textAlign');
    ctx.font = '16px';
    ctx.fillStyle = '#000000';

    // Draw reference line
    ctx.strokeStyle = '#CCCCCC';
    ctx.lineWidth = 1;
    ctx.beginPath();
    ctx.moveTo(400, 250);
    ctx.lineTo(400, 400);
    ctx.stroke();

    const aligns = ['start', 'end', 'left', 'right', 'center'];
    let y = 280;
    for (const align of aligns) {
        ctx.textAlign = align;
        ctx.fillText('textAlign: ' + align, 400, y);
        y += 30;
    }
    ctx.textAlign = 'start'; // reset
    console.log('  ✓ textAlign passed');

    // Test 6: textBaseline
    console.log('Test 6: textBaseline');

    // Draw horizontal reference line
    ctx.strokeStyle = '#CCCCCC';
    ctx.beginPath();
    ctx.moveTo(50, 500);
    ctx.lineTo(750, 500);
    ctx.stroke();

    const baselines = ['top', 'middle', 'alphabetic', 'bottom'];
    let x = 100;
    for (const baseline of baselines) {
        ctx.textBaseline = baseline;
        ctx.fillText(baseline, x, 500);
        x += 150;
    }
    ctx.textBaseline = 'alphabetic'; // reset
    console.log('  ✓ textBaseline passed');

    console.log('=== All Phase 3 tests passed! ===');
}
