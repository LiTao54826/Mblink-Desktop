/**
 * @file test_modal_incremental.js
 * @brief Modal 增量布局测试
 * 
 * 测试 position: fixed/absolute 元素的增量布局优化
 * 验证 Modal 打开/关闭不触发全量重建
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
        console.log(`  Value was null or undefined`);
    }
    return passed;
}

// 创建基础页面结构
function setupPage() {
    document.body.innerHTML = `
        <div id="app" style="width: 100%; height: 100%;">
            <div id="header" style="height: 60px; background: #333; color: white;">
                Header
            </div>
            <div id="content" style="padding: 20px;">
                <p id="text1">This is some content that should not be affected by modal.</p>
                <p id="text2">Another paragraph of content.</p>
                <button id="openModal">Open Modal</button>
            </div>
            <div id="footer" style="height: 40px; background: #666; color: white;">
                Footer
            </div>
        </div>
    `;
}

// 创建 Modal 元素
function createModal() {
    const modal = document.createElement('div');
    modal.id = 'modal';
    modal.style.position = 'fixed';
    modal.style.top = '50%';
    modal.style.left = '50%';
    modal.style.width = '300px';
    modal.style.height = '200px';
    modal.style.backgroundColor = 'white';
    modal.style.border = '1px solid #ccc';
    modal.style.zIndex = '1000';
    
    modal.innerHTML = `
        <div style="padding: 20px;">
            <h2>Modal Title</h2>
            <p>Modal content here</p>
            <button id="closeModal">Close</button>
        </div>
    `;
    
    return modal;
}

// 测试 1: Modal 添加不影响其他元素布局
function testModalAddDoesNotAffectOthers() {
    setupPage();
    
    // 获取其他元素的初始布局
    const header = document.getElementById('header');
    const content = document.getElementById('content');
    const text1 = document.getElementById('text1');
    
    const headerRect1 = header.getBoundingClientRect();
    const contentRect1 = content.getBoundingClientRect();
    const text1Rect1 = text1.getBoundingClientRect();
    
    // 添加 Modal
    const modal = createModal();
    document.body.appendChild(modal);
    
    // 获取添加 Modal 后其他元素的布局
    const headerRect2 = header.getBoundingClientRect();
    const contentRect2 = content.getBoundingClientRect();
    const text1Rect2 = text1.getBoundingClientRect();
    
    // 验证其他元素布局未变化
    let passed = true;
    
    if (headerRect1.top !== headerRect2.top || headerRect1.left !== headerRect2.left) {
        console.log('  Header position changed after modal add');
        passed = false;
    }
    
    if (contentRect1.top !== contentRect2.top || contentRect1.left !== contentRect2.left) {
        console.log('  Content position changed after modal add');
        passed = false;
    }
    
    if (text1Rect1.top !== text1Rect2.top || text1Rect1.left !== text1Rect2.left) {
        console.log('  Text1 position changed after modal add');
        passed = false;
    }
    
    logTest('Modal add does not affect other elements layout', passed);
    
    // 清理
    modal.remove();
    
    return passed;
}

// 测试 2: Modal 移除不影响其他元素布局
function testModalRemoveDoesNotAffectOthers() {
    setupPage();
    
    // 先添加 Modal
    const modal = createModal();
    document.body.appendChild(modal);
    
    // 获取其他元素的布局（Modal 存在时）
    const header = document.getElementById('header');
    const content = document.getElementById('content');
    
    const headerRect1 = header.getBoundingClientRect();
    const contentRect1 = content.getBoundingClientRect();
    
    // 移除 Modal
    modal.remove();
    
    // 获取移除 Modal 后其他元素的布局
    const headerRect2 = header.getBoundingClientRect();
    const contentRect2 = content.getBoundingClientRect();
    
    // 验证其他元素布局未变化
    let passed = true;
    
    if (headerRect1.top !== headerRect2.top || headerRect1.left !== headerRect2.left) {
        console.log('  Header position changed after modal remove');
        passed = false;
    }
    
    if (contentRect1.top !== contentRect2.top || contentRect1.left !== contentRect2.left) {
        console.log('  Content position changed after modal remove');
        passed = false;
    }
    
    logTest('Modal remove does not affect other elements layout', passed);
    
    return passed;
}

// 测试 3: position: absolute 元素同样不影响其他元素
function testAbsolutePositionElement() {
    setupPage();
    
    // 获取初始布局
    const content = document.getElementById('content');
    const contentRect1 = content.getBoundingClientRect();
    
    // 创建 absolute 定位元素
    const dropdown = document.createElement('div');
    dropdown.id = 'dropdown';
    dropdown.style.position = 'absolute';
    dropdown.style.top = '100px';
    dropdown.style.left = '100px';
    dropdown.style.width = '150px';
    dropdown.style.height = '100px';
    dropdown.style.backgroundColor = '#f0f0f0';
    dropdown.textContent = 'Dropdown Menu';
    
    document.body.appendChild(dropdown);
    
    // 获取添加后的布局
    const contentRect2 = content.getBoundingClientRect();
    
    // 验证布局未变化
    const passed = contentRect1.top === contentRect2.top && 
                   contentRect1.left === contentRect2.left &&
                   contentRect1.width === contentRect2.width;
    
    logTest('Absolute position element does not affect other elements', passed);
    
    // 清理
    dropdown.remove();
    
    return passed;
}

// 测试 4: Modal 内容变化不影响外部元素
function testModalContentChangeDoesNotAffectOthers() {
    setupPage();
    
    // 添加 Modal
    const modal = createModal();
    document.body.appendChild(modal);
    
    // 获取外部元素布局
    const content = document.getElementById('content');
    const contentRect1 = content.getBoundingClientRect();
    
    // 修改 Modal 内容
    const modalContent = modal.querySelector('p');
    modalContent.textContent = 'This is a much longer content that might cause layout changes inside the modal but should not affect outside elements at all.';
    
    // 获取修改后的外部元素布局
    const contentRect2 = content.getBoundingClientRect();
    
    // 验证外部布局未变化
    const passed = contentRect1.top === contentRect2.top && 
                   contentRect1.left === contentRect2.left;
    
    logTest('Modal content change does not affect outside elements', passed);
    
    // 清理
    modal.remove();
    
    return passed;
}

// 测试 5: 多个 Modal 同时存在
function testMultipleModals() {
    setupPage();
    
    // 获取初始布局
    const content = document.getElementById('content');
    const contentRect1 = content.getBoundingClientRect();
    
    // 添加多个 Modal
    const modal1 = createModal();
    modal1.id = 'modal1';
    modal1.style.top = '30%';
    
    const modal2 = createModal();
    modal2.id = 'modal2';
    modal2.style.top = '60%';
    
    document.body.appendChild(modal1);
    document.body.appendChild(modal2);
    
    // 获取添加后的布局
    const contentRect2 = content.getBoundingClientRect();
    
    // 验证布局未变化
    const passed = contentRect1.top === contentRect2.top && 
                   contentRect1.left === contentRect2.left;
    
    logTest('Multiple modals do not affect other elements', passed);
    
    // 清理
    modal1.remove();
    modal2.remove();
    
    return passed;
}

// 测试 6: Modal 存在时检查元素是否正确渲染
function testModalRendersCorrectly() {
    setupPage();
    
    // 添加 Modal
    const modal = createModal();
    document.body.appendChild(modal);
    
    // 验证 Modal 存在且有正确的样式
    const modalElement = document.getElementById('modal');
    const passed = modalElement !== null && 
                   modalElement.style.position === 'fixed';
    
    logTest('Modal renders correctly with fixed position', passed);
    
    // 清理
    modal.remove();
    
    return passed;
}

// 运行所有测试
function runAllTests() {
    console.log('[TEST_START] Modal Incremental Layout Tests');
    
    let allPassed = true;
    
    allPassed = testModalAddDoesNotAffectOthers() && allPassed;
    allPassed = testModalRemoveDoesNotAffectOthers() && allPassed;
    allPassed = testAbsolutePositionElement() && allPassed;
    allPassed = testModalContentChangeDoesNotAffectOthers() && allPassed;
    allPassed = testMultipleModals() && allPassed;
    allPassed = testModalRendersCorrectly() && allPassed;
    
    console.log('[TEST_END]');
    
    if (allPassed) {
        console.log('All modal incremental layout tests passed!');
    } else {
        console.log('Some tests failed!');
    }
}

// 执行测试
runAllTests();
