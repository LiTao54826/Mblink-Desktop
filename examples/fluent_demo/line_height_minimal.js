// 最简化的 line-height 测试
const div = document.createElement('div');
div.style.width = '200px';
div.style.height = '100px';
div.style.backgroundColor = '#eee';
div.style.display = 'flex';
div.style.alignItems = 'center';
div.style.justifyContent = 'center';

const span = document.createElement('span');
span.textContent = 'Test';
span.style.lineHeight = 'normal';
span.style.fontSize = '14px';
span.style.backgroundColor = '#ff0';

div.appendChild(span);
document.body.appendChild(div);

console.log('span.style.lineHeight =', span.style.lineHeight);
