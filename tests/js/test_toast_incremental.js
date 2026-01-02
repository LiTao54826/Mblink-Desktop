/**
 * @file test_toast_incremental.js
 * @brief Toast 增量布局测试
 * 
 * 测试 Toast 显示/隐藏不触发全量重建
 * 
 * **Feature: incremental-layout-boundary**
 * **Property 1: Out-of-Flow Element Isolation**
 * **Validates: Requirements 1.1, 1.3**
 */

// 测试辅助函数
function logTest(name, passed) {
    if (passed) {
        console.log(`[TEST_PASS] ${name}`);
    } else {
        console.log(`[TEST_FAIL] ${name}`);
    }
}

// 创建基础页面结构
function setupPage() {
    document.body.innerHTML = `
        <div id="app" style="width: 100%; height: 100%;">
            <div id="header" style="height: 60px; background: #333; color: white;">
                Header
            </div>
            <div id="content" style="padding: 20px;">
                <p id="text1">Main content area</p>
                <button id="showToast">Show Toast</button>
            </div>
        </div>
    `;
}

// 创建 Toast 元素
function createToast(message) {
    const toast = document.createElement('div');
    toast.className = 'toast';
    toast.style.position = 'fixed';
    toast.style.bottom = '20px';
    toast.style.right = '20px';
    toast.style.padding = '12px 24px';
    toast.style.backgroundColor = '#333';
    toast.style.color = 'white';
    toast.style.borderRadius = '4px';
    toast.style.zIndex = '9999';
    toast.textContent = message;
    return toast;
}

// 测试 1: Toast 显示不影响其他元素
function testToastShowDoesNotAffectOthers() {
    setupPage();
    
    // 获取其他元素的初始布局
    const header = document.getElementById('header');
    const content = document.getElementById('content');
    
    const headerRect1 = header.getBoundingClientRect();
    const contentRect1 = content.getBoundingClientRect();
    
    // 显示 Toast
    const toast = createToast('This is a toast message');
    document.body.appendChild(toast);
    
    // 获取显示 Toast 后其他元素的布局
    const headerRect2 = header.getBoundingClientRect();
    const contentRect2 = content.getBoundingClientRect();
    
    // 验证其他元素布局未变化
    let passed = true;
    
    if (headerRect1.top !== headerRect2.top || headerRect1.left !== headerRect2.left) {
        console.log('  Header position changed after toast show');
        passed = false;
    }
    
    if (contentRect1.top !== contentRect2.top || contentRect1.left !== contentRect2.left) {
        console.log('  Content position changed after toast show');
        passed = false;
    }
    
    logTest('Toast show does not affect other elements layout', passed);
    
    // 清理
    toast.remove();
    
    return passed;
}

// 测试 2: Toast 隐藏不影响其他元素
function testToastHideDoesNotAffectOthers() {
    setupPage();
    
    // 先显示 Toast
    const toast = createToast('Toast message');
    document.body.appendChild(toast);
    
    // 获取其他元素的布局
    const header = document.getElementById('header');
    const headerRect1 = header.getBoundingClientRect();
    
    // 隐藏 Toast
    toast.remove();
    
    // 获取隐藏后的布局
    const headerRect2 = header.getBoundingClientRect();
    
    // 验证布局未变化
    const passed = headerRect1.top === headerRect2.top && 
                   headerRect1.left === headerRect2.left;
    
    logTest('Toast hide does not affect other elements layout', passed);
    
    return passed;
}

// 测试 3: 多个 Toast 同时显示
function testMultipleToasts() {
    setupPage();
    
    // 获取初始布局
    const content = document.getElementById('content');
    const contentRect1 = content.getBoundingClientRect();
    
    // 显示多个 Toast
    const toasts = [];
    for (let i = 0; i < 3; i++) {
        const toast = createToast(`Toast ${i + 1}`);
        toast.style.bottom = `${20 + i * 50}px`;
        document.body.appendChild(toast);
        toasts.push(toast);
    }
    
    // 获取显示后的布局
    const contentRect2 = content.getBoundingClientRect();
    
    // 验证布局未变化
    const passed = contentRect1.top === contentRect2.top && 
                   contentRect1.left === contentRect2.left;
    
    logTest('Multiple toasts do not affect other elements', passed);
    
    // 清理
    toasts.forEach(t => t.remove());
    
    return passed;
}

// 测试 4: Toast 内容更新不影响外部
function testToastContentUpdate() {
    setupPage();
    
    // 显示 Toast
    const toast = createToast('Initial message');
    document.body.appendChild(toast);
    
    // 获取外部元素布局
    const content = document.getElementById('content');
    const contentRect1 = content.getBoundingClientRect();
    
    // 更新 Toast 内容
    toast.textContent = 'This is a much longer toast message that might cause the toast to resize';
    
    // 获取更新后的布局
    const contentRect2 = content.getBoundingClientRect();
    
    // 验证外部布局未变化
    const passed = contentRect1.top === contentRect2.top && 
                   contentRect1.left === contentRect2.left;
    
    logTest('Toast content update does not affect outside elements', passed);
    
    // 清理
    toast.remove();
    
    return passed;
}

// 测试 5: Toast 位置在不同角落
function testToastDifferentPositions() {
    setupPage();
    
    // 获取初始布局
    const content = document.getElementById('content');
    const contentRect1 = content.getBoundingClientRect();
    
    // 在不同位置显示 Toast
    const positions = [
        { top: '20px', left: '20px' },
        { top: '20px', right: '20px' },
        { bottom: '20px', left: '20px' },
        { bottom: '20px', right: '20px' }
    ];
    
    const toasts = [];
    positions.forEach((pos, i) => {
        const toast = createToast(`Toast ${i + 1}`);
        Object.assign(toast.style, pos);
        document.body.appendChild(toast);
        toasts.push(toast);
    });
    
    // 获取显示后的布局
    const contentRect2 = content.getBoundingClientRect();
    
    // 验证布局未变化
    const passed = contentRect1.top === contentRect2.top && 
                   contentRect1.left === contentRect2.left;
    
    logTest('Toasts at different positions do not affect layout', passed);
    
    // 清理
    toasts.forEach(t => t.remove());
    
    return passed;
}

// 运行所有测试
function runAllTests() {
    console.log('[TEST_START] Toast Incremental Layout Tests');
    
    let allPassed = true;
    
    allPassed = testToastShowDoesNotAffectOthers() && allPassed;
    allPassed = testToastHideDoesNotAffectOthers() && allPassed;
    allPassed = testMultipleToasts() && allPassed;
    allPassed = testToastContentUpdate() && allPassed;
    allPassed = testToastDifferentPositions() && allPassed;
    
    console.log('[TEST_END]');
    
    if (allPassed) {
        console.log('All toast incremental layout tests passed!');
    } else {
        console.log('Some tests failed!');
    }
}

// 执行测试
runAllTests();
