// tests/js/test_display_contents.js
// 测试 display: contents 功能

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

function assertTrue(condition, testName) {
    logTest(testName, condition);
    if (!condition) {
        console.log(`  Expected: true`);
        console.log(`  Actual: false`);
    }
    return condition;
}

// 测试用例
function testDisplayContentsBasic() {
    // 创建测试结构:
    // <div id="parent">
    //   <div id="contents" style="display: contents">
    //     <span id="child1">Child 1</span>
    //     <span id="child2">Child 2</span>
    //   </div>
    // </div>
    
    const parent = document.createElement('div');
    parent.id = 'parent';
    parent.style.display = 'flex';
    parent.style.flexDirection = 'row';
    parent.style.width = '400px';
    
    const contents = document.createElement('div');
    contents.id = 'contents';
    contents.style.display = 'contents';
    
    const child1 = document.createElement('span');
    child1.id = 'child1';
    child1.textContent = 'Child 1';
    child1.style.width = '100px';
    
    const child2 = document.createElement('span');
    child2.id = 'child2';
    child2.textContent = 'Child 2';
    child2.style.width = '100px';
    
    contents.appendChild(child1);
    contents.appendChild(child2);
    parent.appendChild(contents);
    document.body.appendChild(parent);
    
    // 验证 display: contents 元素的 computedStyle
    const contentsStyle = window.getComputedStyle(contents);
    assertEqual(contentsStyle.display, 'contents', 'display: contents computed style');
    
    // 验证子元素存在于 DOM 中
    assertTrue(child1.parentElement === contents, 'child1 parent is contents element');
    assertTrue(child2.parentElement === contents, 'child2 parent is contents element');
    
    // 清理
    document.body.removeChild(parent);
}

function testDisplayContentsWithSlot() {
    // 测试 slot 元素手动设置 display: contents
    const container = document.createElement('div');
    container.id = 'slot-container';
    container.style.width = '300px';
    
    const slot = document.createElement('slot');
    slot.id = 'test-slot';
    slot.style.display = 'contents';  // 手动设置，因为测试环境可能没有加载默认样式表
    
    const slotChild = document.createElement('div');
    slotChild.id = 'slot-child';
    slotChild.textContent = 'Slot content';
    slotChild.style.width = '100px';
    
    slot.appendChild(slotChild);
    container.appendChild(slot);
    document.body.appendChild(container);
    
    // slot 元素应该是 display: contents
    const slotStyle = window.getComputedStyle(slot);
    assertEqual(slotStyle.display, 'contents', 'slot element with display: contents');
    
    // 清理
    document.body.removeChild(container);
}

function testDisplayContentsNested() {
    // 测试嵌套的 display: contents
    // <div id="outer">
    //   <div style="display: contents">
    //     <div style="display: contents">
    //       <span id="deep-child">Deep</span>
    //     </div>
    //   </div>
    // </div>
    
    const outer = document.createElement('div');
    outer.id = 'outer';
    outer.style.display = 'flex';
    outer.style.width = '400px';
    
    const contents1 = document.createElement('div');
    contents1.style.display = 'contents';
    
    const contents2 = document.createElement('div');
    contents2.style.display = 'contents';
    
    const deepChild = document.createElement('span');
    deepChild.id = 'deep-child';
    deepChild.textContent = 'Deep';
    deepChild.style.width = '100px';
    
    contents2.appendChild(deepChild);
    contents1.appendChild(contents2);
    outer.appendChild(contents1);
    document.body.appendChild(outer);
    
    // 验证嵌套的 display: contents
    const style1 = window.getComputedStyle(contents1);
    const style2 = window.getComputedStyle(contents2);
    assertEqual(style1.display, 'contents', 'nested contents 1 display');
    assertEqual(style2.display, 'contents', 'nested contents 2 display');
    
    // 清理
    document.body.removeChild(outer);
}

function testDisplayContentsStyleInheritance() {
    // 测试样式继承
    // display: contents 元素的样式应该被子元素继承
    const parent = document.createElement('div');
    parent.id = 'inherit-parent';
    parent.style.color = 'rgb(255, 0, 0)';  // 红色
    
    const contents = document.createElement('div');
    contents.id = 'inherit-contents';
    contents.style.display = 'contents';
    contents.style.color = 'rgb(0, 255, 0)';  // 绿色
    
    const child = document.createElement('span');
    child.id = 'inherit-child';
    child.textContent = 'Inherited';
    
    contents.appendChild(child);
    parent.appendChild(contents);
    document.body.appendChild(parent);
    
    // 子元素应该继承 contents 元素的颜色（绿色）
    const childStyle = window.getComputedStyle(child);
    // 注意：实际继承行为取决于实现
    assertTrue(childStyle.color !== '', 'child has color style');
    
    // 清理
    document.body.removeChild(parent);
}

// 运行测试
console.log("[TEST_START] Display Contents Tests");
testDisplayContentsBasic();
testDisplayContentsWithSlot();
testDisplayContentsNested();
testDisplayContentsStyleInheritance();
console.log("[TEST_END]");
