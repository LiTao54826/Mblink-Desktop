/**
 * 水平滚动条测试示例
 * 测试固定宽度内容超出容器时的水平滚动条行为
 */

// 创建容器
const container = document.createElement('div');
container.style.cssText = `
    width: 400px;
    height: 300px;
    overflow: auto;
    border: 2px solid #333;
    margin: 20px;
    background: #f5f5f5;
`;
document.body.appendChild(container);

// 创建超宽内容 - 使用 min-width 确保不被压缩
const wideContent = document.createElement('div');
wideContent.style.cssText = `
    min-width: 800px;
    height: 200px;
    background: linear-gradient(90deg, #4CAF50, #2196F3, #9C27B0, #FF5722);
`;

// 添加一些标记 - 使用 inline-block 横向排列
for (let i = 1; i <= 4; i++) {
    const marker = document.createElement('div');
    marker.style.cssText = `
        width: 150px;
        height: 80px;
        margin: 60px 20px;
        background: white;
        border-radius: 8px;
        display: inline-block;
        text-align: center;
        line-height: 80px;
        font-size: 24px;
        font-weight: bold;
        color: #333;
    `;
    marker.textContent = i;
    wideContent.appendChild(marker);
}

container.appendChild(wideContent);

// 创建信息面板
const info = document.createElement('div');
info.style.cssText = `
    margin: 20px;
    padding: 15px;
    background: #e3f2fd;
    border-radius: 8px;
    font-family: monospace;
`;
document.body.appendChild(info);

// 更新信息
function updateInfo() {
    info.innerHTML = `
        <div><b>容器尺寸:</b> ${container.clientWidth} x ${container.clientHeight}</div>
        <div><b>内容尺寸:</b> ${wideContent.offsetWidth} x ${wideContent.offsetHeight}</div>
        <div><b>scrollWidth:</b> ${container.scrollWidth}</div>
        <div><b>scrollHeight:</b> ${container.scrollHeight}</div>
        <div><b>scrollLeft:</b> ${container.scrollLeft}</div>
        <div><b>scrollTop:</b> ${container.scrollTop}</div>
    `;
}

updateInfo();

// 监听滚动事件
container.addEventListener('scroll', updateInfo);

// 创建控制按钮
const controls = document.createElement('div');
controls.style.cssText = `
    margin: 20px;
    display: flex;
    gap: 10px;
`;
document.body.appendChild(controls);

const btnLeft = document.createElement('button');
btnLeft.textContent = '← 滚动到左边';
btnLeft.style.cssText = 'padding: 10px 20px; cursor: pointer;';
btnLeft.onclick = () => { container.scrollLeft = 0; updateInfo(); };
controls.appendChild(btnLeft);

const btnRight = document.createElement('button');
btnRight.textContent = '滚动到右边 →';
btnRight.style.cssText = 'padding: 10px 20px; cursor: pointer;';
btnRight.onclick = () => { container.scrollLeft = container.scrollWidth; updateInfo(); };
controls.appendChild(btnRight);

const btnCenter = document.createElement('button');
btnCenter.textContent = '滚动到中间';
btnCenter.style.cssText = 'padding: 10px 20px; cursor: pointer;';
btnCenter.onclick = () => { container.scrollLeft = (container.scrollWidth - container.clientWidth) / 2; updateInfo(); };
controls.appendChild(btnCenter);

// 标题
const title = document.createElement('h2');
title.textContent = '水平滚动条测试';
title.style.cssText = 'margin: 20px; color: #333;';
document.body.insertBefore(title, container);

console.log('水平滚动条测试示例已加载');
console.log('容器宽度: 400px, 内容宽度: 800px');
console.log('应该显示水平滚动条');

// 延迟输出实际尺寸
setTimeout(() => {
    console.log('=== 实际布局尺寸 ===');
    const wideRect = wideContent.getBoundingClientRect();
    const containerRect = container.getBoundingClientRect();
    console.log('wideContent rect: ' + wideRect.width + 'x' + wideRect.height);
    console.log('container rect: ' + containerRect.width + 'x' + containerRect.height);
    console.log('wideContent.style.width: ' + wideContent.style.width);
    console.log('wideContent.style.minWidth: ' + wideContent.style.minWidth);
    console.log('container.scrollWidth: ' + container.scrollWidth);
    console.log('container.scrollHeight: ' + container.scrollHeight);
    
    // 测试直接设置宽度
    console.log('=== 测试直接设置宽度 ===');
    const testDiv = document.createElement('div');
    testDiv.style.width = '800px';
    testDiv.style.height = '50px';
    testDiv.style.background = 'red';
    container.appendChild(testDiv);
    
    setTimeout(() => {
        const testRect = testDiv.getBoundingClientRect();
        console.log('testDiv rect (width:800px): ' + testRect.width + 'x' + testRect.height);
        console.log('container.scrollWidth after: ' + container.scrollWidth);
    }, 100);
}, 100);
