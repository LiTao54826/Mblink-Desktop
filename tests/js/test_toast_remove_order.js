/**
 * 测试 Toast 删除顺序问题
 * 
 * 问题描述：当多个 toast 被添加后，删除时会删除错误的 toast
 * - 添加 toast 1, 2, 3 后等待自动删除，toast 1 会残留
 * - 添加 toast 1, 2, 3, 4 后等待自动删除，toast 1 和 2 会残留
 */

import { h, render, Component } from 'preact';

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

// 简化的 Toast 实现，用于测试
let toastContainer = null;
let toasts = [];
let toastId = 0;

function getToastContainer() {
    if (!toastContainer) {
        toastContainer = document.createElement('div');
        toastContainer.id = 'toast-container';
        Object.assign(toastContainer.style, {
            position: 'fixed',
            top: '20px',
            left: '50%',
            display: 'flex',
            flexDirection: 'column',
            gap: '8px',
        });
        document.body.appendChild(toastContainer);
    }
    return toastContainer;
}

function ToastItem({ id, content }) {
    return h('div', { 
        'data-toast-id': id,
        style: {
            padding: '10px 16px',
            backgroundColor: '#333',
            color: '#fff',
            borderRadius: '4px',
        }
    }, content);
}

function renderToasts() {
    const container = getToastContainer();
    render(
        h('div', { style: { display: 'contents' } },
            toasts.map(toast => h(ToastItem, { key: toast.id, ...toast }))
        ),
        container
    );
}

function addToast(content) {
    const id = ++toastId;
    toasts.push({ id, content });
    renderToasts();
    console.log(`[DEBUG] Added toast ${id}: "${content}", total: ${toasts.length}`);
    return id;
}

function removeToast(id) {
    console.log(`[DEBUG] Removing toast ${id}, before: ${toasts.map(t => t.id).join(',')}`);
    toasts = toasts.filter(t => t.id !== id);
    renderToasts();
    console.log(`[DEBUG] After remove: ${toasts.map(t => t.id).join(',')}`);
}

function getVisibleToastIds() {
    const container = getToastContainer();
    const toastElements = container.querySelectorAll('[data-toast-id]');
    const ids = [];
    for (let i = 0; i < toastElements.length; i++) {
        ids.push(parseInt(toastElements[i].getAttribute('data-toast-id')));
    }
    return ids;
}

function clearAllToasts() {
    toasts = [];
    toastId = 0;
    renderToasts();
}

// 测试用例
console.log('[TEST_START] Toast Remove Order Tests');

// 测试 1：添加 3 个 toast，按顺序删除第一个
function test1() {
    console.log('\n--- Test 1: Add 3 toasts, remove first ---');
    clearAllToasts();
    
    const id1 = addToast('Toast 1');
    const id2 = addToast('Toast 2');
    const id3 = addToast('Toast 3');
    
    // 验证添加后的状态
    let visible = getVisibleToastIds();
    console.log(`[DEBUG] Visible after add: ${visible.join(',')}`);
    assertEqual(visible.length, 3, 'Test1: Should have 3 toasts after adding');
    
    // 删除第一个
    removeToast(id1);
    
    visible = getVisibleToastIds();
    console.log(`[DEBUG] Visible after remove id1: ${visible.join(',')}`);
    assertEqual(visible.length, 2, 'Test1: Should have 2 toasts after removing first');
    assertEqual(visible.includes(id1), false, 'Test1: Toast 1 should be removed');
    assertEqual(visible.includes(id2), true, 'Test1: Toast 2 should remain');
    assertEqual(visible.includes(id3), true, 'Test1: Toast 3 should remain');
}

// 测试 2：添加 3 个 toast，按顺序删除（模拟自动删除）
function test2() {
    console.log('\n--- Test 2: Add 3 toasts, remove in order (1, 2, 3) ---');
    clearAllToasts();
    
    const id1 = addToast('Toast 1');
    const id2 = addToast('Toast 2');
    const id3 = addToast('Toast 3');
    
    // 删除第一个
    removeToast(id1);
    let visible = getVisibleToastIds();
    console.log(`[DEBUG] After remove 1: ${visible.join(',')}`);
    assertEqual(visible.length, 2, 'Test2: Should have 2 toasts after removing 1');
    
    // 删除第二个
    removeToast(id2);
    visible = getVisibleToastIds();
    console.log(`[DEBUG] After remove 2: ${visible.join(',')}`);
    assertEqual(visible.length, 1, 'Test2: Should have 1 toast after removing 2');
    
    // 删除第三个
    removeToast(id3);
    visible = getVisibleToastIds();
    console.log(`[DEBUG] After remove 3: ${visible.join(',')}`);
    assertEqual(visible.length, 0, 'Test2: Should have 0 toasts after removing 3');
}

// 测试 3：添加 4 个 toast，按顺序删除
function test3() {
    console.log('\n--- Test 3: Add 4 toasts, remove in order ---');
    clearAllToasts();
    
    const id1 = addToast('Toast 1');
    const id2 = addToast('Toast 2');
    const id3 = addToast('Toast 3');
    const id4 = addToast('Toast 4');
    
    let visible = getVisibleToastIds();
    assertEqual(visible.length, 4, 'Test3: Should have 4 toasts after adding');
    
    // 按顺序删除
    removeToast(id1);
    visible = getVisibleToastIds();
    assertEqual(visible.length, 3, 'Test3: Should have 3 toasts after removing 1');
    assertEqual(visible.includes(id1), false, 'Test3: Toast 1 should be removed');
    
    removeToast(id2);
    visible = getVisibleToastIds();
    assertEqual(visible.length, 2, 'Test3: Should have 2 toasts after removing 2');
    assertEqual(visible.includes(id2), false, 'Test3: Toast 2 should be removed');
    
    removeToast(id3);
    visible = getVisibleToastIds();
    assertEqual(visible.length, 1, 'Test3: Should have 1 toast after removing 3');
    
    removeToast(id4);
    visible = getVisibleToastIds();
    assertEqual(visible.length, 0, 'Test3: Should have 0 toasts after removing 4');
}

// 测试 4：验证删除的是正确的 toast（通过内容验证）
function test4() {
    console.log('\n--- Test 4: Verify correct toast is removed by content ---');
    clearAllToasts();
    
    addToast('First');
    addToast('Second');
    addToast('Third');
    
    // 获取所有 toast 元素的内容
    const container = getToastContainer();
    let toastElements = container.querySelectorAll('[data-toast-id]');
    
    console.log(`[DEBUG] Initial contents:`);
    for (let i = 0; i < toastElements.length; i++) {
        console.log(`  [${i}] id=${toastElements[i].getAttribute('data-toast-id')}, text="${toastElements[i].textContent}"`);
    }
    
    // 删除第一个 toast (id=1)
    removeToast(1);
    
    toastElements = container.querySelectorAll('[data-toast-id]');
    console.log(`[DEBUG] After removing id=1:`);
    for (let i = 0; i < toastElements.length; i++) {
        console.log(`  [${i}] id=${toastElements[i].getAttribute('data-toast-id')}, text="${toastElements[i].textContent}"`);
    }
    
    // 验证剩余的 toast 内容
    assertEqual(toastElements.length, 2, 'Test4: Should have 2 toasts');
    if (toastElements.length >= 2) {
        // 第一个应该是 "Second"
        assertEqual(toastElements[0].textContent, 'Second', 'Test4: First remaining should be "Second"');
        // 第二个应该是 "Third"
        assertEqual(toastElements[1].textContent, 'Third', 'Test4: Second remaining should be "Third"');
    }
}

// 运行测试
test1();
test2();
test3();
test4();

console.log('\n[TEST_END]');
