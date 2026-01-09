/**
 * 测试嵌套 flex 布局中按钮文字垂直居中问题
 * 模拟 fluent_demo 的实际结构：
 * - 外层 flex 容器 (DemoRow) 带 alignItems: center
 * - 内层 inline-flex 按钮
 */

console.log('[TEST_START] Nested Flex Center Test');

// 模拟 DemoRow 结构
function createDemoRow() {
    const row = document.createElement('div');
    row.style.display = 'flex';
    row.style.flexWrap = 'wrap';
    row.style.gap = '12px';
    row.style.alignItems = 'center';
    row.style.marginBottom = '12px';
    row.style.backgroundColor = '#f0f0f0';
    row.style.padding = '10px';
    return row;
}

// 模拟 Fluent Button 结构
function createFluentButton(text, size) {
    const btn = document.createElement('button');
    btn.style.display = 'inline-flex';
    btn.style.alignItems = 'center';
    btn.style.justifyContent = 'center';
    btn.style.height = size === 'large' ? '40px' : '32px';
    btn.style.padding = size === 'large' ? '8px 16px' : '6px 12px';
    btn.style.fontSize = size === 'large' ? '16px' : '14px';
    btn.style.lineHeight = 'normal';
    btn.style.backgroundColor = '#0f6cbd';
    btn.style.color = 'white';
    btn.style.border = 'none';
    btn.style.borderRadius = '4px';
    
    // 关键：文字包裹在 span 中
    const span = document.createElement('span');
    span.textContent = text;
    btn.appendChild(span);
    
    return btn;
}

// 创建测试结构
const container = document.createElement('div');
container.style.padding = '24px';
container.style.backgroundColor = '#f5f5f5';

// 测试1: 单个按钮在 flex row 中
const row1 = createDemoRow();
const btn1 = createFluentButton('Large', 'large');
row1.appendChild(btn1);
container.appendChild(row1);

// 测试2: 多个不同尺寸按钮在同一 flex row 中
const row2 = createDemoRow();
const btnSmall = createFluentButton('Small', 'small');
const btnMedium = createFluentButton('Medium', 'medium');
const btnLarge = createFluentButton('Large', 'large');
row2.appendChild(btnSmall);
row2.appendChild(btnMedium);
row2.appendChild(btnLarge);
container.appendChild(row2);

// 测试3: 对照组 - 不使用 span 包裹
const row3 = createDemoRow();
const btnDirect = document.createElement('button');
btnDirect.style.display = 'inline-flex';
btnDirect.style.alignItems = 'center';
btnDirect.style.justifyContent = 'center';
btnDirect.style.height = '40px';
btnDirect.style.padding = '8px 16px';
btnDirect.style.fontSize = '16px';
btnDirect.style.lineHeight = '1';
btnDirect.style.backgroundColor = '#22c55e';
btnDirect.style.color = 'white';
btnDirect.style.border = 'none';
btnDirect.style.borderRadius = '4px';
btnDirect.textContent = 'Direct';
row3.appendChild(btnDirect);
container.appendChild(row3);

document.body.appendChild(container);

// 检测垂直居中
setTimeout(() => {
    function checkVerticalCenter(btn, name) {
        const span = btn.querySelector('span');
        const textElement = span || btn;
        
        const btnRect = btn.getBoundingClientRect();
        const textRect = textElement.getBoundingClientRect();
        
        const actualOffset = textRect.top - btnRect.top;
        const expectedOffset = (btnRect.height - textRect.height) / 2;
        const diff = actualOffset - expectedOffset;
        
        console.log('[DEBUG] ' + name + ':');
        console.log('  Button: height=' + btnRect.height + ', top=' + btnRect.top);
        console.log('  Text: height=' + textRect.height + ', top=' + textRect.top);
        console.log('  Actual offset: ' + actualOffset);
        console.log('  Expected offset: ' + expectedOffset);
        console.log('  Diff: ' + diff);
        
        if (Math.abs(diff) < 1) {
            console.log('[TEST_PASS] ' + name + ' is vertically centered');
        } else {
            console.log('[TEST_FAIL] ' + name + ' is NOT centered, diff=' + diff.toFixed(2) + 'px');
        }
    }
    
    checkVerticalCenter(btn1, 'Single Large Button');
    checkVerticalCenter(btnSmall, 'Small Button in Row');
    checkVerticalCenter(btnMedium, 'Medium Button in Row');
    checkVerticalCenter(btnLarge, 'Large Button in Row');
    checkVerticalCenter(btnDirect, 'Direct Text Button');
    
    console.log('[TEST_END]');
}, 100);
