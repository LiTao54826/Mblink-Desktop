// 测试 display: contents 在 flex 容器中的行为
// 不使用 Preact，直接测试 DOM

document.body.style.margin = '0';
document.body.style.padding = '20px';
document.body.style.backgroundColor = '#f0f0f0';

// 测试辅助函数
function logTest(name, passed) {
    if (passed) {
        console.log(`[TEST_PASS] ${name}`);
    } else {
        console.log(`[TEST_FAIL] ${name}`);
    }
}

console.log("[TEST_START] Display Contents in Flex Container");

// 创建 flex 容器
const container = document.createElement('div');
container.id = 'flex-container';
Object.assign(container.style, {
    position: 'fixed',
    top: '20px',
    left: '50%',
    transform: 'translateX(-50%)',
    display: 'flex',
    flexDirection: 'column',
    alignItems: 'center',
    gap: '8px',
    border: '2px dashed red',
    padding: '10px',
});
document.body.appendChild(container);

// 创建 display: contents 包装器
const contentsWrapper = document.createElement('div');
contentsWrapper.id = 'contents-wrapper';
contentsWrapper.style.display = 'contents';
container.appendChild(contentsWrapper);

// 创建两个不同宽度的子元素
const longItem = document.createElement('div');
longItem.id = 'long-item';
longItem.textContent = '这是一个很长很长很长很长很长的内容';
Object.assign(longItem.style, {
    padding: '10px 16px',
    backgroundColor: '#ffffff',
    borderRadius: '6px',
    boxShadow: '0 4px 12px rgba(0,0,0,0.15)',
});
contentsWrapper.appendChild(longItem);

const shortItem = document.createElement('div');
shortItem.id = 'short-item';
shortItem.textContent = '短内容';
Object.assign(shortItem.style, {
    padding: '10px 16px',
    backgroundColor: '#ffffff',
    borderRadius: '6px',
    boxShadow: '0 4px 12px rgba(0,0,0,0.15)',
});
contentsWrapper.appendChild(shortItem);

// 等待布局完成后检查宽度
setTimeout(() => {
    const longRect = longItem.getBoundingClientRect();
    const shortRect = shortItem.getBoundingClientRect();
    
    console.log(`Long item width: ${longRect.width}`);
    console.log(`Short item width: ${shortRect.width}`);
    
    // 短元素的宽度应该小于长元素
    const widthsDifferent = shortRect.width < longRect.width;
    logTest('Short item should be narrower than long item', widthsDifferent);
    
    // 检查是否居中（两个元素的中心点应该相同）
    const longCenter = longRect.left + longRect.width / 2;
    const shortCenter = shortRect.left + shortRect.width / 2;
    const centered = Math.abs(longCenter - shortCenter) < 2; // 允许 2px 误差
    logTest('Both items should be centered', centered);
    
    console.log(`Long item center: ${longCenter}`);
    console.log(`Short item center: ${shortCenter}`);
    
    console.log("[TEST_END]");
}, 100);
