/**
 * 测试fillRect
 */

console.log('Creating canvas...');
const canvas = document.createElement('canvas');
canvas.width = 400;
canvas.height = 300;
document.body.appendChild(canvas);

const ctx = canvas.getContext('2d');

if (ctx) {
    console.log('Got context');

    try {
        console.log('Setting fillStyle...');
        ctx.fillStyle = '#ff0000';
        console.log('fillStyle set to:', ctx.fillStyle);

        console.log('Calling fillRect...');
        ctx.fillRect(50, 50, 100, 100);
        console.log('✓ fillRect succeeded!');

    } catch (e) {
        console.log('✗ Error:', e.toString());
    }
} else {
    console.log('No context');
}

console.log('Test complete');
