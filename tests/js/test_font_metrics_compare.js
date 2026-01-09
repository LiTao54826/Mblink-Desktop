/**
 * 对比不同字体的度量值差异
 * 验证 Segoe UI 字体导致的垂直居中问题
 */

console.log('[TEST_START] Font Metrics Compare');

// 创建测试容器
function createTestButton(fontFamily, label) {
    const btn = document.createElement('button');
    btn.style.display = 'inline-flex';
    btn.style.alignItems = 'center';
    btn.style.justifyContent = 'center';
    btn.style.height = '40px';
    btn.style.padding = '0 16px';
    btn.style.fontSize = '16px';
    btn.style.fontWeight = '600';
    btn.style.lineHeight = 'normal';
    btn.style.backgroundColor = '#0f6cbd';
    btn.style.color = 'white';
    btn.style.border = 'none';
    btn.style.borderRadius = '4px';
    btn.style.marginRight = '10px';
    
    if (fontFamily) {
        btn.style.fontFamily = fontFamily;
    }
    
    const span = document.createElement('span');
    span.textContent = label;
    btn.appendChild(span);
    
    return btn;
}

const container = document.createElement('div');
container.style.padding = '50px';
container.style.backgroundColor = '#f0f0f0';

// 测试1: 默认字体
const btn1 = createTestButton(null, 'Default Font');
container.appendChild(btn1);

// 测试2: Segoe UI 字体
const btn2 = createTestButton("'Segoe UI', sans-serif", 'Segoe UI');
container.appendChild(btn2);

// 测试3: Arial 字体
const btn3 = createTestButton("Arial, sans-serif", 'Arial');
container.appendChild(btn3);

document.body.appendChild(container);

setTimeout(() => {
    const buttons = [
        { el: btn1, name: 'Default Font' },
        { el: btn2, name: 'Segoe UI' },
        { el: btn3, name: 'Arial' }
    ];
    
    buttons.forEach(({ el, name }) => {
        const span = el.querySelector('span');
        const btnRect = el.getBoundingClientRect();
        const spanRect = span.getBoundingClientRect();
        
        const btnCenter = btnRect.top + btnRect.height / 2;
        const spanCenter = spanRect.top + spanRect.height / 2;
        const centerDiff = spanCenter - btnCenter;
        
        console.log('[DEBUG] ' + name + ':');
        console.log('  Button height: ' + btnRect.height);
        console.log('  Span height: ' + spanRect.height);
        console.log('  Center diff: ' + centerDiff.toFixed(2) + ' (positive=below center)');
        
        if (Math.abs(centerDiff) < 1.5) {
            console.log('[TEST_PASS] ' + name + ' centered');
        } else {
            console.log('[TEST_FAIL] ' + name + ' NOT centered, diff=' + centerDiff.toFixed(2));
        }
    });
    
    console.log('[TEST_END]');
}, 200);
