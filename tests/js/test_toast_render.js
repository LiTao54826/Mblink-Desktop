/**
 * Toast/Modal/Overlay 渲染可视化测试
 * 
 * 这个测试不会清理元素，用于验证渲染效果
 * 
 * Feature: unified-layer-system
 */

console.log('[TEST_START] Toast/Modal/Overlay Render Test');

// 设置 body 样式
document.body.style.margin = '0';
document.body.style.padding = '20px';
document.body.style.backgroundColor = '#f5f5f5';
document.body.style.fontFamily = 'Arial, sans-serif';

// 创建标题
const title = document.createElement('h1');
title.textContent = 'Z-Index Layer Test';
title.style.marginBottom = '20px';
document.body.appendChild(title);

// 创建普通内容（z-index: auto/0）
const content = document.createElement('div');
content.id = 'base-content';
content.style.position = 'relative';
content.style.width = '400px';
content.style.height = '200px';
content.style.backgroundColor = '#e0e0e0';
content.style.padding = '20px';
content.style.border = '2px solid #999';
content.innerHTML = '<h3>Base Content (z-index: 0)</h3><p>This is the base layer content.</p>';
document.body.appendChild(content);

// 创建 Overlay（z-index: 50）
const overlay = document.createElement('div');
overlay.id = 'overlay';
overlay.style.position = 'fixed';
overlay.style.bottom = '150px';
overlay.style.left = '20px';
overlay.style.width = '180px';
overlay.style.height = '100px';
overlay.style.backgroundColor = 'rgba(0, 0, 255, 0.3)';
overlay.style.zIndex = '50';
overlay.style.padding = '10px';
overlay.style.borderRadius = '8px';
overlay.style.color = 'white';
overlay.innerHTML = '<strong>Overlay</strong><br>z-index: 50';
document.body.appendChild(overlay);

// 创建 Toast（z-index: 100）
const toast = document.createElement('div');
toast.id = 'toast';
toast.style.position = 'fixed';
toast.style.bottom = '20px';
toast.style.right = '20px';
toast.style.width = '200px';
toast.style.height = '60px';
toast.style.backgroundColor = '#333';
toast.style.color = 'white';
toast.style.zIndex = '100';
toast.style.padding = '15px';
toast.style.borderRadius = '8px';
toast.style.boxShadow = '0 4px 12px rgba(0,0,0,0.3)';
toast.innerHTML = '<strong>Toast Notification</strong><br>z-index: 100';
document.body.appendChild(toast);

// 创建半透明遮罩（z-index: 999）
const mask = document.createElement('div');
mask.id = 'modal-mask';
mask.style.position = 'fixed';
mask.style.top = '0';
mask.style.left = '0';
mask.style.width = '100%';
mask.style.height = '100%';
mask.style.backgroundColor = 'rgba(0, 0, 0, 0.5)';
mask.style.zIndex = '999';
document.body.appendChild(mask);

// 创建 Modal（z-index: 1000）
const modal = document.createElement('div');
modal.id = 'modal';
modal.style.position = 'fixed';
modal.style.top = '50%';
modal.style.left = '50%';
modal.style.transform = 'translate(-50%, -50%)';
modal.style.width = '300px';
modal.style.height = '200px';
modal.style.backgroundColor = 'white';
modal.style.zIndex = '1000';
modal.style.padding = '20px';
modal.style.borderRadius = '12px';
modal.style.boxShadow = '0 8px 32px rgba(0,0,0,0.3)';
modal.innerHTML = `
    <h2 style="margin-top:0">Modal Dialog</h2>
    <p>z-index: 1000</p>
    <p>This modal should appear above everything else.</p>
    <button id="close-btn" style="padding:10px 20px; cursor:pointer;">Close</button>
`;
document.body.appendChild(modal);

// 验证元素创建
function logTest(name, passed) {
    if (passed) {
        console.log(`[TEST_PASS] ${name}`);
    } else {
        console.log(`[TEST_FAIL] ${name}`);
    }
}

logTest('Base content created', document.getElementById('base-content') !== null);
logTest('Overlay created', document.getElementById('overlay') !== null);
logTest('Toast created', document.getElementById('toast') !== null);
logTest('Modal mask created', document.getElementById('modal-mask') !== null);
logTest('Modal created', document.getElementById('modal') !== null);

// 验证 z-index 顺序
const baseZ = parseInt(content.style.zIndex) || 0;
const overlayZ = parseInt(overlay.style.zIndex);
const toastZ = parseInt(toast.style.zIndex);
const maskZ = parseInt(mask.style.zIndex);
const modalZ = parseInt(modal.style.zIndex);

logTest('Z-order: base(0) < overlay(50)', baseZ < overlayZ);
logTest('Z-order: overlay(50) < toast(100)', overlayZ < toastZ);
logTest('Z-order: toast(100) < mask(999)', toastZ < maskZ);
logTest('Z-order: mask(999) < modal(1000)', maskZ < modalZ);

console.log('\n[TEST_END] Toast/Modal/Overlay Render Test');
console.log('\nElements are NOT removed - check the UI to verify rendering!');
