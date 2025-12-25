// 多动画组合测试 - 测试 fadeIn + bounce 组合
console.log('=== 多动画组合测试 ===');

// 创建样式
var style = document.createElement('style');
style.textContent = [
    '@keyframes fadeIn {',
    '    from { opacity: 0; }',
    '    to { opacity: 1; }',
    '}',
    '@keyframes bounce {',
    '    0%, 100% { transform: translateY(0px); }',
    '    50% { transform: translateY(-50px); }',
    '}',
    '.container {',
    '    padding: 100px;',
    '    background: #f0f0f0;',
    '}',
    '.box {',
    '    width: 100px;',
    '    height: 100px;',
    '    background: #3498db;',
    '    margin: 50px;',
    '}',
    '.single-bounce {',
    '    animation: bounce 2s ease-in-out infinite;',
    '}',
    '.multi-animation {',
    '    animation: fadeIn 1s ease-out, bounce 2s ease-in-out infinite;',
    '}'
].join('\n');
document.head.appendChild(style);

// 创建容器
var container = document.createElement('div');
container.className = 'container';

// 创建标题
var title = document.createElement('h2');
title.textContent = '多动画组合测试';
container.appendChild(title);

// 创建说明
var desc = document.createElement('p');
desc.textContent = '左边是单独的 bounce 动画，右边是 fadeIn + bounce 组合';
container.appendChild(desc);

// 创建包装器
var wrapper = document.createElement('div');
wrapper.style.display = 'flex';
wrapper.style.gap = '50px';
wrapper.style.marginTop = '50px';

// 单独的 bounce 动画
var singleBox = document.createElement('div');
singleBox.className = 'box single-bounce';
singleBox.textContent = 'Bounce';
singleBox.style.color = 'white';
singleBox.style.textAlign = 'center';
singleBox.style.lineHeight = '100px';
wrapper.appendChild(singleBox);

// 多动画组合
var multiBox = document.createElement('div');
multiBox.className = 'box multi-animation';
multiBox.textContent = 'Multi';
multiBox.style.color = 'white';
multiBox.style.textAlign = 'center';
multiBox.style.lineHeight = '100px';
wrapper.appendChild(multiBox);

container.appendChild(wrapper);
document.body.appendChild(container);

console.log('渲染完成');
