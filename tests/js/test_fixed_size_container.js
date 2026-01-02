/**
 * @file test_fixed_size_container.js
 * @brief 固定尺寸容器增量布局测试
 * 
 * 测试固定宽高容器内元素增删不影响祖先布局
 * 
 * **Feature: incremental-layout-boundary**
 * **Property 4: Fixed-Size Container Boundary**
 * **Validates: Requirements 3.1, 3.2, 3.3**
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
        <div id="app" style="width: 100%;">
            <div id="header" style="height: 60px; background: #333; color: white;">
                Header
            </div>
            <div id="fixedContainer" style="width: 300px; height: 200px; border: 2px solid #666; overflow: hidden;">
                <div id="innerContent">
                    <p>Initial content</p>
                </div>
            </div>
            <div id="sibling" style="height: 50px; background: #eee;">
                Sibling element
            </div>
            <div id="footer" style="height: 40px; background: #666; color: white;">
                Footer
            </div>
        </div>
    `;
}

// 测试 1: 添加内容到固定尺寸容器不影响兄弟元素
function testAddContentDoesNotAffectSibling() {
    setupPage();
    
    // 获取兄弟元素的初始布局
    const sibling = document.getElementById('sibling');
    const footer = document.getElementById('footer');
    
    const siblingRect1 = sibling.getBoundingClientRect();
    const footerRect1 = footer.getBoundingClientRect();
    
    // 在固定尺寸容器内添加大量内容
    const innerContent = document.getElementById('innerContent');
    for (let i = 0; i < 10; i++) {
        const p = document.createElement('p');
        p.textContent = `Added paragraph ${i + 1} with some content`;
        innerContent.appendChild(p);
    }
    
    // 获取添加后的布局
    const siblingRect2 = sibling.getBoundingClientRect();
    const footerRect2 = footer.getBoundingClientRect();
    
    // 验证兄弟元素布局未变化
    let passed = true;
    
    if (siblingRect1.top !== siblingRect2.top) {
        console.log(`  Sibling top changed: ${siblingRect1.top} -> ${siblingRect2.top}`);
        passed = false;
    }
    
    if (footerRect1.top !== footerRect2.top) {
        console.log(`  Footer top changed: ${footerRect1.top} -> ${footerRect2.top}`);
        passed = false;
    }
    
    logTest('Add content to fixed container does not affect siblings', passed);
    
    return passed;
}

// 测试 2: 移除内容从固定尺寸容器不影响兄弟元素
function testRemoveContentDoesNotAffectSibling() {
    setupPage();
    
    // 先添加一些内容
    const innerContent = document.getElementById('innerContent');
    for (let i = 0; i < 5; i++) {
        const p = document.createElement('p');
        p.id = `para${i}`;
        p.textContent = `Paragraph ${i}`;
        innerContent.appendChild(p);
    }
    
    // 获取兄弟元素的布局
    const sibling = document.getElementById('sibling');
    const siblingRect1 = sibling.getBoundingClientRect();
    
    // 移除内容
    for (let i = 0; i < 3; i++) {
        const p = document.getElementById(`para${i}`);
        if (p) p.remove();
    }
    
    // 获取移除后的布局
    const siblingRect2 = sibling.getBoundingClientRect();
    
    // 验证布局未变化
    const passed = siblingRect1.top === siblingRect2.top;
    
    logTest('Remove content from fixed container does not affect siblings', passed);
    
    return passed;
}

// 测试 3: 固定尺寸容器本身尺寸不变
function testContainerSizeUnchanged() {
    setupPage();
    
    const container = document.getElementById('fixedContainer');
    const rect1 = container.getBoundingClientRect();
    
    // 添加大量内容
    const innerContent = document.getElementById('innerContent');
    for (let i = 0; i < 20; i++) {
        const div = document.createElement('div');
        div.style.height = '30px';
        div.textContent = `Item ${i}`;
        innerContent.appendChild(div);
    }
    
    const rect2 = container.getBoundingClientRect();
    
    // 容器尺寸应该保持不变
    const passed = rect1.width === rect2.width && rect1.height === rect2.height;
    
    logTest('Fixed container size remains unchanged', passed);
    if (!passed) {
        console.log(`  Initial: ${rect1.width}x${rect1.height}`);
        console.log(`  After: ${rect2.width}x${rect2.height}`);
    }
    
    return passed;
}

// 测试 4: 嵌套固定尺寸容器
function testNestedFixedContainers() {
    document.body.innerHTML = `
        <div id="outer" style="width: 400px; height: 300px; border: 1px solid #333;">
            <div id="inner" style="width: 200px; height: 150px; border: 1px solid #666; margin: 20px;">
                <div id="content">Initial</div>
            </div>
        </div>
        <div id="below" style="height: 50px; background: #eee;">Below</div>
    `;
    
    // 获取外部元素布局
    const below = document.getElementById('below');
    const belowRect1 = below.getBoundingClientRect();
    
    // 在最内层添加内容
    const content = document.getElementById('content');
    for (let i = 0; i < 10; i++) {
        const p = document.createElement('p');
        p.textContent = `Nested content ${i}`;
        content.appendChild(p);
    }
    
    // 验证外部元素布局未变化
    const belowRect2 = below.getBoundingClientRect();
    const passed = belowRect1.top === belowRect2.top;
    
    logTest('Nested fixed containers maintain layout boundary', passed);
    
    return passed;
}

// 测试 5: px 单位的固定尺寸
function testPxUnitFixedSize() {
    document.body.innerHTML = `
        <div id="container" style="width: 250px; height: 180px; border: 1px solid #333;">
            <div id="content">Content</div>
        </div>
        <div id="after" style="height: 30px; background: #ddd;">After</div>
    `;
    
    const after = document.getElementById('after');
    const afterRect1 = after.getBoundingClientRect();
    
    // 添加内容
    const content = document.getElementById('content');
    for (let i = 0; i < 15; i++) {
        const div = document.createElement('div');
        div.textContent = `Line ${i}`;
        content.appendChild(div);
    }
    
    const afterRect2 = after.getBoundingClientRect();
    const passed = afterRect1.top === afterRect2.top;
    
    logTest('px unit fixed size acts as boundary', passed);
    
    return passed;
}

// 测试 6: 视口单位的固定尺寸 (vw, vh)
function testViewportUnitFixedSize() {
    document.body.innerHTML = `
        <div id="container" style="width: 50vw; height: 30vh; border: 1px solid #333;">
            <div id="content">Content</div>
        </div>
        <div id="after" style="height: 30px; background: #ddd;">After</div>
    `;
    
    const after = document.getElementById('after');
    const afterRect1 = after.getBoundingClientRect();
    
    // 添加内容
    const content = document.getElementById('content');
    for (let i = 0; i < 10; i++) {
        const div = document.createElement('div');
        div.textContent = `Line ${i}`;
        content.appendChild(div);
    }
    
    const afterRect2 = after.getBoundingClientRect();
    const passed = afterRect1.top === afterRect2.top;
    
    logTest('Viewport unit fixed size acts as boundary', passed);
    
    return passed;
}

// 运行所有测试
function runAllTests() {
    console.log('[TEST_START] Fixed Size Container Incremental Layout Tests');
    
    let allPassed = true;
    
    allPassed = testAddContentDoesNotAffectSibling() && allPassed;
    allPassed = testRemoveContentDoesNotAffectSibling() && allPassed;
    allPassed = testContainerSizeUnchanged() && allPassed;
    allPassed = testNestedFixedContainers() && allPassed;
    allPassed = testPxUnitFixedSize() && allPassed;
    allPassed = testViewportUnitFixedSize() && allPassed;
    
    console.log('[TEST_END]');
    
    if (allPassed) {
        console.log('All fixed size container tests passed!');
    } else {
        console.log('Some tests failed!');
    }
}

// 执行测试
runAllTests();
