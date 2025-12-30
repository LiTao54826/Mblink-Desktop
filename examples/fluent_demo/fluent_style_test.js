// 测试 Fluent Button 的样式
const container = document.createElement('div');
container.style.padding = '50px';
container.style.backgroundColor = '#f0f0f0';
container.style.display = 'flex';
container.style.gap = '20px';
container.style.alignItems = 'flex-start';

// 测试1: line-height: 1
const btn1 = document.createElement('button');
btn1.style.display = 'inline-flex';
btn1.style.alignItems = 'center';
btn1.style.justifyContent = 'center';
btn1.style.height = '64px';
btn1.style.padding = '0 20px';
btn1.style.fontSize = '14px';
btn1.style.lineHeight = '1';
btn1.style.backgroundColor = '#0078d4';
btn1.style.color = 'white';
btn1.style.border = 'none';
const span1 = document.createElement('span');
span1.textContent = 'LH=1';
btn1.appendChild(span1);

// 测试2: line-height: normal
const btn2 = document.createElement('button');
btn2.style.display = 'inline-flex';
btn2.style.alignItems = 'center';
btn2.style.justifyContent = 'center';
btn2.style.height = '64px';
btn2.style.padding = '0 20px';
btn2.style.fontSize = '14px';
btn2.style.lineHeight = 'normal';
btn2.style.backgroundColor = '#107c10';
btn2.style.color = 'white';
btn2.style.border = 'none';
const span2 = document.createElement('span');
span2.textContent = 'LH=N';
btn2.appendChild(span2);

// 测试3: line-height: 1.2
const btn3 = document.createElement('button');
btn3.style.display = 'inline-flex';
btn3.style.alignItems = 'center';
btn3.style.justifyContent = 'center';
btn3.style.height = '64px';
btn3.style.padding = '0 20px';
btn3.style.fontSize = '14px';
btn3.style.lineHeight = '1.2';
btn3.style.backgroundColor = '#d83b01';
btn3.style.color = 'white';
btn3.style.border = 'none';
const span3 = document.createElement('span');
span3.textContent = 'LH=1.2';
btn3.appendChild(span3);

container.appendChild(btn1);
container.appendChild(btn2);
container.appendChild(btn3);
document.body.appendChild(container);
