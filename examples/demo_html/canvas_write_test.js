/**
 * 测试写入fillStyle
 */

console.log('Getting canvas...');
const canvas = document.createElement('canvas');
canvas.width = 400;
canvas.height = 300;
document.body.appendChild(canvas);

console.log('Getting context...');
const ctx = canvas.getContext('2d');

console.log('Got context');

if (ctx) {
    try {
        console.log('Current fillStyle:', ctx.fillStyle);

        console.log('Setting fillStyle to #ff0000...');
        ctx.fillStyle = '#ff0000';

        console.log('Reading it back...');
        console.log('New fillStyle:', ctx.fillStyle);

        console.log('✓ Write/Read succeeded!');
    } catch (e) {
        console.log('✗ Error:', e.toString());
        console.log('Stack:', e.stack);
    }
} else {
    console.log('✗ No context');
}

console.log('Test complete');
