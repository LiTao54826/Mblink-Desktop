/**
 * @file test_virtual_list_incremental.js
 * @brief 虚拟列表增量布局测试
 * 
 * 测试滚动容器内元素增删的增量布局优化
 * 验证只有滚动容器被标记为脏，外部元素不受影响
 * 
 * **Feature: incremental-layout-boundary**
 * **Property 2: Scroll Container Boundary**
 * **Property 3: Scroll Dimensions Update**
 * **Validates: Requirements 2.1, 2.2, 2.3, 2.4**
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

// 创建基础页面结构
function setupPage() {
    document.body.innerHTML = `
        <div id="app" style="width: 100%; height: 100%;">
            <div id="header" style="height: 60px; background: #333; color: white;">
                Header - Should not be affected
            </div>
            <div id="scrollContainer" style="width: 300px; height: 200px; overflow: auto; border: 1px solid #ccc;">
                <div id="listContent">
                    <div class="item" style="height: 40px; border-bottom: 1px solid #eee;">Item 1</div>
                    <div class="item" style="height: 40px; border-bottom: 1px solid #eee;">Item 2</div>
                    <div class="item" style="height: 40px; border-bottom: 1px solid #eee;">Item 3</div>
                </div>
            </div>
            <div id="footer" style="height: 40px; background: #666; color: white;">
                Footer - Should not be affected
            </div>
        </div>
    `;
}

// 创建列表项
function createListItem(index) {
    const item = document.createElement('div');
    item.className = 'item';
    item.style.height = '40px';
    item.style.borderBottom = '1px solid #eee';
    item.textContent = `Item ${index}`;
    return item;
}

// 测试 1: 添加元素到滚动容器不影响外部元素
function testAddItemDoesNotAffectOutside() {
    setupPage();
    
    // 获取外部元素的初始布局
    const header = document.getElementById('header');
    const footer = document.getElementById('footer');
    
    const headerRect1 = header.getBoundingClientRect();
    const footerRect1 = footer.getBoundingClientRect();
    
    // 添加元素到滚动容器
    const listContent = document.getElementById('listContent');
    const newItem = createListItem(4);
    listContent.appendChild(newItem);
    
    // 获取添加后外部元素的布局
    const headerRect2 = header.getBoundingClientRect();
    const footerRect2 = footer.getBoundingClientRect();
    
    // 验证外部元素布局未变化
    let passed = true;
    
    if (headerRect1.top !== headerRect2.top || headerRect1.height !== headerRect2.height) {
        console.log('  Header layout changed after adding item to scroll container');
        passed = false;
    }
    
    if (footerRect1.top !== footerRect2.top || footerRect1.height !== footerRect2.height) {
        console.log('  Footer layout changed after adding item to scroll container');
        passed = false;
    }
    
    logTest('Add item to scroll container does not affect outside elements', passed);
    
    return passed;
}

// 测试 2: 移除元素从滚动容器不影响外部元素
function testRemoveItemDoesNotAffectOutside() {
    setupPage();
    
    // 获取外部元素的初始布局
    const header = document.getElementById('header');
    const footer = document.getElementById('footer');
    
    const headerRect1 = header.getBoundingClientRect();
    const footerRect1 = footer.getBoundingClientRect();
    
    // 移除滚动容器中的元素
    const listContent = document.getElementById('listContent');
    const items = listContent.querySelectorAll('.item');
    if (items.length > 0) {
        items[0].remove();
    }
    
    // 获取移除后外部元素的布局
    const headerRect2 = header.getBoundingClientRect();
    const footerRect2 = footer.getBoundingClientRect();
    
    // 验证外部元素布局未变化
    let passed = true;
    
    if (headerRect1.top !== headerRect2.top) {
        console.log('  Header position changed after removing item');
        passed = false;
    }
    
    if (footerRect1.top !== footerRect2.top) {
        console.log('  Footer position changed after removing item');
        passed = false;
    }
    
    logTest('Remove item from scroll container does not affect outside elements', passed);
    
    return passed;
}

// 测试 3: 滚动容器尺寸保持不变
function testScrollContainerSizeUnchanged() {
    setupPage();
    
    const scrollContainer = document.getElementById('scrollContainer');
    const rect1 = scrollContainer.getBoundingClientRect();
    
    // 添加多个元素
    const listContent = document.getElementById('listContent');
    for (let i = 4; i <= 10; i++) {
        listContent.appendChild(createListItem(i));
    }
    
    const rect2 = scrollContainer.getBoundingClientRect();
    
    // 滚动容器的外部尺寸应该保持不变（因为有固定尺寸）
    const passed = rect1.width === rect2.width && rect1.height === rect2.height;
    
    logTest('Scroll container size remains unchanged after adding items', passed);
    
    return passed;
}

// 测试 4: scrollHeight 正确更新
function testScrollHeightUpdates() {
    setupPage();
    
    const scrollContainer = document.getElementById('scrollContainer');
    const initialScrollHeight = scrollContainer.scrollHeight;
    
    // 添加元素
    const listContent = document.getElementById('listContent');
    for (let i = 4; i <= 10; i++) {
        listContent.appendChild(createListItem(i));
    }
    
    const newScrollHeight = scrollContainer.scrollHeight;
    
    // scrollHeight 应该增加（每个 item 40px + 1px border）
    const passed = newScrollHeight > initialScrollHeight;
    
    logTest('scrollHeight updates correctly after adding items', passed);
    if (!passed) {
        console.log(`  Initial scrollHeight: ${initialScrollHeight}`);
        console.log(`  New scrollHeight: ${newScrollHeight}`);
    }
    
    return passed;
}

// 测试 5: 批量添加元素
function testBatchAddItems() {
    setupPage();
    
    // 获取外部元素布局
    const footer = document.getElementById('footer');
    const footerRect1 = footer.getBoundingClientRect();
    
    // 批量添加 20 个元素
    const listContent = document.getElementById('listContent');
    for (let i = 4; i <= 23; i++) {
        listContent.appendChild(createListItem(i));
    }
    
    // 验证外部元素布局未变化
    const footerRect2 = footer.getBoundingClientRect();
    const passed = footerRect1.top === footerRect2.top;
    
    logTest('Batch add items does not affect outside elements', passed);
    
    return passed;
}

// 测试 6: 固定尺寸容器作为布局边界
function testFixedSizeContainerBoundary() {
    document.body.innerHTML = `
        <div id="outside" style="height: 50px; background: #eee;">Outside</div>
        <div id="fixedContainer" style="width: 200px; height: 150px; border: 1px solid #333;">
            <div id="inner">Initial content</div>
        </div>
        <div id="below" style="height: 50px; background: #ddd;">Below</div>
    `;
    
    // 获取外部元素布局
    const outside = document.getElementById('outside');
    const below = document.getElementById('below');
    const outsideRect1 = outside.getBoundingClientRect();
    const belowRect1 = below.getBoundingClientRect();
    
    // 在固定尺寸容器内添加内容
    const inner = document.getElementById('inner');
    for (let i = 0; i < 5; i++) {
        const p = document.createElement('p');
        p.textContent = `Added paragraph ${i + 1}`;
        inner.appendChild(p);
    }
    
    // 验证外部元素布局未变化
    const outsideRect2 = outside.getBoundingClientRect();
    const belowRect2 = below.getBoundingClientRect();
    
    const passed = outsideRect1.top === outsideRect2.top && 
                   belowRect1.top === belowRect2.top;
    
    logTest('Fixed size container acts as layout boundary', passed);
    
    return passed;
}

// 运行所有测试
function runAllTests() {
    console.log('[TEST_START] Virtual List Incremental Layout Tests');
    
    let allPassed = true;
    
    allPassed = testAddItemDoesNotAffectOutside() && allPassed;
    allPassed = testRemoveItemDoesNotAffectOutside() && allPassed;
    allPassed = testScrollContainerSizeUnchanged() && allPassed;
    allPassed = testScrollHeightUpdates() && allPassed;
    allPassed = testBatchAddItems() && allPassed;
    allPassed = testFixedSizeContainerBoundary() && allPassed;
    
    console.log('[TEST_END]');
    
    if (allPassed) {
        console.log('All virtual list incremental layout tests passed!');
    } else {
        console.log('Some tests failed!');
    }
}

// 执行测试
runAllTests();
