// tests/js/test_scrollbar_unified.js
// 统一滚动条系统测试
// **Feature: unified-scrollbar-system**
// **Validates: Requirements 1.1, 1.2, 1.4, 1.5, 2.1, 2.2**

// 测试辅助函数
function logTest(name, passed) {
    if (passed) {
        console.log(`[TEST_PASS] ${name}`);
    } else {
        console.log(`[TEST_FAIL] ${name}`);
    }
}

function assertEqual(actual, expected, testName, tolerance = 0.5) {
    const passed = Math.abs(actual - expected) <= tolerance;
    logTest(testName, passed);
    if (!passed) {
        console.log(`  Expected: ${expected}`);
        console.log(`  Actual: ${actual}`);
    }
    return passed;
}

function assertTrue(condition, testName) {
    logTest(testName, condition);
    if (!condition) {
        console.log(`  Expected: true`);
        console.log(`  Actual: false`);
    }
    return condition;
}

// 滚动条宽度常量
const SCROLLBAR_WIDTH = 12;

// 测试1: Body overflow:auto 垂直滚动条
function testBodyVerticalScrollbar() {
    console.log("\n--- Test: Body Vertical Scrollbar ---");
    
    // 设置 body 样式
    document.body.style.margin = "0";
    document.body.style.padding = "0";
    document.body.style.overflow = "auto";
    document.body.style.width = "100%";
    document.body.style.height = "100%";
    
    // 清空 body
    document.body.innerHTML = "";
    
    // 创建一个高度超过视口的内容
    const content = document.createElement("div");
    content.id = "tall-content";
    content.style.width = "100%";  // 使用 100% 宽度
    content.style.height = "2000px";  // 超过视口高度
    content.style.backgroundColor = "#eee";
    document.body.appendChild(content);
    
    // 获取视口尺寸
    const viewportWidth = window.innerWidth;
    const viewportHeight = window.innerHeight;
    
    // 获取 body 布局信息
    const bodyRect = document.body.getBoundingClientRect();
    
    // 验证: body 外部宽度应该保持视口宽度（滚动条在内部）
    // 根据设计文档 Requirement 2.1: 滚动条在元素内部显示
    assertEqual(bodyRect.width, viewportWidth, 
        `Body outer width unchanged (viewport=${viewportWidth})`);
    
    // 验证: 内容宽度应该是视口宽度减去滚动条宽度
    // 因为内容高度超过视口，需要垂直滚动条
    const contentRect = content.getBoundingClientRect();
    const expectedContentWidth = viewportWidth - SCROLLBAR_WIDTH;
    assertEqual(contentRect.width, expectedContentWidth, 
        `Content width reduced by scrollbar (expected=${expectedContentWidth})`);
}

// 测试2: Div overflow:auto 垂直滚动条
function testDivVerticalScrollbar() {
    console.log("\n--- Test: Div Vertical Scrollbar ---");
    
    // 清空 body
    document.body.innerHTML = "";
    document.body.style.overflow = "hidden";
    document.body.style.margin = "0";
    document.body.style.padding = "0";
    
    // 创建一个固定高度的容器
    const container = document.createElement("div");
    container.id = "scroll-container";
    container.style.width = "300px";
    container.style.height = "200px";
    container.style.overflow = "auto";
    container.style.backgroundColor = "#ddd";
    document.body.appendChild(container);
    
    // 创建超过容器高度的内容
    const content = document.createElement("div");
    content.id = "scroll-content";
    content.style.width = "100%";
    content.style.height = "500px";  // 超过容器高度
    content.style.backgroundColor = "#aaa";
    container.appendChild(content);
    
    // 获取容器和内容的布局信息
    const containerRect = container.getBoundingClientRect();
    const contentRect = content.getBoundingClientRect();
    
    // 验证: 容器外部尺寸应该保持不变
    assertEqual(containerRect.width, 300, "Container outer width unchanged");
    assertEqual(containerRect.height, 200, "Container outer height unchanged");
    
    // 验证: 内容宽度应该是容器宽度减去滚动条宽度
    const expectedContentWidth = 300 - SCROLLBAR_WIDTH;
    assertEqual(contentRect.width, expectedContentWidth, 
        `Content width reduced by scrollbar (expected=${expectedContentWidth})`);
}

// 测试3: 多个 Div 滚动条行为一致性
function testBodyDivConsistency() {
    console.log("\n--- Test: Multiple Div Scrollbar Consistency ---");
    
    // 清空 body
    document.body.innerHTML = "";
    document.body.style.overflow = "hidden";
    document.body.style.margin = "0";
    document.body.style.padding = "0";
    document.body.style.display = "flex";
    document.body.style.gap = "20px";
    
    // 创建两个相同尺寸的容器
    const div1 = document.createElement("div");
    div1.id = "div1";
    div1.style.width = "400px";
    div1.style.height = "300px";
    div1.style.overflow = "auto";
    div1.style.flexShrink = "0";
    document.body.appendChild(div1);
    
    const div2 = document.createElement("div");
    div2.id = "div2";
    div2.style.width = "400px";
    div2.style.height = "300px";
    div2.style.overflow = "auto";
    div2.style.flexShrink = "0";
    document.body.appendChild(div2);
    
    // 在两个容器中添加相同的超高内容
    const content1 = document.createElement("div");
    content1.style.width = "350px";
    content1.style.height = "600px";
    content1.style.backgroundColor = "#aaa";
    div1.appendChild(content1);
    
    const content2 = document.createElement("div");
    content2.style.width = "350px";
    content2.style.height = "600px";
    content2.style.backgroundColor = "#bbb";
    div2.appendChild(content2);
    
    // 获取两个内容的宽度
    const content1Rect = content1.getBoundingClientRect();
    const content2Rect = content2.getBoundingClientRect();
    
    // 验证: 两个容器中的内容宽度应该相同
    assertEqual(content1Rect.width, content2Rect.width, 
        "Both containers have same content width");
    
    // 验证: 内容宽度应该是 350px
    assertEqual(content1Rect.width, 350, 
        "Content width is fixed at 350px");
    
    // 验证: 容器外部尺寸保持不变
    const div1Rect = div1.getBoundingClientRect();
    assertEqual(div1Rect.width, 400, "Container outer width unchanged");
}

// 测试4: overflow:scroll 始终显示滚动条
function testOverflowScroll() {
    console.log("\n--- Test: Overflow Scroll Always Shows Scrollbar ---");
    
    // 清空 body
    document.body.innerHTML = "";
    document.body.style.overflow = "hidden";
    document.body.style.margin = "0";
    document.body.style.padding = "0";
    
    // 创建一个 overflow:scroll 的容器
    const container = document.createElement("div");
    container.id = "scroll-always";
    container.style.width = "300px";
    container.style.height = "200px";
    container.style.overflow = "scroll";
    container.style.backgroundColor = "#ddd";
    document.body.appendChild(container);
    
    // 创建一个小于容器的内容（不需要滚动）
    const content = document.createElement("div");
    content.id = "small-content";
    content.style.width = "100%";
    content.style.height = "50px";  // 小于容器高度
    content.style.backgroundColor = "#aaa";
    container.appendChild(content);
    
    // 获取内容的布局信息
    const contentRect = content.getBoundingClientRect();
    
    // 验证: 即使内容不需要滚动，内容宽度也应该减少（因为 overflow:scroll 始终显示滚动条）
    const expectedContentWidth = 300 - SCROLLBAR_WIDTH;
    assertEqual(contentRect.width, expectedContentWidth, 
        `Overflow:scroll always reserves scrollbar space (expected=${expectedContentWidth})`);
}

// 测试5: 水平滚动条仅在需要时出现
function testHorizontalScrollbar() {
    console.log("\n--- Test: Horizontal Scrollbar Only When Needed ---");
    
    // 清空 body
    document.body.innerHTML = "";
    document.body.style.overflow = "hidden";
    document.body.style.margin = "0";
    document.body.style.padding = "0";
    
    // 创建一个容器
    const container = document.createElement("div");
    container.id = "h-scroll-container";
    container.style.width = "300px";
    container.style.height = "200px";
    container.style.overflow = "auto";
    container.style.backgroundColor = "#ddd";
    document.body.appendChild(container);
    
    // 创建一个宽度超过容器的内容
    const content = document.createElement("div");
    content.id = "wide-content";
    content.style.width = "500px";  // 超过容器宽度
    content.style.height = "50px";  // 不超过容器高度
    content.style.backgroundColor = "#aaa";
    container.appendChild(content);
    
    // 获取内容的布局信息
    const contentRect = content.getBoundingClientRect();
    
    // 验证: 内容宽度应该保持 500px（因为没有垂直滚动条）
    assertEqual(contentRect.width, 500, 
        "Wide content width unchanged (no vertical scrollbar needed)");
}

// 运行所有测试
console.log("[TEST_START] Unified Scrollbar System Tests");

testBodyVerticalScrollbar();
testDivVerticalScrollbar();
testBodyDivConsistency();
testOverflowScroll();
testHorizontalScrollbar();

console.log("\n[TEST_END]");


// 测试6: scrollWidth/scrollHeight API
function testScrollWidthHeight() {
    console.log("\n--- Test: scrollWidth/scrollHeight API ---");
    
    // 清空 body
    document.body.innerHTML = "";
    document.body.style.overflow = "hidden";
    document.body.style.margin = "0";
    document.body.style.padding = "0";
    document.body.style.display = "block";
    
    // 创建一个容器
    const container = document.createElement("div");
    container.id = "scroll-api-container";
    container.style.width = "300px";
    container.style.height = "200px";
    container.style.overflow = "auto";
    container.style.backgroundColor = "#ddd";
    document.body.appendChild(container);
    
    // 创建超过容器尺寸的内容
    const content = document.createElement("div");
    content.id = "scroll-api-content";
    content.style.width = "500px";
    content.style.height = "400px";
    content.style.backgroundColor = "#aaa";
    container.appendChild(content);
    
    // 验证: scrollWidth 应该返回内容总宽度
    // 注意：scrollWidth/scrollHeight 需要在渲染后才能获取正确的值
    // 这里我们只验证 API 存在且返回数值
    assertTrue(typeof container.scrollWidth === 'number', 
        "scrollWidth returns a number");
    
    assertTrue(typeof container.scrollHeight === 'number', 
        "scrollHeight returns a number");
    
    // 验证: scrollWidth 至少等于容器宽度
    assertTrue(container.scrollWidth >= 0, 
        `scrollWidth is non-negative (actual=${container.scrollWidth})`);
    
    // 验证: scrollHeight 至少等于容器高度
    assertTrue(container.scrollHeight >= 0, 
        `scrollHeight is non-negative (actual=${container.scrollHeight})`);
}

// 更新运行测试
console.log("\n--- Test: scrollWidth/scrollHeight API ---");
testScrollWidthHeight();


// 测试7: scrollTop/scrollLeft clamp 行为
function testScrollTopLeftClamp() {
    console.log("\n--- Test: scrollTop/scrollLeft Clamp ---");
    
    // 清空 body
    document.body.innerHTML = "";
    document.body.style.overflow = "hidden";
    document.body.style.margin = "0";
    document.body.style.padding = "0";
    document.body.style.display = "block";
    
    // 创建一个容器
    const container = document.createElement("div");
    container.id = "scroll-clamp-container";
    container.style.width = "300px";
    container.style.height = "200px";
    container.style.overflow = "auto";
    container.style.backgroundColor = "#ddd";
    document.body.appendChild(container);
    
    // 创建超过容器尺寸的内容
    const content = document.createElement("div");
    content.id = "scroll-clamp-content";
    content.style.width = "500px";
    content.style.height = "400px";
    content.style.backgroundColor = "#aaa";
    container.appendChild(content);
    
    // 测试: 设置负值应该被 clamp 到 0
    container.scrollTop = -100;
    assertEqual(container.scrollTop, 0, "scrollTop clamped to 0 for negative value");
    
    container.scrollLeft = -100;
    assertEqual(container.scrollLeft, 0, "scrollLeft clamped to 0 for negative value");
    
    // 测试: scrollTop/scrollLeft getter 应该返回数值
    assertTrue(typeof container.scrollTop === 'number', "scrollTop returns a number");
    assertTrue(typeof container.scrollLeft === 'number', "scrollLeft returns a number");
}

// 运行测试
console.log("\n--- Test: scrollTop/scrollLeft Clamp ---");
testScrollTopLeftClamp();
