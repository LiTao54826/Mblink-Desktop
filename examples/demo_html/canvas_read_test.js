/**
 * 测试读取fillStyle
 */

console.log('Getting canvas...');
const canvas = document.createElement('canvas');
canvas.width = 400;
canvas.height = 300;
document.body.appendChild(canvas);

console.log('Getting context...');
const ctx = canvas.getContext('2d');

console.log('Got context:', ctx);

if (ctx) {
    try {
        console.log('Reading fill Style...');
        const style = ctx.fillStyle;
        console.log('fillStyle:', style);
        console.log('✓ Read succeeded!');
    } catch (e) {
        console.log('✗ Error reading fillStyle:', e.toString());
    }
} else {
    console.log('✗ No context');
}

console.log('Done');
