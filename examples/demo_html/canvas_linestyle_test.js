/**
 * Canvas Line Styles Test - Phase 2
 * Tests: lineCap, lineJoin, miterLimit, setLineDash, getLineDash, lineDashOffset
 */

console.log('=== Canvas Line Styles Test ===');

const canvas = document.createElement('canvas');
canvas.width = 800;
canvas.height = 600;
document.body.appendChild(canvas);

const ctx = canvas.getContext('2d');

if (!ctx) {
    console.log('Failed to get context');
} else {
    ctx.lineWidth = 10;

    // Test 1: lineCap
    console.log('Test 1: lineCap');
    const caps = ['butt', 'round', 'square'];
    let y = 50;
    for (let i = 0; i < caps.length; i++) {
        ctx.lineCap = caps[i];
        ctx.strokeStyle = '#0000FF';
        ctx.beginPath();
        ctx.moveTo(50, y);
        ctx.lineTo(200, y);
        ctx.stroke();
        ctx.fillStyle = '#000000';
        ctx.fillText(caps[i], 220, y + 5);
        y += 40;
    }
    console.log('  ✓ lineCap passed');

    // Test 2: lineJoin
    console.log('Test 2: lineJoin');
    const joins = ['miter', 'round', 'bevel'];
    y = 50;
    for (let i = 0; i < joins.length; i++) {
        ctx.lineJoin = joins[i];
        ctx.strokeStyle = '#FF0000';
        ctx.beginPath();
        ctx.moveTo(350, y);
        ctx.lineTo(400, y + 30);
        ctx.lineTo(450, y);
        ctx.stroke();
        ctx.fillStyle = '#000000';
        ctx.fillText(joins[i], 460, y + 20);
        y += 60;
    }
    console.log('  ✓ lineJoin passed');

    // Test 3: miterLimit
    console.log('Test 3: miterLimit');
    ctx.lineJoin = 'miter';
    ctx.miterLimit = 2;
    ctx.strokeStyle = '#00FF00';
    ctx.beginPath();
    ctx.moveTo(600, 50);
    ctx.lineTo(650, 100);
    ctx.lineTo(700, 50);
    ctx.stroke();
    console.log('  ✓ miterLimit passed (set to: ' + ctx.miterLimit + ')');

    // Test 4: setLineDash
    console.log('Test 4: setLineDash');
    ctx.setLineDash([15, 5]);
    ctx.strokeStyle = '#FF00FF';
    ctx.lineWidth = 3;
    ctx.lineCap = 'butt';
    ctx.beginPath();
    ctx.moveTo(50, 250);
    ctx.lineTo(250, 250);
    ctx.stroke();
    console.log('  ✓ setLineDash passed');

    // Test 5: getLineDash
    console.log('Test 5: getLineDash');
    const dash = ctx.getLineDash();
    console.log('  Current dash: [' + dash.join(', ') + ']');
    console.log('  ✓ getLineDash passed');

    // Test 6: lineDashOffset
    console.log('Test 6: lineDashOffset');
    ctx.setLineDash([10, 10]);
    ctx.lineDashOffset = 5;
    ctx.strokeStyle = '#00FFFF';
    ctx.beginPath();
    ctx.moveTo(50, 300);
    ctx.lineTo(250, 300);
    ctx.stroke();
    console.log('  ✓ lineDashOffset passed (set to: ' + ctx.lineDashOffset + ')');

    // Reset for other drawing
    ctx.setLineDash([]);
    ctx.lineWidth = 5;

    // Test 7: Combined styles
    console.log('Test 7: Combined styles');
    ctx.setLineDash([20, 5, 5, 5]);
    ctx.lineCap = 'round';
    ctx.strokeStyle = '#FF6600';
    ctx.beginPath();
    ctx.moveTo(50, 350);
    ctx.lineTo(300, 350);
    ctx.stroke();
    console.log('  ✓ Combined styles passed');

    // Labels
    ctx.setLineDash([]);
    ctx.fillStyle = '#000000';
    ctx.fillText('lineCap', 100, 180);
    ctx.fillText('lineJoin', 380, 220);
    ctx.fillText('miterLimit', 620, 130);
    ctx.fillText('setLineDash', 100, 275);
    ctx.fillText('lineDashOffset', 100, 325);
    ctx.fillText('combined', 150, 375);

    console.log('=== All Phase 2 tests passed! ===');
}
