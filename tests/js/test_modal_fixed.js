/**
 * Modal Fixed Position 测试
 * 
 * 验证 Modal 组件使用 position: fixed 时能正确显示在视口中心
 * 不受页面滚动影响
 * 
 * Feature: unified-layer-system
 */

console.log('[TEST_START] Modal Fixed Position Test');

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
document.body.style.height = '3000px';  // 足够高以产生滚动
document.body.style.overflow = 'auto';
document.body.style.backgroundColor = '#f0f0f0';

// 创建一些普通内容
for (let i = 0; i < 30; i++) {
    const p = document.createElement('p');
    p.textContent = `Content line ${i + 1} - This is some content to make the page scrollable.`;
    p.style.padding = '15px';
    p.style.margin = '10px';
    p.style.backgroundColor = '#fff';
    document.body.appendChild(p);
}

// 创建 Modal 遮罩层 (position: fixed)
const mask = document.createElement('div');
mask.id = 'modal-mask';
mask.style.position = 'fixed';
mask.style.top = '0';
mask.style.left = '0';
mask.style.right = '0';
mask.style.bottom = '0';
mask.style.width = '100%';
mask.style.height = '100%';
mask.style.backgroundColor = 'rgba(0, 0, 0, 0.5)';
mask.style.display = 'flex';
mask.style.alignItems = 'center';
mask.style.justifyContent = 'center';
mask.style.zIndex = '1000';
document.body.appendChild(mask);

// 创建 Modal 对话框
const modal = document.createElement('div');
modal.id = 'modal-dialog';
modal.style.width = '400px';
modal.style.height = '200px';
modal.style.backgroundColor = 'white';
modal.style.borderRadius = '8px';
modal.style.padding = '20px';
modal.style.boxShadow = '0 4px 20px rgba(0,0,0,0.3)';
modal.innerHTML = `
    <h2 style="margin-top:0">Modal Dialog</h2>
    <p>This modal should be centered in the viewport.</p>
    <p>It should NOT move when the page scrolls.</p>
`;
mask.appendChild(modal);

// 验证元素创建
logTest('Modal mask created', document.getElementById('modal-mask') !== null);
logTest('Modal dialog created', document.getElementById('modal-dialog') !== null);
logTest('Mask position is fixed', mask.style.position === 'fixed');

// 获取初始位置
const maskRect = mask.getBoundingClientRect();
const modalRect = modal.getBoundingClientRect();

console.log(`[DEBUG] Viewport size: ${window.innerWidth} x ${window.innerHeight}`);
console.log(`[DEBUG] Mask rect: top=${maskRect.top}, left=${maskRect.left}, width=${maskRect.width}, height=${maskRect.height}`);
console.log(`[DEBUG] Modal rect: top=${modalRect.top}, left=${modalRect.left}, width=${modalRect.width}, height=${modalRect.height}`);

// 验证遮罩层覆盖整个视口
logTest('Mask covers viewport (top=0)', Math.abs(maskRect.top) < 5);
logTest('Mask covers viewport (left=0)', Math.abs(maskRect.left) < 5);

// 验证 Modal 大致居中
const expectedModalTop = (window.innerHeight - modalRect.height) / 2;
const expectedModalLeft = (window.innerWidth - modalRect.width) / 2;
console.log(`[DEBUG] Expected modal position: top=${expectedModalTop}, left=${expectedModalLeft}`);

const topDiff = Math.abs(modalRect.top - expectedModalTop);
const leftDiff = Math.abs(modalRect.left - expectedModalLeft);
logTest('Modal roughly centered vertically', topDiff < 50);
logTest('Modal roughly centered horizontally', leftDiff < 50);

// 滚动页面
console.log('[DEBUG] Scrolling page by 500px...');
window.scrollTo(0, 500);

// 等待一帧后检查位置
setTimeout(() => {
    const maskRectAfter = mask.getBoundingClientRect();
    const modalRectAfter = modal.getBoundingClientRect();
    
    console.log(`[DEBUG] After scroll - Mask rect: top=${maskRectAfter.top}, left=${maskRectAfter.left}`);
    console.log(`[DEBUG] After scroll - Modal rect: top=${modalRectAfter.top}, left=${modalRectAfter.left}`);
    
    // Fixed 元素应该保持在视口的相同位置
    logTest('Mask stays at top:0 after scroll', Math.abs(maskRectAfter.top) < 5);
    logTest('Mask stays at left:0 after scroll', Math.abs(maskRectAfter.left) < 5);
    
    // Modal 应该仍然居中
    const topDiffAfter = Math.abs(modalRectAfter.top - expectedModalTop);
    const leftDiffAfter = Math.abs(modalRectAfter.left - expectedModalLeft);
    logTest('Modal stays centered after scroll (vertical)', topDiffAfter < 50);
    logTest('Modal stays centered after scroll (horizontal)', leftDiffAfter < 50);
    
    // 检查是否跟着滚动了
    if (maskRectAfter.top < -100) {
        console.log('[ERROR] Mask scrolled with document! Expected top=0, got top=' + maskRectAfter.top);
    }
    if (modalRectAfter.top < -100) {
        console.log('[ERROR] Modal scrolled with document! Expected centered, got top=' + modalRectAfter.top);
    }
    
    console.log('[TEST_END]');
}, 200);
