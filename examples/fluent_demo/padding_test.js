// 测试 padding 对垂直居中的影响
const container = document.createElement('div');
container.style.padding = '50px';
container.style.backgroundColor = '#f0f0f0';
container.style.display = 'flex';
container.style.gap = '20px';
container.style.alignItems = 'flex-start';

// 测试1: 无 padding
const btn1 = document.createElement('button');
btn1.style.display = 'inline-flex';
btn1.style.alignItems = 'center';
btn1.style.justifyContent = 'center';
btn1.style.height = '40px';
btn1.style.minWidth = '96px';
btn1.style.padding = '0';
btn1.style.fontSize = '14px';
btn1.style.lineHeight = 'normal';
btn1.style.backgroundColor = '#0078d4';
btn1.style.color = 'white';
btn1.style.border = 'none';
const span1 = document.createElement('span');
span1.textContent = 'P=0';
btn1.appendChild(span1);

// 测试2: padding: 8px 16px (类似 Fluent large)
const btn2 = document.createElement('button');
btn2.style.display = 'inline-flex';
btn2.style.alignItems = 'center';
btn2.style.justifyContent = 'center';
btn2.style.height = '40px';
btn2.style.minWidth = '96px';
btn2.style.padding = '8px 16px';
btn2.style.fontSize = '14px';
btn2.style.lineHeight = 'normal';
btn2.style.backgroundColor = '#107c10';
btn2.style.color = 'white';
btn2.style.border = 'none';
const span2 = document.createElement('span');
span2.textContent = 'P=8 16';
btn2.appendChild(span2);

// 测试3: padding: 8px 16px + boxSizing: border-box
const btn3 = document.createElement('button');
btn3.style.display = 'inline-flex';
btn3.style.alignItems = 'center';
btn3.style.justifyContent = 'center';
btn3.style.height = '40px';
btn3.style.minWidth = '96px';
btn3.style.padding = '8px 16px';
btn3.style.fontSize = '14px';
btn3.style.lineHeight = 'normal';
btn3.style.backgroundColor = '#d83b01';
btn3.style.color = 'white';
btn3.style.border = 'none';
btn3.style.boxSizing = 'border-box';
const span3 = document.createElement('span');
span3.textContent = 'P+Box';
btn3.appendChild(span3);

container.appendChild(btn1);
container.appendChild(btn2);
container.appendChild(btn3);
document.body.appendChild(container);
