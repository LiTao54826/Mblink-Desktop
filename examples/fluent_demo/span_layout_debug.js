// 调试 span 在 flex 容器中的布局
// 问题：Preact 渲染的 Fluent Button 中 span 文字偏下

const container = document.createElement('div');
container.style.padding = '20px';
container.style.backgroundColor = '#f0f0f0';
container.style.display = 'flex';
container.style.flexDirection = 'column';
container.style.gap = '20px';
document.body.appendChild(container);

// 测试1: 简单的 button > span 结构
const test1 = document.createElement('div');
test1.innerHTML = '<h4>测试1: button > span (原生DOM)</h4>';
container.appendChild(test1);

const btn1 = document.createElement('button');
btn1.style.display = 'inline-flex';
btn1.style.alignItems = 'center';
btn1.style.justifyContent = 'center';
btn1.style.height = '40px';
btn1.style.minWidth = '96px';
btn1.style.padding = '8px 16px';
btn1.style.fontSize = '16px';
btn1.style.fontFamily = "'Segoe UI', sans-serif";
btn1.style.fontWeight = '600';
btn1.style.lineHeight = 'normal';
btn1.style.boxSizing = 'border-box';
btn1.style.backgroundColor = '#0078d4';
btn1.style.color = 'white';
btn1.style.border = 'none';
btn1.style.borderRadius = '4px';

const span1 = document.createElement('span');
span1.textContent = 'Native Button';
btn1.appendChild(span1);
test1.appendChild(btn1);

// 测试2: 直接文本（无 span）
const test2 = document.createElement('div');
test2.innerHTML = '<h4>测试2: button 直接文本 (无span)</h4>';
container.appendChild(test2);

const btn2 = document.createElement('button');
btn2.style.display = 'inline-flex';
btn2.style.alignItems = 'center';
btn2.style.justifyContent = 'center';
btn2.style.height = '40px';
btn2.style.minWidth = '96px';
btn2.style.padding = '8px 16px';
btn2.style.fontSize = '16px';
btn2.style.fontFamily = "'Segoe UI', sans-serif";
btn2.style.fontWeight = '600';
btn2.style.lineHeight = 'normal';
btn2.style.boxSizing = 'border-box';
btn2.style.backgroundColor = '#107c10';
btn2.style.color = 'white';
btn2.style.border = 'none';
btn2.style.borderRadius = '4px';
btn2.textContent = 'Direct Text';
test2.appendChild(btn2);

// 测试3: 带背景色的 span 以便观察
const test3 = document.createElement('div');
test3.innerHTML = '<h4>测试3: span 带背景色</h4>';
container.appendChild(test3);

const btn3 = document.createElement('button');
btn3.style.display = 'inline-flex';
btn3.style.alignItems = 'center';
btn3.style.justifyContent = 'center';
btn3.style.height = '40px';
btn3.style.minWidth = '96px';
btn3.style.padding = '8px 16px';
btn3.style.fontSize = '16px';
btn3.style.fontFamily = "'Segoe UI', sans-serif";
btn3.style.fontWeight = '600';
btn3.style.lineHeight = 'normal';
btn3.style.boxSizing = 'border-box';
btn3.style.backgroundColor = '#d83b01';
btn3.style.color = 'white';
btn3.style.border = 'none';
btn3.style.borderRadius = '4px';

const span3 = document.createElement('span');
span3.textContent = 'Span BG';
span3.style.backgroundColor = 'rgba(255,255,0,0.5)';
btn3.appendChild(span3);
test3.appendChild(btn3);

// 输出调试信息
setTimeout(() => {
    console.log('=== 布局调试 ===');
    
    console.log('btn1 (button>span):', {
        btnHeight: btn1.offsetHeight,
        spanHeight: span1.offsetHeight,
        spanOffsetTop: span1.offsetTop
    });
    
    console.log('btn2 (直接文本):', {
        btnHeight: btn2.offsetHeight
    });
    
    console.log('btn3 (span带背景):', {
        btnHeight: btn3.offsetHeight,
        spanHeight: span3.offsetHeight,
        spanOffsetTop: span3.offsetTop
    });
}, 100);
