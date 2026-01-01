/**
 * Position: Fixed 滚动测试
 * 
 * 验证 position: fixed 元素在页面滚动时保持在视口固定位置
 * 
 * Feature: unified-layer-system
 */

console.log('[TEST_START] Fixed Position Scroll Test');

// 测试辅助函数
function logTest(name, passed) {
    if (passed) {
        console.log(`[TEST_PASS] ${name}`);
    } else {
        console.log(`[TEST_FAIL] ${name}`);
    }
}

// 设置 body 样式，使其可滚动
document.body.style.margin = '0';
document.body.style.padding = '0';
document.body.style.height = '2000px';  // 足够高以产生滚动
document.body.style.overflow = 'auto';

// 创建一个 fixed 元素
const fixedElement = document.createElement('div');
fixedElement.id = 'fixed-element';
fixedElement.style.position = 'fixed';
fixedElement.style.top = '20px';
fixedElement.style.right = '20px';
fixedElement.style.width = '100px';
fixedElement.style.height = '50px';
fixedElement.style.backgroundColor = 'red';
fixedElement.style.zIndex = '1000';
fixedElement.textContent = 'FIXED';
document.body.appendChild(fixedElement);

// 创建一些普通内容
for (let i = 0; i < 20; i++) {
    const p = document.createElement('p');
    p.textContent = `Paragraph ${i + 1} - This is some content to make the page scrollable.`;
    p.style.padding = '20px';
    p.style.margin = '10px';
    p.style.backgroundColor = '#f0f0f0';
    document.body.appendChild(p);
}

// 验证 fixed 元素创建
logTest('Fixed element created', document.getElementById('fixed-element') !== null);
logTest('Fixed element position is fixed', fixedElement.style.position === 'fixed');

// 获取初始位置
const initialRect = fixedElement.getBoundingClientRect();
console.log(`[DEBUG] Initial fixed element position: top=${initialRect.top}, right=${window.innerWidth - initialRect.right}`);

logTest('Initial top position is 20px', Math.abs(initialRect.top - 20) < 5);

// 模拟滚动
console.log('[DEBUG] Scrolling page by 500px...');
window.scrollTo(0, 500);

// 等待一帧后检查位置
setTimeout(() => {
    const afterScrollRect = fixedElement.getBoundingClientRect();
    console.log(`[DEBUG] After scroll fixed element position: top=${afterScrollRect.top}, right=${window.innerWidth - afterScrollRect.right}`);
    
    // Fixed 元素应该保持在视口的相同位置
    // top 应该仍然是 20px（相对于视口）
    const topDiff = Math.abs(afterScrollRect.top - 20);
    logTest('Fixed element stays at top:20px after scroll', topDiff < 5);
    
    // 如果 fixed 元素跟着滚动了，top 会变成 20 - 500 = -480
    if (afterScrollRect.top < 0) {
        console.log('[ERROR] Fixed element scrolled with document! Expected top=20, got top=' + afterScrollRect.top);
    }
    
    console.log('[TEST_END]');
}, 100);
