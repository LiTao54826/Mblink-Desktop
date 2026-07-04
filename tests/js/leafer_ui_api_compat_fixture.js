function assert(condition, message) {
  if (!condition) {
    throw new Error(message);
  }
}

const root = document.createElement('div');
root.id = 'leafer-api-compat';
root.style.width = '240px';
root.style.height = '160px';
document.body.style.margin = '0';
document.body.appendChild(root);

const canvas = document.createElement('canvas');
canvas.id = 'leafer-api-canvas';
canvas.width = 120;
canvas.height = 80;
canvas.style.display = 'block';
canvas.style.width = '120px';
canvas.style.height = '80px';
canvas.style.background = '#ffffff';
root.appendChild(canvas);

assert(typeof root.hasChildNodes === 'function', 'Node.hasChildNodes missing');
assert(root.hasChildNodes(), 'Node.hasChildNodes false for non-empty node');
assert(!document.createElement('div').hasChildNodes(), 'Node.hasChildNodes true for empty node');

assert(typeof CanvasRenderingContext2D === 'function', 'CanvasRenderingContext2D global missing');
const context = canvas.getContext('2d');
assert(context instanceof CanvasRenderingContext2D, '2d context is not a CanvasRenderingContext2D');
context.fillStyle = '#2f80ed';
context.fillRect(8, 8, 80, 44);
context.font = 'normal 600 28px sans-serif';
assert(context.font === '28px', 'Canvas font shorthand px parsing failed');
assert(context.measureText('direct canvas text').width > 0, 'Canvas measureText returned zero width');
context.fillStyle = '#ffffff';
context.fillText('text', 16, 40);

CanvasRenderingContext2D.prototype.__leaferCompatPatch = function() {
  return 'patched';
};
assert(context.__leaferCompatPatch() === 'patched', 'CanvasRenderingContext2D prototype patch failed');

assert(typeof Path2D === 'function', 'Path2D global missing');
const path = new Path2D();
assert(path instanceof Path2D, 'Path2D instance check failed');
Path2D.prototype.__leaferCompatPatch = function() {
  this.moveTo(0, 0);
  this.lineTo(4, 4);
  return 'patched';
};
assert(path.__leaferCompatPatch() === 'patched', 'Path2D prototype patch failed');

assert(typeof ImageData === 'function', 'ImageData global missing');
const imageData = new ImageData(2, 2);
assert(imageData instanceof ImageData, 'ImageData instance check failed');
assert(imageData.width === 2 && imageData.height === 2, 'ImageData dimensions wrong');
assert(imageData.data.length === 16, 'ImageData data length wrong');

assert(typeof CanvasGradient === 'function', 'CanvasGradient global missing');
const gradient = context.createLinearGradient(0, 0, 10, 10);
assert(gradient instanceof CanvasGradient, 'CanvasGradient instance check failed');
assert(typeof CanvasPattern === 'function', 'CanvasPattern global missing');

assert(typeof Element === 'function', 'Element global missing');
assert(typeof HTMLElement === 'function', 'HTMLElement global missing');
assert(typeof HTMLCanvasElement === 'function', 'HTMLCanvasElement global missing');
assert(canvas instanceof Element, 'canvas is not an Element');
assert(canvas instanceof HTMLElement, 'canvas is not an HTMLElement');
assert(canvas instanceof HTMLCanvasElement, 'canvas is not an HTMLCanvasElement');

assert(typeof navigator === 'object' && typeof navigator.userAgent === 'string', 'navigator.userAgent missing');
assert(typeof devicePixelRatio === 'number' && devicePixelRatio > 0, 'devicePixelRatio missing');

assert(typeof MouseEvent === 'function', 'MouseEvent global missing');
assert(typeof PointerEvent === 'function', 'PointerEvent global missing');
assert(typeof DragEvent === 'function', 'DragEvent global missing');
assert(typeof KeyboardEvent === 'function', 'KeyboardEvent global missing');
assert(new PointerEvent('pointerdown', { clientX: 7, clientY: 9 }).clientX === 7, 'PointerEvent clientX wrong');
assert(new DragEvent('dragstart').dataTransfer, 'DragEvent dataTransfer missing');
assert(new KeyboardEvent('keydown', { key: 'A', code: 'KeyA' }).key === 'A', 'KeyboardEvent key wrong');

assert(typeof ResizeObserver === 'function', 'ResizeObserver global missing');
let resizeObserved = false;
const status = document.createElement('div');
status.id = 'leafer-api-status';
status.textContent = 'waiting';
root.appendChild(status);

const observer = new ResizeObserver((entries, instance) => {
  assert(instance === observer, 'ResizeObserver callback instance wrong');
  assert(entries.length === 1, 'ResizeObserver entry count wrong');
  assert(entries[0].target === canvas, 'ResizeObserver target wrong');
  assert(typeof entries[0].contentRect.width === 'number', 'ResizeObserver contentRect missing');
  resizeObserved = true;
  status.textContent = 'ready';
  observer.disconnect();
  console.log('[leafer-api-compat] ready');
});

observer.observe(canvas);

setTimeout(() => {
  assert(resizeObserved, 'ResizeObserver did not fire');
}, 250);
