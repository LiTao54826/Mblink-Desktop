/**
 * 对比使用 key 和不使用 key 的行为
 */

import { h, render } from 'preact';

document.body.style.margin = '0';
document.body.style.padding = '20px';

console.log('[TEST_START] Preact Key Compare');

function logTest(name, passed) {
    if (passed) {
        console.log(`[TEST_PASS] ${name}`);
    } else {
        console.log(`[TEST_FAIL] ${name}`);
    }
}

function checkElements(container, label) {
    console.log(`\n[DEBUG] ${label}`);
    const allItems = container.querySelectorAll('[data-id]');
    console.log(`[DEBUG] Found ${allItems.length} items`);
    
    let allValid = true;
    for (let i = 0; i < allItems.length; i++) {
        const elem = allItems[i];
        const id = elem.getAttribute('data-id');
        const rect = elem.getBoundingClientRect();
        console.log(`[DEBUG] Item ${id}: bbox h=${Math.round(rect.height)}`);
        if (rect.height <= 0) allValid = false;
    }
    return allValid;
}

// 测试 1：不使用 key
const container1 = document.createElement('div');
container1.id = 'container1';
container1.style.display = 'flex';
container1.style.flexDirection = 'column';
container1.style.gap = '10px';
container1.style.marginBottom = '20px';
document.body.appendChild(container1);

// 测试 2：使用 key
const container2 = document.createElement('div');
container2.id = 'container2';
container2.style.display = 'flex';
container2.style.flexDirection = 'column';
container2.style.gap = '10px';
document.body.appendChild(container2);

setTimeout(() => {
    // 初始渲染
    console.log('\n=== Initial render ===');
    
    // 不使用 key
    render(
        h('div', { id: 'wrapper1' },
            h('div', { 'data-id': 'a', style: { padding: '10px', backgroundColor: '#faa' } }, 'A'),
            h('div', { 'data-id': 'b', style: { padding: '10px', backgroundColor: '#afa' } }, 'B'),
            h('div', { 'data-id': 'c', style: { padding: '10px', backgroundColor: '#aaf' } }, 'C')
        ),
        container1
    );
    
    // 使用 key
    render(
        h('div', { id: 'wrapper2' },
            h('div', { key: 'a', 'data-id': 'a', style: { padding: '10px', backgroundColor: '#faa' } }, 'A'),
            h('div', { key: 'b', 'data-id': 'b', style: { padding: '10px', backgroundColor: '#afa' } }, 'B'),
            h('div', { key: 'c', 'data-id': 'c', style: { padding: '10px', backgroundColor: '#aaf' } }, 'C')
        ),
        container2
    );
    
    setTimeout(() => {
        checkElements(container1, 'Container1 (no keys) after initial');
        checkElements(container2, 'Container2 (with keys) after initial');
        
        // 移除中间元素
        console.log('\n=== Remove middle element ===');
        
        // 不使用 key
        render(
            h('div', { id: 'wrapper1' },
                h('div', { 'data-id': 'a', style: { padding: '10px', backgroundColor: '#faa' } }, 'A'),
                h('div', { 'data-id': 'c', style: { padding: '10px', backgroundColor: '#aaf' } }, 'C')
            ),
            container1
        );
        
        // 使用 key
        render(
            h('div', { id: 'wrapper2' },
                h('div', { key: 'a', 'data-id': 'a', style: { padding: '10px', backgroundColor: '#faa' } }, 'A'),
                h('div', { key: 'c', 'data-id': 'c', style: { padding: '10px', backgroundColor: '#aaf' } }, 'C')
            ),
            container2
        );
        
        setTimeout(() => {
            logTest('No keys: remaining items valid', checkElements(container1, 'Container1 (no keys) after remove'));
            logTest('With keys: remaining items valid', checkElements(container2, 'Container2 (with keys) after remove'));
            
            console.log('\n[TEST_END]');
        }, 300);
    }, 300);
}, 100);
