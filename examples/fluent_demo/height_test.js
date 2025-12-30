// 测试不同高度按钮的文字垂直居中
const container = document.createElement('div');
container.style.padding = '50px';
container.style.backgroundColor = '#f0f0f0';
container.style.display = 'flex';
container.style.gap = '20px';
container.style.alignItems = 'flex-start';

// 高度 32px
const btn1 = document.createElement('button');
btn1.textContent = 'H32';
btn1.style.display = 'inline-flex';
btn1.style.alignItems = 'center';
btn1.style.justifyContent = 'center';
btn1.style.height = '32px';
btn1.style.padding = '0 12px';
btn1.style.fontSize = '14px';
btn1.style.lineHeight = '1';
btn1.style.backgroundColor = '#0078d4';
btn1.style.color = 'white';
btn1.style.border = 'none';

// 高度 48px
const btn2 = document.createElement('button');
btn2.textContent = 'H48';
btn2.style.display = 'inline-flex';
btn2.style.alignItems = 'center';
btn2.style.justifyContent = 'center';
btn2.style.height = '48px';
btn2.style.padding = '0 12px';
btn2.style.fontSize = '14px';
btn2.style.lineHeight = '1';
btn2.style.backgroundColor = '#0078d4';
btn2.style.color = 'white';
btn2.style.border = 'none';

// 高度 64px
const btn3 = document.createElement('button');
btn3.textContent = 'H64';
btn3.style.display = 'inline-flex';
btn3.style.alignItems = 'center';
btn3.style.justifyContent = 'center';
btn3.style.height = '64px';
btn3.style.padding = '0 12px';
btn3.style.fontSize = '14px';
btn3.style.lineHeight = '1';
btn3.style.backgroundColor = '#0078d4';
btn3.style.color = 'white';
btn3.style.border = 'none';

// 高度 100px
const btn4 = document.createElement('button');
btn4.textContent = 'H100';
btn4.style.display = 'inline-flex';
btn4.style.alignItems = 'center';
btn4.style.justifyContent = 'center';
btn4.style.height = '100px';
btn4.style.padding = '0 12px';
btn4.style.fontSize = '14px';
btn4.style.lineHeight = '1';
btn4.style.backgroundColor = '#0078d4';
btn4.style.color = 'white';
btn4.style.border = 'none';

container.appendChild(btn1);
container.appendChild(btn2);
container.appendChild(btn3);
container.appendChild(btn4);
document.body.appendChild(container);
