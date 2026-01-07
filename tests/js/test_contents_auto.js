// 自动化测试 display: contents 增量更新
// 测试场景：模拟 Preact 的 render 行为，验证子元素样式是否正确

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

function assertInRange(actual, min, max, testName) {
    const passed = actual >= min && actual <= max;
    logTest(testName, passed);
    if (!passed) {
        console.log(`  Expected: ${min} - ${max}`);
        console.log(`  Actual: ${actual}`);
    }
    return passed;
}

function getWidth(el) {
    // 尝试多种方式获取宽度
    if (el.getBoundingClientRect) {
        return el.getBoundingClientRect().width;
    }
    if (el.offsetWidth !== undefined) {
        return el.offsetWidth;
    }
    return 0;
}

console.log("[TEST_START] Display Contents Incremental Update");

// 创建 flex 容器
const container = document.createElement('div');
container.id = 'container';
Object.assign(container.style, {
    display: 'flex',
    flexDirection: 'column',
    alignItems: 'center',
    gap: '8px',
    padding: '20px',
    backgroundColor: '#f0f0f0'
});
document.body.appendChild(container);

// 创建 display: contents 包装器
const wrapper = document.createElement('div');
wrapper.id = 'wrapper';
wrapper.style.display = 'contents';
container.appendChild(wrapper);

// 添加第一个子元素（宽）
const item1 = document.createElement('div');
item1.id = 'item1';
item1.textContent = '这是一个很长很长很长的文本内容';
Object.assign(item1.style, {
    padding: '10px 16px',
    backgroundColor: '#ffffff',
    fontSize: '14px',
    color: '#1e293b'
});
wrapper.appendChild(item1);

// 强制布局
document.body.offsetHeight;

setTimeout(() => {
    // 记录第一个元素的宽度
    const item1Rect = item1.getBoundingClientRect();
    const item1Width = item1Rect ? item1Rect.width : 0;
    console.log(`[DEBUG] item1 rect: ${JSON.stringify(item1Rect)}`);
    console.log(`[DEBUG] item1 width after first render: ${item1Width}`);
    
    // 添加第二个子元素（窄）- 模拟增量更新
    const item2 = document.createElement('div');
    item2.id = 'item2';
    item2.textContent = '短文本';
    Object.assign(item2.style, {
        padding: '10px 16px',
        backgroundColor: '#ffffff',
        fontSize: '14px',
        color: '#1e293b'
    });
    wrapper.appendChild(item2);
    
    // 强制布局
    document.body.offsetHeight;
    
    setTimeout(() => {
        const item1RectAfter = item1.getBoundingClientRect();
        const item2Rect = item2.getBoundingClientRect();
        const item1WidthAfter = item1RectAfter ? item1RectAfter.width : 0;
        const item2Width = item2Rect ? item2Rect.width : 0;
        const item2FontSize = parseFloat(getComputedStyle(item2).fontSize);
        const item2BgColor = getComputedStyle(item2).backgroundColor;
        
        console.log(`[DEBUG] item1 rect after: ${JSON.stringify(item1RectAfter)}`);
        console.log(`[DEBUG] item2 rect: ${JSON.stringify(item2Rect)}`);
        console.log(`[DEBUG] item1 width after second render: ${item1WidthAfter}`);
        console.log(`[DEBUG] item2 width: ${item2Width}`);
        console.log(`[DEBUG] item2 fontSize: ${item2FontSize}`);
        console.log(`[DEBUG] item2 backgroundColor: ${item2BgColor}`);
        
        // 测试1: item1 宽度应该保持不变（允许小误差）
        const widthDiff = Math.abs(item1WidthAfter - item1Width);
        logTest("item1 width unchanged after adding item2", widthDiff < 2);
        
        // 测试2: item2 应该比 item1 窄（因为文本更短）
        logTest("item2 should be narrower than item1", item2Width < item1Width);
        
        // 测试3: item2 字体大小应该是 14px
        assertEqual(item2FontSize, 14, "item2 fontSize should be 14px");
        
        // 测试4: item2 背景色应该是白色
        const isWhiteBg = item2BgColor === 'rgb(255, 255, 255)' || 
                          item2BgColor === '#ffffff' ||
                          item2BgColor === 'white';
        logTest("item2 backgroundColor should be white", isWhiteBg);
        
        // 测试5: 两个元素都应该有合理的宽度（不是 0 或异常大）
        assertInRange(item1Width, 100, 500, "item1 width in reasonable range");
        assertInRange(item2Width, 50, 200, "item2 width in reasonable range");
        
        console.log("[TEST_END]");
    }, 300);
}, 300);
