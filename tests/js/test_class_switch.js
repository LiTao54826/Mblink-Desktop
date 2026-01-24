// tests/js/test_class_switch.js
// 测试 CSS 类切换功能，特别是涉及 display 属性变化的场景

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

function assertNotEqual(actual, notExpected, testName) {
    const passed = actual !== notExpected;
    logTest(testName, passed);
    if (!passed) {
        console.log(`  Should not be: ${notExpected}`);
        console.log(`  Actual: ${actual}`);
    }
    return passed;
}

// 创建测试样式
function setupTestStyles() {
    const style = document.createElement('style');
    style.textContent = `
        .hidden { display: none; }
        .visible { display: block; }
        .page { display: none; }
        .page.active { display: block; }
        .red-bg { background-color: red; }
        .blue-bg { background-color: blue; }
    `;
    document.head.appendChild(style);
    return style;
}

// 测试用例 1: className 切换 display: none <-> display: block
function testClassNameDisplaySwitch() {
    console.log("[TEST_GROUP] className display switch");
    
    const div = document.createElement('div');
    div.id = 'test-display-switch';
    div.textContent = 'Test Element';
    document.body.appendChild(div);
    
    // 初始状态：无类名，应该可见
    div.className = '';
    
    // 切换到 hidden 类
    div.className = 'hidden';
    // 注意：这里我们测试的是类名是否正确设置
    assertEqual(div.className, 'hidden', 'className set to hidden');
    
    // 切换到 visible 类
    div.className = 'visible';
    assertEqual(div.className, 'visible', 'className set to visible');
    
    // 清理
    document.body.removeChild(div);
}

// 测试用例 2: page/active 模式切换
function testPageActiveSwitch() {
    console.log("[TEST_GROUP] page active switch");
    
    const page1 = document.createElement('div');
    page1.id = 'page1';
    page1.className = 'page active';
    page1.textContent = 'Page 1';
    
    const page2 = document.createElement('div');
    page2.id = 'page2';
    page2.className = 'page';
    page2.textContent = 'Page 2';
    
    document.body.appendChild(page1);
    document.body.appendChild(page2);
    
    // 初始状态
    assertEqual(page1.className, 'page active', 'page1 initial className');
    assertEqual(page2.className, 'page', 'page2 initial className');
    
    // 切换页面：page1 隐藏，page2 显示
    page1.className = 'page';
    page2.className = 'page active';
    
    assertEqual(page1.className, 'page', 'page1 after switch');
    assertEqual(page2.className, 'page active', 'page2 after switch');
    
    // 清理
    document.body.removeChild(page1);
    document.body.removeChild(page2);
}

// 测试用例 3: classList.add/remove 操作
function testClassListOperations() {
    console.log("[TEST_GROUP] classList operations");
    
    const div = document.createElement('div');
    div.id = 'test-classlist';
    div.className = 'visible';
    document.body.appendChild(div);
    
    // 测试 classList.add
    div.classList.add('red-bg');
    logTest('classList.add adds class', div.classList.contains('red-bg'));
    
    // 测试 classList.remove
    div.classList.remove('visible');
    logTest('classList.remove removes class', !div.classList.contains('visible'));
    
    // 测试 classList.toggle
    div.classList.toggle('hidden');
    logTest('classList.toggle adds class', div.classList.contains('hidden'));
    
    div.classList.toggle('hidden');
    logTest('classList.toggle removes class', !div.classList.contains('hidden'));
    
    // 测试 classList.replace
    div.classList.add('red-bg');
    div.classList.replace('red-bg', 'blue-bg');
    logTest('classList.replace works', 
        !div.classList.contains('red-bg') && div.classList.contains('blue-bg'));
    
    // 清理
    document.body.removeChild(div);
}

// 测试用例 4: 多次快速切换
function testRapidSwitch() {
    console.log("[TEST_GROUP] rapid class switch");
    
    const div = document.createElement('div');
    div.id = 'test-rapid';
    document.body.appendChild(div);
    
    // 快速切换多次
    for (let i = 0; i < 10; i++) {
        div.className = (i % 2 === 0) ? 'hidden' : 'visible';
    }
    
    // 最终状态应该是 visible（因为 10 是偶数，最后一次 i=9 是奇数）
    assertEqual(div.className, 'visible', 'rapid switch final state');
    
    // 清理
    document.body.removeChild(div);
}

// 运行所有测试
console.log("[TEST_START] CSS Class Switch Tests");

const styleElement = setupTestStyles();

testClassNameDisplaySwitch();
testPageActiveSwitch();
testClassListOperations();
testRapidSwitch();

// 清理样式
document.head.removeChild(styleElement);

console.log("[TEST_END]");
