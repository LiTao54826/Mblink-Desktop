/**
 * 逐步排查问题
 */

console.log('[TEST_START] Isolate Issue Test');

function checkCenter(name, container, textEl) {
    const contRect = container.getBoundingClientRect();
    const textRect = textEl.getBoundingClientRect();
    
    const actualOffset = textRect.top - contRect.top;
    const expectedOffset = (contRect.height - textRect.height) / 2;
    const diff = actualOffset - expectedOffset;
    
    console.log('[DEBUG] ' + name + ':');
    console.log('  Container: h=' + contRect.height + ', top=' + contRect.top.toFixed(1));
    console.log('  Text: h=' + textRect.height + ', top=' + textRect.top.toFixed(1));
    console.log('  Diff: ' + diff.toFixed(2));
    
    if (Math.abs(diff) < 1) {
        console.log('[TEST_PASS] ' + name);
    } else {
        console.log('[TEST_FAIL] ' + name + ' diff=' + diff.toFixed(2));
    }
}

document.body.style.padding = '20px';

// Test 1: 简单 flex + span (正常的基准)
const t1 = document.createElement('div');
t1.style.display = 'flex';
t1.style.alignItems = 'center';
t1.style.height = '40px';
t1.style.backgroundColor = '#22c55e';
const t1span = document.createElement('span');
t1span.style.fontSize = '16px';
t1span.style.lineHeight = 'normal';
t1span.textContent = 'Test1';
t1.appendChild(t1span);
document.body.appendChild(t1);

// Test 2: 加上 padding
const t2 = document.createElement('div');
t2.style.display = 'flex';
t2.style.alignItems = 'center';
t2.style.height = '40px';
t2.style.padding = '0 16px';
t2.style.backgroundColor = '#3b82f6';
t2.style.marginTop = '10px';
const t2span = document.createElement('span');
t2span.style.fontSize = '16px';
t2span.style.lineHeight = 'normal';
t2span.textContent = 'Test2';
t2.appendChild(t2span);
document.body.appendChild(t2);

// Test 3: inline-flex 而不是 flex
const t3 = document.createElement('div');
t3.style.display = 'inline-flex';
t3.style.alignItems = 'center';
t3.style.height = '40px';
t3.style.padding = '0 16px';
t3.style.backgroundColor = '#f59e0b';
t3.style.marginTop = '10px';
const t3span = document.createElement('span');
t3span.style.fontSize = '16px';
t3span.style.lineHeight = 'normal';
t3span.textContent = 'Test3';
t3.appendChild(t3span);
document.body.appendChild(document.createElement('br'));
document.body.appendChild(t3);

// Test 4: button 元素
const t4 = document.createElement('button');
t4.style.display = 'inline-flex';
t4.style.alignItems = 'center';
t4.style.height = '40px';
t4.style.padding = '0 16px';
t4.style.backgroundColor = '#ef4444';
t4.style.border = 'none';
t4.style.marginTop = '10px';
const t4span = document.createElement('span');
t4span.style.fontSize = '16px';
t4span.style.lineHeight = 'normal';
t4span.textContent = 'Test4';
t4.appendChild(t4span);
document.body.appendChild(document.createElement('br'));
document.body.appendChild(t4);

// Test 5: button 在 flex 容器中
const t5row = document.createElement('div');
t5row.style.display = 'flex';
t5row.style.alignItems = 'center';
t5row.style.marginTop = '10px';
t5row.style.backgroundColor = '#e0e0e0';
t5row.style.padding = '10px';

const t5 = document.createElement('button');
t5.style.display = 'inline-flex';
t5.style.alignItems = 'center';
t5.style.height = '40px';
t5.style.padding = '0 16px';
t5.style.backgroundColor = '#8b5cf6';
t5.style.border = 'none';
const t5span = document.createElement('span');
t5span.style.fontSize = '16px';
t5span.style.lineHeight = 'normal';
t5span.textContent = 'Test5';
t5.appendChild(t5span);
t5row.appendChild(t5);
document.body.appendChild(t5row);

// Test 6: 完整 fluent 样式
const t6row = document.createElement('div');
t6row.style.display = 'flex';
t6row.style.flexWrap = 'wrap';
t6row.style.gap = '12px';
t6row.style.alignItems = 'center';
t6row.style.marginTop = '10px';
t6row.style.backgroundColor = '#e0e0e0';
t6row.style.padding = '10px';

const t6 = document.createElement('button');
t6.type = 'button';
t6.style.display = 'inline-flex';
t6.style.alignItems = 'center';
t6.style.justifyContent = 'center';
t6.style.gap = '8px';
t6.style.height = '40px';
t6.style.minWidth = '96px';
t6.style.padding = '8px 16px';
t6.style.fontSize = '16px';
t6.style.fontWeight = '600';
t6.style.lineHeight = 'normal';
t6.style.backgroundColor = '#0f6cbd';
t6.style.color = 'white';
t6.style.border = 'none';
t6.style.borderRadius = '4px';
t6.style.boxSizing = 'border-box';
const t6span = document.createElement('span');
t6span.textContent = 'Test6';
t6.appendChild(t6span);
t6row.appendChild(t6);
document.body.appendChild(t6row);

setTimeout(() => {
    checkCenter('Test1: simple flex+span', t1, t1span);
    checkCenter('Test2: flex+padding+span', t2, t2span);
    checkCenter('Test3: inline-flex+span', t3, t3span);
    checkCenter('Test4: button inline-flex', t4, t4span);
    checkCenter('Test5: button in flex row', t5, t5span);
    checkCenter('Test6: full fluent style', t6, t6span);
    console.log('[TEST_END]');
}, 200);
