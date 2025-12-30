// 测试按钮文字垂直居中
const container = document.createElement('div');
container.style.padding = '50px';
container.style.backgroundColor = '#f0f0f0';

// 测试1: 原生 button + inline-flex
const btn1 = document.createElement('button');
btn1.textContent = 'AAAA';
btn1.style.display = 'inline-flex';
btn1.style.alignItems = 'center';
btn1.style.justifyContent = 'center';
btn1.style.height = '32px';
btn1.style.padding = '0 12px';
btn1.style.fontSize = '14px';
btn1.style.lineHeight = 'normal';
btn1.style.backgroundColor = '#0078d4';
btn1.style.color = 'white';
btn1.style.border = 'none';
btn1.style.marginRight = '10px';

// 测试2: button 包含 span
const btn2 = document.createElement('button');
btn2.style.display = 'inline-flex';
btn2.style.alignItems = 'center';
btn2.style.justifyContent = 'center';
btn2.style.height = '32px';
btn2.style.padding = '0 12px';
btn2.style.fontSize = '14px';
btn2.style.lineHeight = 'normal';
btn2.style.backgroundColor = '#107c10';
btn2.style.color = 'white';
btn2.style.border = 'none';
btn2.style.marginRight = '10px';

const span2 = document.createElement('span');
span2.textContent = 'BBBB';
btn2.appendChild(span2);

// 测试3: div 模拟按钮
const btn3 = document.createElement('div');
btn3.style.display = 'inline-flex';
btn3.style.alignItems = 'center';
btn3.style.justifyContent = 'center';
btn3.style.height = '32px';
btn3.style.padding = '0 12px';
btn3.style.fontSize = '14px';
btn3.style.lineHeight = 'normal';
btn3.style.backgroundColor = '#d83b01';
btn3.style.color = 'white';

const span3 = document.createElement('span');
span3.textContent = 'CCCC';
btn3.appendChild(span3);

container.appendChild(btn1);
container.appendChild(btn2);
container.appendChild(btn3);
document.body.appendChild(container);
