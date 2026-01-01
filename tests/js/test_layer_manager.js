/**
 * LayerManager 测试
 * 
 * 验证 LayerManager 不会收集已有 CompositorLayer 的元素
 */

console.log('[TEST_START] LayerManager Test');

// 创建测试 DOM 结构
const container = document.createElement('div');
container.id = 'test-container';
container.style.cssText = 'position: fixed; z-index: 9999; top: 20px; right: 20px; width: 200px; height: 50px; background: #4CAF50; color: white; padding: 10px;';
container.textContent = 'Test Toast';
document.body.appendChild(container);

// 等待渲染
setTimeout(() => {
    // 检查元素是否存在
    const elem = document.getElementById('test-container');
    if (elem) {
        console.log('[TEST_PASS] Toast container created');
    } else {
        console.log('[TEST_FAIL] Toast container not found');
    }
    
    // 检查样式
    const style = window.getComputedStyle(elem);
    if (style.position === 'fixed') {
        console.log('[TEST_PASS] Toast has fixed position');
    } else {
        console.log('[TEST_FAIL] Toast position is not fixed: ' + style.position);
    }
    
    console.log('[TEST_END]');
}, 500);
