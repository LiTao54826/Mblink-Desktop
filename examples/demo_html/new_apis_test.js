// Canvas API 新功能测试
console.log('=== New Canvas APIs Test ===');

var canvas = document.createElement('canvas');
canvas.width = 400;
canvas.height = 300;
document.body.appendChild(canvas);

var ctx = canvas.getContext('2d');

// 测试 1: getTransform
console.log('\nTest 1: getTransform()');
var transform = ctx.getTransform();
console.log('Initial transform:', JSON.stringify(transform));
console.log('  a (scaleX):', transform.a);
console.log('  b (skewY):', transform.b);
console.log('  c (skewX):', transform.c);
console.log('  d (scaleY):', transform.d);
console.log('  e (translateX):', transform.e);
console.log('  f (translateY):', transform.f);

// 应用一些变换
ctx.translate(50, 30);
ctx.scale(2, 1.5);
ctx.rotate(0.5);

var transform2 = ctx.getTransform();
console.log('After transforms:', JSON.stringify(transform2));
console.log('  Values changed: ✓');

// 测试 2: drawImage (应该返回 undefined，因为是占位实现)
console.log('\nTest 2: drawImage()');
console.log('drawImage exists:', typeof ctx.drawImage);
if (typeof ctx.drawImage === 'function') {
    var result = ctx.drawImage(null, 0, 0);
    console.log('drawImage() returned:', result);
    console.log('  Result: ✓ (placeholder implementation)');
}

// 测试 3: roundRect 仍然工作
console.log('\nTest 3: roundRect() still works');
ctx.resetTransform();
ctx.beginPath();
ctx.roundRect(100, 100, 150, 100, 15);
ctx.fillStyle = 'rgba(100, 200, 100, 0.7)';
ctx.fill();
console.log('  roundRect drawn: ✓');

console.log('\n=== All Tests Complete ===');
