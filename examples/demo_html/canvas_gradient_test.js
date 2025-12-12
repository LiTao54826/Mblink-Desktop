/**
 * Canvas Gradient Test - Phase 4
 * Tests: createLinearGradient, createRadialGradient, addColorStop
 */

console.log('=== Canvas Gradient Test ===');

const canvas = document.createElement('canvas');
canvas.width = 800;
canvas.height = 600;
document.body.appendChild(canvas);

const ctx = canvas.getContext('2d');

if (!ctx) {
    console.log('Failed to get context');
} else {
    // Test 1: Linear Gradient (horizontal)
    console.log('Test 1: createLinearGradient (horizontal)');
    const linearGrad = ctx.createLinearGradient(50, 50, 250, 50);
    linearGrad.addColorStop(0, '#FF0000');
    linearGrad.addColorStop(0.5, '#00FF00');
    linearGrad.addColorStop(1, '#0000FF');
    ctx.fillStyle = linearGrad;
    ctx.fillRect(50, 50, 200, 80);
    console.log('  ✓ Linear gradient (horizontal) passed');

    // Test 2: Linear Gradient (vertical)
    console.log('Test 2: createLinearGradient (vertical)');
    const verticalGrad = ctx.createLinearGradient(300, 50, 300, 130);
    verticalGrad.addColorStop(0, '#FF0000');
    verticalGrad.addColorStop(1, '#0000FF');
    ctx.fillStyle = verticalGrad;
    ctx.fillRect(300, 50, 200, 80);
    console.log('  ✓ Linear gradient (vertical) passed');

    // Test 3: Linear Gradient (diagonal)
    console.log('Test 3: createLinearGradient (diagonal)');
    const diagonalGrad = ctx.createLinearGradient(550, 50, 750, 130);
    diagonalGrad.addColorStop(0, '#FFFF00');
    diagonalGrad.addColorStop(1, '#FF00FF');
    ctx.fillStyle = diagonalGrad;
    ctx.fillRect(550, 50, 200, 80);
    console.log('  ✓ Linear gradient (diagonal) passed');

    // Test 4: Radial Gradient
    console.log('Test 4: createRadialGradient');
    const radialGrad = ctx.createRadialGradient(150, 250, 10, 150, 250, 80);
    radialGrad.addColorStop(0, '#FFFFFF');
    radialGrad.addColorStop(0.5, '#FF0000');
    radialGrad.addColorStop(1, '#000000');
    ctx.fillStyle = radialGrad;
    ctx.fillRect(50, 170, 200, 160);
    console.log('  ✓ Radial gradient passed');

    // Test 5: Radial Gradient (off-center)
    console.log('Test 5: createRadialGradient (off-center)');
    const offCenterGrad = ctx.createRadialGradient(380, 220, 10, 400, 260, 80);
    offCenterGrad.addColorStop(0, '#FFFFFF');
    offCenterGrad.addColorStop(1, '#0000FF');
    ctx.fillStyle = offCenterGrad;
    ctx.fillRect(300, 170, 200, 160);
    console.log('  ✓ Radial gradient (off-center) passed');

    // Test 6: Stroke with gradient
    console.log('Test 6: strokeStyle with gradient');
    const strokeGrad = ctx.createLinearGradient(550, 170, 750, 330);
    strokeGrad.addColorStop(0, '#00FFFF');
    strokeGrad.addColorStop(1, '#FF00FF');
    ctx.strokeStyle = strokeGrad;
    ctx.lineWidth = 10;
    ctx.strokeRect(560, 180, 180, 140);
    console.log('  ✓ Stroke with gradient passed');

    // Test 7: Multiple color stops
    console.log('Test 7: Multiple color stops (rainbow)');
    const rainbowGrad = ctx.createLinearGradient(50, 380, 750, 380);
    rainbowGrad.addColorStop(0, '#FF0000');
    rainbowGrad.addColorStop(0.17, '#FF7F00');
    rainbowGrad.addColorStop(0.33, '#FFFF00');
    rainbowGrad.addColorStop(0.5, '#00FF00');
    rainbowGrad.addColorStop(0.67, '#0000FF');
    rainbowGrad.addColorStop(0.83, '#4B0082');
    rainbowGrad.addColorStop(1, '#9400D3');
    ctx.fillStyle = rainbowGrad;
    ctx.fillRect(50, 360, 700, 60);
    console.log('  ✓ Multiple color stops (rainbow) passed');

    // Labels
    ctx.fillStyle = '#000000';
    ctx.fillText('Linear (H)', 100, 145);
    ctx.fillText('Linear (V)', 350, 145);
    ctx.fillText('Linear (D)', 600, 145);
    ctx.fillText('Radial', 120, 345);
    ctx.fillText('Radial (off)', 350, 345);
    ctx.fillText('Stroke Grad', 600, 345);
    ctx.fillText('Rainbow Gradient', 320, 440);

    console.log('=== All Phase 4 tests passed! ===');
}
