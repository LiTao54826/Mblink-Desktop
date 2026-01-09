/**
 * 测试按钮文字垂直居中问题
 * 对比 fluent 和 component 两种按钮的布局
 */

// 创建 fluent 风格按钮 (inline-flex + span 包裹文字)
function createFluentButton() {
    const btn = document.createElement('button');
    btn.style.display = 'inline-flex';
    btn.style.alignItems = 'center';
    btn.style.justifyContent = 'center';
    btn.style.height = '40px';
    btn.style.padding = '8px 16px';
    btn.style.fontSize = '16px';
    btn.style.lineHeight = 'normal';
    btn.style.backgroundColor = '#0f6cbd';
    btn.style.color = 'white';
    btn.style.border = 'none';
    btn.style.borderRadius = '6px';
    
    const span = document.createElement('span');
    span.textContent = 'Large';
    btn.appendChild(span);
    
    return btn;
}

// 创建 component 风格按钮 (inline-flex + 直接文字)
function createComponentButton() {
    const btn = document.createElement('button');
    btn.style.display = 'inline-flex';
    btn.style.alignItems = 'center';
    btn.style.justifyContent = 'center';
    btn.style.height = '44px';
    btn.style.padding = '12px 20px';
    btn.style.fontSize = '16px';
    btn.style.lineHeight = '1';
    btn.style.backgroundColor = '#3b82f6';
    btn.style.color = 'white';
    btn.style.border = 'none';
    btn.style.borderRadius = '6px';
    btn.textContent = 'Large';
    
    return btn;
}

// 创建简化测试：只测试 inline-flex 容器中 span 的垂直居中
function createSimpleTest() {
    const container = document.createElement('div');
    container.style.display = 'inline-flex';
    container.style.alignItems = 'center';
    container.style.justifyContent = 'center';
    container.style.height = '40px';
    container.style.width = '100px';
    container.style.backgroundColor = '#0f6cbd';
    
    const span = document.createElement('span');
    span.style.fontSize = '16px';
    span.style.lineHeight = 'normal';
    span.style.color = 'white';
    span.textContent = 'Test';
    container.appendChild(span);
    
    return container;
}

// 创建对照测试：line-height: 1
function createSimpleTestLineHeight1() {
    const container = document.createElement('div');
    container.style.display = 'inline-flex';
    container.style.alignItems = 'center';
    container.style.justifyContent = 'center';
    container.style.height = '40px';
    container.style.width = '100px';
    container.style.backgroundColor = '#22c55e';
    
    const span = document.createElement('span');
    span.style.fontSize = '16px';
    span.style.lineHeight = '1';
    span.style.color = 'white';
    span.textContent = 'Test';
    container.appendChild(span);
    
    return container;
}

// 测试
console.log('[TEST_START] Button Vertical Center Test');

const fluentBtn = createFluentButton();
const componentBtn = createComponentButton();
const simpleTest = createSimpleTest();
const simpleTestLH1 = createSimpleTestLineHeight1();

document.body.appendChild(fluentBtn);
document.body.appendChild(document.createElement('br'));
document.body.appendChild(componentBtn);
document.body.appendChild(document.createElement('br'));
document.body.appendChild(simpleTest);
document.body.appendChild(document.createElement('br'));
document.body.appendChild(simpleTestLH1);

// 等待布局完成后输出信息
setTimeout(() => {
    // 获取 fluent 按钮内 span 的位置
    const fluentSpan = fluentBtn.querySelector('span');
    if (fluentSpan) {
        const btnRect = fluentBtn.getBoundingClientRect();
        const spanRect = fluentSpan.getBoundingClientRect();
        
        console.log('[DEBUG] Fluent Button:');
        console.log('  Button height: ' + btnRect.height);
        console.log('  Button top: ' + btnRect.top);
        console.log('  Span height: ' + spanRect.height);
        console.log('  Span top: ' + spanRect.top);
        console.log('  Span offset from button top: ' + (spanRect.top - btnRect.top));
        console.log('  Expected center offset: ' + ((btnRect.height - spanRect.height) / 2));
        
        const actualOffset = spanRect.top - btnRect.top;
        const expectedOffset = (btnRect.height - spanRect.height) / 2;
        const diff = Math.abs(actualOffset - expectedOffset);
        
        if (diff < 1) {
            console.log('[TEST_PASS] Fluent button span is vertically centered');
        } else {
            console.log('[TEST_FAIL] Fluent button span is NOT vertically centered, diff=' + diff);
        }
    }
    
    // 获取 simple test 的位置
    const simpleSpan = simpleTest.querySelector('span');
    if (simpleSpan) {
        const containerRect = simpleTest.getBoundingClientRect();
        const spanRect = simpleSpan.getBoundingClientRect();
        
        console.log('[DEBUG] Simple Test (line-height: normal):');
        console.log('  Container height: ' + containerRect.height);
        console.log('  Container top: ' + containerRect.top);
        console.log('  Span height: ' + spanRect.height);
        console.log('  Span top: ' + spanRect.top);
        console.log('  Span offset from container top: ' + (spanRect.top - containerRect.top));
        console.log('  Expected center offset: ' + ((containerRect.height - spanRect.height) / 2));
        
        const actualOffset = spanRect.top - containerRect.top;
        const expectedOffset = (containerRect.height - spanRect.height) / 2;
        const diff = Math.abs(actualOffset - expectedOffset);
        
        if (diff < 1) {
            console.log('[TEST_PASS] Simple test span is vertically centered');
        } else {
            console.log('[TEST_FAIL] Simple test span is NOT vertically centered, diff=' + diff);
        }
    }
    
    // 获取 simple test line-height:1 的位置
    const simpleLH1Span = simpleTestLH1.querySelector('span');
    if (simpleLH1Span) {
        const containerRect = simpleTestLH1.getBoundingClientRect();
        const spanRect = simpleLH1Span.getBoundingClientRect();
        
        console.log('[DEBUG] Simple Test (line-height: 1):');
        console.log('  Container height: ' + containerRect.height);
        console.log('  Container top: ' + containerRect.top);
        console.log('  Span height: ' + spanRect.height);
        console.log('  Span top: ' + spanRect.top);
        console.log('  Span offset from container top: ' + (spanRect.top - containerRect.top));
        console.log('  Expected center offset: ' + ((containerRect.height - spanRect.height) / 2));
        
        const actualOffset = spanRect.top - containerRect.top;
        const expectedOffset = (containerRect.height - spanRect.height) / 2;
        const diff = Math.abs(actualOffset - expectedOffset);
        
        if (diff < 1) {
            console.log('[TEST_PASS] Simple test (LH1) span is vertically centered');
        } else {
            console.log('[TEST_FAIL] Simple test (LH1) span is NOT vertically centered, diff=' + diff);
        }
    }
    
    console.log('[TEST_END]');
}, 100);
