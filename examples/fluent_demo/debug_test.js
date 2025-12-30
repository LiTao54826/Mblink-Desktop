/**
 * 调试测试 - 测试 createElementNS
 */

console.log('=== createElementNS Test ===');

// 直接测试 createElementNS
console.log('Testing document.createElementNS...');
try {
  var svg = document.createElementNS('http://www.w3.org/2000/svg', 'svg');
  console.log('SVG element created:', svg);
  console.log('SVG tagName:', svg.tagName);
  
  svg.setAttribute('width', '100');
  svg.setAttribute('height', '100');
  console.log('SVG attributes set');
  
  var circle = document.createElementNS('http://www.w3.org/2000/svg', 'circle');
  console.log('Circle element created:', circle);
  circle.setAttribute('cx', '50');
  circle.setAttribute('cy', '50');
  circle.setAttribute('r', '40');
  circle.setAttribute('fill', 'blue');
  console.log('Circle attributes set');
  
  svg.appendChild(circle);
  console.log('Circle appended to SVG');
  
  document.body.appendChild(svg);
  console.log('SVG appended to body');
  
  console.log('document.body.childNodes.length:', document.body.childNodes.length);
  console.log('SUCCESS: createElementNS works!');
} catch (e) {
  console.log('ERROR:', e);
  console.log('Error message:', e.message);
  console.log('Error stack:', e.stack);
}
