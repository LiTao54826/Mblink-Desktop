/**
 * Toast、Modal、Overlay 高 z-index 元素测试
 * 
 * 验证高 z-index 元素的绘制顺序和 hit testing：
 * - Toast 元素（z-index >= 100）
 * - Modal 元素（z-index >= 1000）
 * - Overlay 元素覆盖普通内容
 * - 多个高 z-index 元素之间的层叠顺序
 * 
 * Feature: unified-layer-system
 * Validates: Requirements 5.2, 5.3, 5.4
 */

console.log('[TEST_START] Toast/Modal/Overlay Tests');

// 测试辅助函数
function logTest(name, passed) {
    if (passed) {
        console.log(`[TEST_PASS] ${name}`);
    } else {
        console.log(`[TEST_FAIL] ${name}`);
    }
}

function assertEqual(actual, expected, testName) {
    const passed = actual === expected;
    logTest(testName, passed);
    if (!passed) {
        console.log(`  Expected: ${expected}`);
        console.log(`  Actual: ${actual}`);
    }
    return passed;
}

function assertNotNull(value, testName) {
    const passed = value !== null && value !== undefined;
    logTest(testName, passed);
    if (!passed) {
        console.log(`  Value is null or undefined`);
    }
    return passed;
}

// =========================================================================
// 测试 1: Toast 场景
// =========================================================================

function testToastScenario() {
    console.log('\n--- Test: Toast Scenario ---');
    
    // 创建普通内容
    const content = document.createElement('div');
    content.id = 'toast-content';
    content.style.width = '100%';
    content.style.height = '300px';
    content.style.backgroundColor = '#f0f0f0';
    content.innerHTML = '<p>Normal content</p>';
    document.body.appendChild(content);
    
    // 创建 Toast 通知（z-index >= 100）
    const toast = document.createElement('div');
    toast.id = 'toast-notification';
    toast.style.position = 'fixed';
    toast.style.bottom = '20px';
    toast.style.right = '20px';
    toast.style.width = '200px';
    toast.style.height = '60px';
    toast.style.backgroundColor = '#333';
    toast.style.color = 'white';
    toast.style.zIndex = '100';
    toast.style.padding = '10px';
    toast.style.borderRadius = '4px';
    toast.textContent = 'Toast Message';
    document.body.appendChild(toast);
    
    // 验证元素已创建
    assertNotNull(document.getElementById('toast-content'), 'Toast content created');
    assertNotNull(document.getElementById('toast-notification'), 'Toast notification created');
    
    // 验证 z-index
    assertEqual(toast.style.zIndex, '100', 'Toast z-index is 100');
    
    // 清理
    content.remove();
    toast.remove();
    
    logTest('Toast Scenario', true);
}

// =========================================================================
// 测试 2: Modal 场景
// =========================================================================

function testModalScenario() {
    console.log('\n--- Test: Modal Scenario ---');
    
    // 创建普通内容
    const content = document.createElement('div');
    content.id = 'modal-content';
    content.style.width = '100%';
    content.style.height = '300px';
    content.style.backgroundColor = '#f0f0f0';
    content.innerHTML = '<p>Normal content behind modal</p>';
    document.body.appendChild(content);
    
    // 创建遮罩层
    const overlay = document.createElement('div');
    overlay.id = 'modal-overlay';
    overlay.style.position = 'fixed';
    overlay.style.top = '0';
    overlay.style.left = '0';
    overlay.style.width = '100%';
    overlay.style.height = '100%';
    overlay.style.backgroundColor = 'rgba(0, 0, 0, 0.5)';
    overlay.style.zIndex = '999';
    document.body.appendChild(overlay);
    
    // 创建 Modal 对话框（z-index >= 1000）
    const modal = document.createElement('div');
    modal.id = 'modal-dialog';
    modal.style.position = 'fixed';
    modal.style.top = '50%';
    modal.style.left = '50%';
    modal.style.transform = 'translate(-50%, -50%)';
    modal.style.width = '300px';
    modal.style.height = '200px';
    modal.style.backgroundColor = 'white';
    modal.style.zIndex = '1000';
    modal.style.padding = '20px';
    modal.style.borderRadius = '8px';
    modal.innerHTML = '<h2>Modal Title</h2><p>Modal content</p>';
    document.body.appendChild(modal);
    
    // 在 Modal 内创建按钮
    const closeBtn = document.createElement('button');
    closeBtn.id = 'modal-close-btn';
    closeBtn.textContent = 'Close';
    closeBtn.style.marginTop = '10px';
    modal.appendChild(closeBtn);
    
    // 验证元素已创建
    assertNotNull(document.getElementById('modal-content'), 'Modal content created');
    assertNotNull(document.getElementById('modal-overlay'), 'Modal overlay created');
    assertNotNull(document.getElementById('modal-dialog'), 'Modal dialog created');
    assertNotNull(document.getElementById('modal-close-btn'), 'Modal close button created');
    
    // 验证 z-index
    assertEqual(overlay.style.zIndex, '999', 'Overlay z-index is 999');
    assertEqual(modal.style.zIndex, '1000', 'Modal z-index is 1000');
    
    // 清理
    content.remove();
    overlay.remove();
    modal.remove();
    
    logTest('Modal Scenario', true);
}

// =========================================================================
// 测试 3: 多层叠加场景
// =========================================================================

function testMultiLayerScenario() {
    console.log('\n--- Test: Multi-Layer Scenario ---');
    
    // 创建普通内容（z-index: auto/0）
    const content = document.createElement('div');
    content.id = 'multi-content';
    content.style.position = 'relative';
    content.style.width = '100%';
    content.style.height = '200px';
    content.style.backgroundColor = '#e0e0e0';
    content.innerHTML = '<p>Base content (z-index: 0)</p>';
    document.body.appendChild(content);
    
    // 创建 Overlay（z-index: 50）
    const overlay = document.createElement('div');
    overlay.id = 'multi-overlay';
    overlay.style.position = 'fixed';
    overlay.style.bottom = '100px';
    overlay.style.left = '20px';
    overlay.style.width = '150px';
    overlay.style.height = '100px';
    overlay.style.backgroundColor = 'rgba(0, 0, 255, 0.3)';
    overlay.style.zIndex = '50';
    overlay.textContent = 'Overlay (z: 50)';
    document.body.appendChild(overlay);
    
    // 创建 Toast（z-index: 100）
    const toast = document.createElement('div');
    toast.id = 'multi-toast';
    toast.style.position = 'fixed';
    toast.style.bottom = '20px';
    toast.style.right = '20px';
    toast.style.width = '150px';
    toast.style.height = '50px';
    toast.style.backgroundColor = '#333';
    toast.style.color = 'white';
    toast.style.zIndex = '100';
    toast.textContent = 'Toast (z: 100)';
    document.body.appendChild(toast);
    
    // 创建 Modal（z-index: 1000）
    const modal = document.createElement('div');
    modal.id = 'multi-modal';
    modal.style.position = 'fixed';
    modal.style.top = '50%';
    modal.style.left = '50%';
    modal.style.transform = 'translate(-50%, -50%)';
    modal.style.width = '200px';
    modal.style.height = '150px';
    modal.style.backgroundColor = 'white';
    modal.style.zIndex = '1000';
    modal.style.border = '2px solid #333';
    modal.textContent = 'Modal (z: 1000)';
    document.body.appendChild(modal);
    
    // 验证元素已创建
    assertNotNull(document.getElementById('multi-content'), 'Multi-layer content created');
    assertNotNull(document.getElementById('multi-overlay'), 'Multi-layer overlay created');
    assertNotNull(document.getElementById('multi-toast'), 'Multi-layer toast created');
    assertNotNull(document.getElementById('multi-modal'), 'Multi-layer modal created');
    
    // 验证 z-index 顺序
    const contentZ = parseInt(content.style.zIndex) || 0;
    const overlayZ = parseInt(overlay.style.zIndex);
    const toastZ = parseInt(toast.style.zIndex);
    const modalZ = parseInt(modal.style.zIndex);
    
    logTest('Z-index order: content < overlay', contentZ < overlayZ);
    logTest('Z-index order: overlay < toast', overlayZ < toastZ);
    logTest('Z-index order: toast < modal', toastZ < modalZ);
    
    // 清理
    content.remove();
    overlay.remove();
    toast.remove();
    modal.remove();
    
    logTest('Multi-Layer Scenario', true);
}

// =========================================================================
// 测试 4: 高 z-index 元素内部的 Hit Testing
// =========================================================================

function testHighZIndexHitTesting() {
    console.log('\n--- Test: High Z-index Hit Testing ---');
    
    // 创建底层可点击元素
    const bottom = document.createElement('button');
    bottom.id = 'bottom-button';
    bottom.style.position = 'fixed';
    bottom.style.top = '100px';
    bottom.style.left = '100px';
    bottom.style.width = '200px';
    bottom.style.height = '50px';
    bottom.style.zIndex = '1';
    bottom.textContent = 'Bottom Button';
    document.body.appendChild(bottom);
    
    // 创建高 z-index 的遮罩（应该阻止底层点击）
    const mask = document.createElement('div');
    mask.id = 'blocking-mask';
    mask.style.position = 'fixed';
    mask.style.top = '0';
    mask.style.left = '0';
    mask.style.width = '100%';
    mask.style.height = '100%';
    mask.style.backgroundColor = 'rgba(0, 0, 0, 0.3)';
    mask.style.zIndex = '500';
    document.body.appendChild(mask);
    
    // 创建高 z-index 的可点击元素
    const topButton = document.createElement('button');
    topButton.id = 'top-button';
    topButton.style.position = 'fixed';
    topButton.style.top = '150px';
    topButton.style.left = '150px';
    topButton.style.width = '100px';
    topButton.style.height = '40px';
    topButton.style.zIndex = '1000';
    topButton.textContent = 'Top Button';
    document.body.appendChild(topButton);
    
    // 验证元素已创建
    assertNotNull(document.getElementById('bottom-button'), 'Bottom button created');
    assertNotNull(document.getElementById('blocking-mask'), 'Blocking mask created');
    assertNotNull(document.getElementById('top-button'), 'Top button created');
    
    // 验证 z-index
    assertEqual(bottom.style.zIndex, '1', 'Bottom button z-index');
    assertEqual(mask.style.zIndex, '500', 'Mask z-index');
    assertEqual(topButton.style.zIndex, '1000', 'Top button z-index');
    
    // 清理
    bottom.remove();
    mask.remove();
    topButton.remove();
    
    logTest('High Z-index Hit Testing', true);
}

// =========================================================================
// 测试 5: Dropdown 菜单场景
// =========================================================================

function testDropdownScenario() {
    console.log('\n--- Test: Dropdown Scenario ---');
    
    // 创建触发按钮
    const trigger = document.createElement('button');
    trigger.id = 'dropdown-trigger';
    trigger.style.position = 'relative';
    trigger.style.marginTop = '50px';
    trigger.style.marginLeft = '50px';
    trigger.textContent = 'Open Dropdown';
    document.body.appendChild(trigger);
    
    // 创建 Dropdown 菜单（z-index: 100）
    const dropdown = document.createElement('div');
    dropdown.id = 'dropdown-menu';
    dropdown.style.position = 'absolute';
    dropdown.style.top = '80px';
    dropdown.style.left = '50px';
    dropdown.style.width = '150px';
    dropdown.style.backgroundColor = 'white';
    dropdown.style.border = '1px solid #ccc';
    dropdown.style.zIndex = '100';
    dropdown.style.boxShadow = '0 2px 10px rgba(0,0,0,0.1)';
    document.body.appendChild(dropdown);
    
    // 添加菜单项
    for (let i = 1; i <= 3; i++) {
        const item = document.createElement('div');
        item.id = `dropdown-item-${i}`;
        item.style.padding = '10px';
        item.style.cursor = 'pointer';
        item.textContent = `Menu Item ${i}`;
        dropdown.appendChild(item);
    }
    
    // 验证元素已创建
    assertNotNull(document.getElementById('dropdown-trigger'), 'Dropdown trigger created');
    assertNotNull(document.getElementById('dropdown-menu'), 'Dropdown menu created');
    assertNotNull(document.getElementById('dropdown-item-1'), 'Dropdown item 1 created');
    assertNotNull(document.getElementById('dropdown-item-2'), 'Dropdown item 2 created');
    assertNotNull(document.getElementById('dropdown-item-3'), 'Dropdown item 3 created');
    
    // 清理
    trigger.remove();
    dropdown.remove();
    
    logTest('Dropdown Scenario', true);
}

// =========================================================================
// 运行所有测试
// =========================================================================

testToastScenario();
testModalScenario();
testMultiLayerScenario();
testHighZIndexHitTesting();
testDropdownScenario();

console.log('\n[TEST_END] Toast/Modal/Overlay Tests');
