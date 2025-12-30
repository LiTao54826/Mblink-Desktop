// 精确调试 flex 容器中 span 的垂直居中问题
// 关键：对比 span 的测量高度和实际渲染位置

const container = document.createElement('div');
container.style.padding = '20px';
container.style.backgroundColor = '#f0f0f0';
document.body.appendChild(container);

// 创建一个简单的 flex 容器 + span
const flexBox = document.createElement('div');
flexBox.style.display = 'inline-flex';
flexBox.style.alignItems = 'center';
flexBox.style.justifyContent = 'center';
flexBox.style.height = '40px';
flexBox.style.width = '150px';
flexBox.style.backgroundColor = '#0078d4';
flexBox.style.color = 'white';
flexBox.style.fontSize = '16px';
flexBox.style.fontFamily = "'Segoe UI', sans-serif";
flexBox.style.fontWeight = '600';
flexBox.style.lineHeight = 'normal';

const span = document.createElement('span');
span.textContent = 'Test';
span.style.backgroundColor = 'rgba(255,255,0,0.3)';  // 黄色半透明背景便于观察
flexBox.appendChild(span);

container.appendChild(flexBox);

// 添加参考线
const refLine = document.createElement('div');
refLine.style.position = 'absolute';
refLine.style.left = '20px';
refLine.style.width = '150px';
refLine.style.height = '1px';
refLine.style.backgroundColor = 'red';
// 计算中心线位置
setTimeout(() => {
    const rect = flexBox.getBoundingClientRect();
    refLine.style.top = (rect.top + rect.height / 2) + 'px';
    document.body.appendChild(refLine);
    
    // 输出调试信息
    const spanRect = span.getBoundingClientRect();
    console.log('=== Flex Span 调试 ===');
    console.log('flexBox:', {
        height: flexBox.offsetHeight,
        rect: rect
    });
    console.log('span:', {
        height: span.offsetHeight,
        rect: spanRect,
        offsetTop: span.offsetTop
    });
    console.log('span 中心 Y:', spanRect.top + spanRect.height / 2);
    console.log('flexBox 中心 Y:', rect.top + rect.height / 2);
    console.log('偏移量:', (spanRect.top + spanRect.height / 2) - (rect.top + rect.height / 2));
}, 100);
