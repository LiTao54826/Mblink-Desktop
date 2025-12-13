// 检查 Chart.js 需要的所有 Canvas API
console.log('=== Canvas API Compatibility Check ===');

var canvas = document.createElement('canvas');
canvas.width = 100;
canvas.height = 100;
document.body.appendChild(canvas);

var ctx = canvas.getContext('2d');

// 检查所有可能的 API
var apis = [
    'fillRect', 'strokeRect', 'clearRect',
    'beginPath', 'closePath', 'moveTo', 'lineTo',
    'arc', 'arcTo', 'bezierCurveTo', 'quadraticCurveTo',
    'rect', 'fill', 'stroke', 'clip',
    'save', 'restore',
    'scale', 'rotate', 'translate', 'transform', 'setTransform', 'resetTransform',
    'createLinearGradient', 'createRadialGradient',
    'fillText', 'strokeText', 'measureText',
    'getImageData', 'putImageData', 'createImageData',
    'drawImage',
    'setLineDash', 'getLineDash',
    'isPointInPath', 'isPointInStroke',
    // 新的 API
    'roundRect',
    'ellipse',
    'getTransform',
    'createPattern',
    'createConicGradient'
];

console.log('Checking ctx APIs:');
for (var i = 0; i < apis.length; i++) {
    var api = apis[i];
    var exists = typeof ctx[api] === 'function';
    console.log('  ' + api + ': ' + (exists ? 'OK' : 'MISSING'));
}

// 检查属性
var props = [
    'fillStyle', 'strokeStyle', 'lineWidth', 'lineCap', 'lineJoin',
    'miterLimit', 'lineDashOffset',
    'font', 'textAlign', 'textBaseline',
    'globalAlpha', 'globalCompositeOperation',
    'shadowColor', 'shadowBlur', 'shadowOffsetX', 'shadowOffsetY',
    'canvas'
];

console.log('Checking ctx properties:');
for (var i = 0; i < props.length; i++) {
    var prop = props[i];
    var exists = ctx[prop] !== undefined;
    console.log('  ' + prop + ': ' + (exists ? 'OK' : 'MISSING'));
}

console.log('=== Check Complete ===');
