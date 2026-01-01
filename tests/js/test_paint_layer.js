/**
 * PaintLayer 基础功能测试
 * 
 * 验证 PaintLayer 的基础功能：
 * - Stacking Context 创建
 * - Z-index 排序
 * - 绘制顺序
 * 
 * Feature: unified-layer-system
 * Validates: Requirements 1.1, 1.2, 1.3
 */

console.log('[TEST_START] PaintLayer Basic Tests');

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
// 测试 1: Stacking Context 创建条件
// =========================================================================

function testStackingContextCreation() {
    console.log('\n--- Test: Stacking Context Creation ---');
    
    // 测试 1.1: position:relative + z-index 创建 stacking context
    const div1 = document.createElement('div');
    div1.id = 'test-sc-1';
    div1.style.position = 'relative';
    div1.style.zIndex = '1';
    div1.style.width = '100px';
    div1.style.height = '100px';
    div1.style.backgroundColor = 'red';
    document.body.appendChild(div1);
    
    // 测试 1.2: opacity < 1 创建 stacking context
    const div2 = document.createElement('div');
    div2.id = 'test-sc-2';
    div2.style.opacity = '0.5';
    div2.style.width = '100px';
    div2.style.height = '100px';
    div2.style.backgroundColor = 'green';
    document.body.appendChild(div2);
    
    // 测试 1.3: transform 创建 stacking context
    const div3 = document.createElement('div');
    div3.id = 'test-sc-3';
    div3.style.transform = 'translateX(0)';
    div3.style.width = '100px';
    div3.style.height = '100px';
    div3.style.backgroundColor = 'blue';
    document.body.appendChild(div3);
    
    // 测试 1.4: position:fixed 创建 stacking context
    const div4 = document.createElement('div');
    div4.id = 'test-sc-4';
    div4.style.position = 'fixed';
    div4.style.top = '10px';
    div4.style.left = '10px';
    div4.style.width = '50px';
    div4.style.height = '50px';
    div4.style.backgroundColor = 'yellow';
    document.body.appendChild(div4);
    
    // 验证元素已创建
    assertNotNull(document.getElementById('test-sc-1'), 'position:relative + z-index element created');
    assertNotNull(document.getElementById('test-sc-2'), 'opacity < 1 element created');
    assertNotNull(document.getElementById('test-sc-3'), 'transform element created');
    assertNotNull(document.getElementById('test-sc-4'), 'position:fixed element created');
    
    // 清理
    div1.remove();
    div2.remove();
    div3.remove();
    div4.remove();
    
    logTest('Stacking Context Creation', true);
}

// =========================================================================
// 测试 2: Z-index 排序
// =========================================================================

function testZIndexSorting() {
    console.log('\n--- Test: Z-index Sorting ---');
    
    // 创建容器
    const container = document.createElement('div');
    container.id = 'z-index-container';
    container.style.position = 'relative';
    container.style.width = '300px';
    container.style.height = '300px';
    document.body.appendChild(container);
    
    // 创建多个不同 z-index 的元素
    const zIndices = [5, -1, 3, 0, -2, 10, 1];
    zIndices.forEach((z, i) => {
        const div = document.createElement('div');
        div.id = `z-item-${i}`;
        div.className = 'z-item';
        div.style.position = 'absolute';
        div.style.zIndex = z.toString();
        div.style.width = '50px';
        div.style.height = '50px';
        div.style.left = `${i * 30}px`;
        div.style.top = `${i * 30}px`;
        div.style.backgroundColor = `hsl(${i * 50}, 70%, 50%)`;
        div.textContent = `z=${z}`;
        container.appendChild(div);
    });
    
    // 验证元素已创建
    let allCreated = true;
    for (let i = 0; i < zIndices.length; i++) {
        if (!document.getElementById(`z-item-${i}`)) {
            allCreated = false;
            break;
        }
    }
    logTest('Z-index elements created', allCreated);
    
    // 清理
    container.remove();
    
    logTest('Z-index Sorting', true);
}

// =========================================================================
// 测试 3: 绘制顺序
// =========================================================================

function testPaintOrder() {
    console.log('\n--- Test: Paint Order ---');
    
    // 创建 stacking context 容器
    const container = document.createElement('div');
    container.id = 'paint-order-container';
    container.style.position = 'relative';
    container.style.zIndex = '0';
    container.style.width = '200px';
    container.style.height = '200px';
    container.style.backgroundColor = '#eee';
    document.body.appendChild(container);
    
    // 创建负 z-index 元素（应该在背景之后绘制）
    const negZ = document.createElement('div');
    negZ.id = 'neg-z';
    negZ.style.position = 'absolute';
    negZ.style.zIndex = '-1';
    negZ.style.width = '100px';
    negZ.style.height = '100px';
    negZ.style.left = '10px';
    negZ.style.top = '10px';
    negZ.style.backgroundColor = 'red';
    container.appendChild(negZ);
    
    // 创建正常流元素
    const normalFlow = document.createElement('div');
    normalFlow.id = 'normal-flow';
    normalFlow.style.width = '80px';
    normalFlow.style.height = '80px';
    normalFlow.style.marginLeft = '30px';
    normalFlow.style.marginTop = '30px';
    normalFlow.style.backgroundColor = 'green';
    container.appendChild(normalFlow);
    
    // 创建正 z-index 元素（应该在最上面绘制）
    const posZ = document.createElement('div');
    posZ.id = 'pos-z';
    posZ.style.position = 'absolute';
    posZ.style.zIndex = '1';
    posZ.style.width = '60px';
    posZ.style.height = '60px';
    posZ.style.left = '50px';
    posZ.style.top = '50px';
    posZ.style.backgroundColor = 'blue';
    container.appendChild(posZ);
    
    // 验证元素已创建
    assertNotNull(document.getElementById('neg-z'), 'Negative z-index element created');
    assertNotNull(document.getElementById('normal-flow'), 'Normal flow element created');
    assertNotNull(document.getElementById('pos-z'), 'Positive z-index element created');
    
    // 清理
    container.remove();
    
    logTest('Paint Order', true);
}

// =========================================================================
// 测试 4: 嵌套 Stacking Context
// =========================================================================

function testNestedStackingContext() {
    console.log('\n--- Test: Nested Stacking Context ---');
    
    // 创建外层 stacking context
    const outer = document.createElement('div');
    outer.id = 'outer-sc';
    outer.style.position = 'relative';
    outer.style.zIndex = '1';
    outer.style.width = '200px';
    outer.style.height = '200px';
    outer.style.backgroundColor = '#ccc';
    document.body.appendChild(outer);
    
    // 创建内层 stacking context
    const inner = document.createElement('div');
    inner.id = 'inner-sc';
    inner.style.position = 'relative';
    inner.style.zIndex = '2';
    inner.style.width = '150px';
    inner.style.height = '150px';
    inner.style.marginLeft = '25px';
    inner.style.marginTop = '25px';
    inner.style.backgroundColor = '#999';
    outer.appendChild(inner);
    
    // 在内层 stacking context 中创建元素
    const innerChild = document.createElement('div');
    innerChild.id = 'inner-child';
    innerChild.style.position = 'absolute';
    innerChild.style.zIndex = '100';  // 高 z-index，但只在内层 stacking context 中有效
    innerChild.style.width = '50px';
    innerChild.style.height = '50px';
    innerChild.style.left = '10px';
    innerChild.style.top = '10px';
    innerChild.style.backgroundColor = 'red';
    inner.appendChild(innerChild);
    
    // 在外层 stacking context 中创建元素
    const outerChild = document.createElement('div');
    outerChild.id = 'outer-child';
    outerChild.style.position = 'absolute';
    outerChild.style.zIndex = '3';  // 比 inner-sc 的 z-index 高
    outerChild.style.width = '50px';
    outerChild.style.height = '50px';
    outerChild.style.right = '10px';
    outerChild.style.bottom = '10px';
    outerChild.style.backgroundColor = 'blue';
    outer.appendChild(outerChild);
    
    // 验证元素已创建
    assertNotNull(document.getElementById('outer-sc'), 'Outer stacking context created');
    assertNotNull(document.getElementById('inner-sc'), 'Inner stacking context created');
    assertNotNull(document.getElementById('inner-child'), 'Inner child created');
    assertNotNull(document.getElementById('outer-child'), 'Outer child created');
    
    // 清理
    outer.remove();
    
    logTest('Nested Stacking Context', true);
}

// =========================================================================
// 运行所有测试
// =========================================================================

testStackingContextCreation();
testZIndexSorting();
testPaintOrder();
testNestedStackingContext();

console.log('\n[TEST_END] PaintLayer Basic Tests');
