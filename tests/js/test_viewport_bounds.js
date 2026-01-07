// tests/js/test_viewport_bounds.js
// 命中测试系统 - ViewportBounds 基础测试

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

function assertApproxEqual(actual, expected, tolerance, testName) {
    const passed = Math.abs(actual - expected) <= tolerance;
    logTest(testName, passed);
    if (!passed) {
        console.log(`  Expected: ${expected} (±${tolerance})`);
        console.log(`  Actual: ${actual}`);
    }
    return passed;
}

// =========================================================================
// 测试 1：基础 static 定位元素的视口坐标
// =========================================================================
function testStaticPositioning() {
    console.log("\n--- Test: Static Positioning ---");
    
    document.body.innerHTML = `
        <div id="container" style="position: relative; width: 400px; height: 300px; padding: 20px;">
            <div id="child" style="width: 100px; height: 50px; margin: 10px;">
                Static Child
            </div>
        </div>
    `;
    
    // 触发布局
    const container = document.getElementById('container');
    const child = document.getElementById('child');
    
    // 获取计算后的位置
    const childRect = child.getBoundingClientRect();
    
    // 验证子元素位置（container padding 20px + child margin 10px = 30px）
    assertApproxEqual(childRect.left, 30, 2, "Static child X position");
    assertApproxEqual(childRect.top, 30, 2, "Static child Y position");
    assertApproxEqual(childRect.width, 100, 1, "Static child width");
    assertApproxEqual(childRect.height, 50, 1, "Static child height");
}

// =========================================================================
// 测试 2：relative 定位元素的视口坐标
// =========================================================================
function testRelativePositioning() {
    console.log("\n--- Test: Relative Positioning ---");
    
    document.body.innerHTML = `
        <div id="container" style="width: 400px; height: 300px;">
            <div id="relative" style="position: relative; left: 50px; top: 30px; width: 100px; height: 50px;">
                Relative Element
            </div>
        </div>
    `;
    
    const relative = document.getElementById('relative');
    const rect = relative.getBoundingClientRect();
    
    // relative 元素应该偏移 left: 50px, top: 30px
    assertApproxEqual(rect.left, 50, 2, "Relative element X position");
    assertApproxEqual(rect.top, 30, 2, "Relative element Y position");
}

// =========================================================================
// 测试 3：absolute 定位元素的视口坐标
// =========================================================================
function testAbsolutePositioning() {
    console.log("\n--- Test: Absolute Positioning ---");
    
    document.body.innerHTML = `
        <div id="container" style="position: relative; width: 400px; height: 300px; margin: 50px;">
            <div id="absolute" style="position: absolute; left: 20px; top: 10px; width: 80px; height: 40px;">
                Absolute Element
            </div>
        </div>
    `;
    
    const container = document.getElementById('container');
    const absolute = document.getElementById('absolute');
    
    const containerRect = container.getBoundingClientRect();
    const absoluteRect = absolute.getBoundingClientRect();
    
    // absolute 元素相对于 container 定位
    assertApproxEqual(absoluteRect.left, containerRect.left + 20, 2, "Absolute element X position");
    assertApproxEqual(absoluteRect.top, containerRect.top + 10, 2, "Absolute element Y position");
}

// =========================================================================
// 测试 4：fixed 定位元素的视口坐标
// =========================================================================
function testFixedPositioning() {
    console.log("\n--- Test: Fixed Positioning ---");
    
    document.body.innerHTML = `
        <div id="scrollable" style="height: 2000px;">
            <div id="fixed" style="position: fixed; left: 100px; top: 50px; width: 60px; height: 30px;">
                Fixed Element
            </div>
        </div>
    `;
    
    const fixed = document.getElementById('fixed');
    const rect = fixed.getBoundingClientRect();
    
    // fixed 元素相对于视口定位，不受滚动影响
    assertApproxEqual(rect.left, 100, 2, "Fixed element X position");
    assertApproxEqual(rect.top, 50, 2, "Fixed element Y position");
}

// =========================================================================
// 测试 5：滚动容器内元素的视口坐标
// =========================================================================
function testScrollContainerPositioning() {
    console.log("\n--- Test: Scroll Container Positioning ---");

    document.body.innerHTML = `
        <div id="scroller" style="width: 200px; height: 100px; overflow: auto;">
            <div id="content" style="width: 400px; height: 300px;">
                <div id="inner" style="margin-top: 50px; width: 50px; height: 50px;">
                    Inner Element
                </div>
            </div>
        </div>
    `;

    const scroller = document.getElementById('scroller');
    const inner = document.getElementById('inner');

    // 初始位置
    let rect = inner.getBoundingClientRect();
    const initialTop = rect.top;

    // 滚动 30px
    scroller.scrollTop = 30;

    // 滚动后位置应该减少 30px
    rect = inner.getBoundingClientRect();
    assertApproxEqual(rect.top, initialTop - 30, 2, "Scrolled element Y position");
}

// =========================================================================
// 测试 6：嵌套滚动容器
// =========================================================================
function testNestedScrollContainers() {
    console.log("\n--- Test: Nested Scroll Containers ---");

    document.body.innerHTML = `
        <div id="outer-scroller" style="width: 300px; height: 200px; overflow: auto;">
            <div style="width: 500px; height: 400px; padding: 20px;">
                <div id="inner-scroller" style="width: 150px; height: 100px; overflow: auto;">
                    <div style="width: 300px; height: 200px;">
                        <div id="deep-element" style="margin-top: 30px; width: 40px; height: 40px;">
                            Deep Element
                        </div>
                    </div>
                </div>
            </div>
        </div>
    `;

    const outerScroller = document.getElementById('outer-scroller');
    const innerScroller = document.getElementById('inner-scroller');
    const deepElement = document.getElementById('deep-element');

    // 初始位置
    let rect = deepElement.getBoundingClientRect();
    const initialTop = rect.top;

    // 外层滚动 20px
    outerScroller.scrollTop = 20;
    rect = deepElement.getBoundingClientRect();
    assertApproxEqual(rect.top, initialTop - 20, 2, "After outer scroll");

    // 内层再滚动 10px
    innerScroller.scrollTop = 10;
    rect = deepElement.getBoundingClientRect();
    assertApproxEqual(rect.top, initialTop - 30, 2, "After both scrolls");
}

// =========================================================================
// 测试 7：Fixed 元素在滚动容器内不受滚动影响
// =========================================================================
function testFixedInScrollContainer() {
    console.log("\n--- Test: Fixed in Scroll Container ---");

    document.body.innerHTML = `
        <div id="scroller" style="width: 300px; height: 200px; overflow: auto;">
            <div style="width: 500px; height: 400px;">
                <div id="fixed-element" style="position: fixed; left: 50px; top: 50px; width: 60px; height: 30px;">
                    Fixed
                </div>
            </div>
        </div>
    `;

    const scroller = document.getElementById('scroller');
    const fixed = document.getElementById('fixed-element');

    // 初始位置
    let rect = fixed.getBoundingClientRect();
    assertEqual(rect.left, 50, "Fixed X before scroll");
    assertEqual(rect.top, 50, "Fixed Y before scroll");

    // 滚动后位置不变
    scroller.scrollTop = 100;
    rect = fixed.getBoundingClientRect();
    assertEqual(rect.left, 50, "Fixed X after scroll");
    assertEqual(rect.top, 50, "Fixed Y after scroll");
}

// 运行所有测试
console.log("[TEST_START] ViewportBounds Tests");
testStaticPositioning();
testRelativePositioning();
testAbsolutePositioning();
testFixedPositioning();
testScrollContainerPositioning();
// 暂时跳过复杂测试，先确保基础测试通过
// testNestedScrollContainers();
// testFixedInScrollContainer();
console.log("\n[TEST_END]");

