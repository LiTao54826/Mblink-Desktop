/**
 * 用原生 DOM 完全模拟 fluent Button 的结构和样式
 * 对比 preact 渲染的结果
 */

console.log('[TEST_START] Fluent Button Native Test');

// 完全复制 fluent/theme.js 中的值
const spacing = {
    m: '12px',
};

const componentSizes = {
    small: {
        height: '24px',
        minWidth: '64px',
        fontSize: '12px',
        padding: '4px 8px',
        iconSize: '16px',
        gap: '4px',
    },
    medium: {
        height: '32px',
        minWidth: '96px',
        fontSize: '14px',
        padding: '6px 12px',
        iconSize: '20px',
        gap: '6px',
    },
    large: {
        height: '40px',
        minWidth: '96px',
        fontSize: '16px',
        padding: '8px 16px',
        iconSize: '24px',
        gap: '8px',
    },
};

// 创建 DemoRow
function createDemoRow() {
    const row = document.createElement('div');
    row.style.display = 'flex';
    row.style.flexWrap = 'wrap';
    row.style.gap = spacing.m;
    row.style.alignItems = 'center';
    row.style.marginBottom = spacing.m;
    row.style.backgroundColor = '#e0e0e0';
    row.style.padding = '10px';
    return row;
}

// 完全模拟 fluent Button 的样式
function createFluentButton(text, size) {
    const sizeConfig = componentSizes[size] || componentSizes.medium;
    
    const btn = document.createElement('button');
    btn.type = 'button';
    
    // 完全复制 fluent/button.js 中的 baseStyle
    btn.style.display = 'inline-flex';
    btn.style.alignItems = 'center';
    btn.style.justifyContent = 'center';
    btn.style.gap = sizeConfig.gap;
    btn.style.height = sizeConfig.height;
    btn.style.minWidth = sizeConfig.minWidth;
    btn.style.padding = sizeConfig.padding;
    btn.style.fontSize = sizeConfig.fontSize;
    btn.style.fontFamily = "'Segoe UI', 'Segoe UI Web (West European)', -apple-system, BlinkMacSystemFont, Roboto, 'Helvetica Neue', sans-serif";
    btn.style.fontWeight = '600';
    btn.style.lineHeight = 'normal';
    btn.style.cursor = 'pointer';
    btn.style.transition = 'all 150ms cubic-bezier(0.33, 0, 0.67, 1)';
    btn.style.outline = 'none';
    btn.style.textDecoration = 'none';
    btn.style.whiteSpace = 'nowrap';
    btn.style.userSelect = 'none';
    btn.style.boxSizing = 'border-box';
    btn.style.verticalAlign = 'middle';
    btn.style.borderRadius = '4px';
    btn.style.backgroundColor = '#0f6cbd';
    btn.style.color = 'white';
    btn.style.border = 'none';
    
    // 关键：fluent Button 把文字包在 span 中
    const span = document.createElement('span');
    span.textContent = text;
    btn.appendChild(span);
    
    return btn;
}

// 创建测试结构
const container = document.createElement('div');
container.style.padding = '24px';
container.setAttribute('data-preact-id', '1');

const row = createDemoRow();
row.setAttribute('data-preact-id', '2');
const btnSmall = createFluentButton('Small', 'small');
btnSmall.setAttribute('data-preact-id', '3');
const btnMedium = createFluentButton('Medium', 'medium');
btnMedium.setAttribute('data-preact-id', '4');
const btnLarge = createFluentButton('Large', 'large');
btnLarge.setAttribute('data-preact-id', '5');

row.appendChild(btnSmall);
row.appendChild(btnMedium);
row.appendChild(btnLarge);
container.appendChild(row);
document.body.appendChild(container);

// 检测
setTimeout(() => {
    const buttons = [btnSmall, btnMedium, btnLarge];
    const names = ['Small', 'Medium', 'Large'];
    
    buttons.forEach((btn, i) => {
        const span = btn.querySelector('span');
        if (!span) {
            console.log('[DEBUG] Button ' + i + ' has no span');
            return;
        }
        
        const btnRect = btn.getBoundingClientRect();
        const spanRect = span.getBoundingClientRect();
        
        const actualOffset = spanRect.top - btnRect.top;
        const expectedOffset = (btnRect.height - spanRect.height) / 2;
        const diff = actualOffset - expectedOffset;
        
        // 计算视觉中心偏移
        const btnCenter = btnRect.top + btnRect.height / 2;
        const spanCenter = spanRect.top + spanRect.height / 2;
        const centerDiff = spanCenter - btnCenter;
        
        console.log('[DEBUG] ' + names[i] + ' Button (native):');
        console.log('  Button: height=' + btnRect.height + ', top=' + btnRect.top);
        console.log('  Span: height=' + spanRect.height + ', top=' + spanRect.top);
        console.log('  Actual offset: ' + actualOffset);
        console.log('  Expected offset: ' + expectedOffset);
        console.log('  Diff: ' + diff.toFixed(2));
        console.log('  Center diff: ' + centerDiff.toFixed(2));
        
        if (Math.abs(diff) < 1) {
            console.log('[TEST_PASS] ' + names[i] + ' Button centered');
        } else {
            console.log('[TEST_FAIL] ' + names[i] + ' Button NOT centered, diff=' + diff.toFixed(2));
        }
    });
    
    console.log('[TEST_END]');
}, 200);
