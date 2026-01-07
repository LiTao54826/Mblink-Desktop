// 测试不使用 display: contents 的情况
import { h, render } from 'preact';

function logTest(name, passed) {
    if (passed) {
        console.log(`[TEST_PASS] ${name}`);
    } else {
        console.log(`[TEST_FAIL] ${name}`);
    }
}

let container = null;
let toasts = [];
let toastId = 0;

function getContainer() {
    if (!container) {
        container = document.createElement('div');
        container.id = 'toast-container';
        Object.assign(container.style, {
            position: 'fixed',
            top: '20px',
            left: '50%',
            display: 'flex',
            flexDirection: 'column',
            gap: '8px',
        });
        document.body.appendChild(container);
    }
    return container;
}

function ToastItem({ id, content }) {
    return h('div', { 
        'data-toast-id': id,
        style: { 
            padding: '10px', 
            backgroundColor: '#333', 
            color: '#fff',
            borderRadius: '4px'
        } 
    }, content);
}

function renderToasts() {
    const container = getContainer();
    // 不使用 display: contents，直接渲染
    render(
        h('div', { style: { display: 'flex', flexDirection: 'column', gap: '8px' } },
            toasts.map(toast => h(ToastItem, { key: toast.id, ...toast }))
        ),
        container
    );
}

function addToast(content) {
    const id = ++toastId;
    toasts.push({ id, content: `${content} (id=${id})` });
    renderToasts();
    return id;
}

function removeToast(id) {
    toasts = toasts.filter(t => t.id !== id);
    renderToasts();
}

function checkState(stepName, expectedCount) {
    const container = getContainer();
    const domElements = container.querySelectorAll('[data-toast-id]');
    const domCount = domElements.length;
    
    let layoutCorrect = true;
    let prevBottom = 20;
    
    domElements.forEach((el, index) => {
        const rect = el.getBoundingClientRect();
        const expectedTop = index === 0 ? 20 : prevBottom + 8;
        const topDiff = Math.abs(rect.top - expectedTop);
        
        if (topDiff > 1) {
            layoutCorrect = false;
            console.log(`[DEBUG] Toast ${el.getAttribute('data-toast-id')}: top=${rect.top.toFixed(1)}, expected=${expectedTop.toFixed(1)}`);
        }
        
        prevBottom = rect.bottom;
    });
    
    logTest(`${stepName}: DOM count=${expectedCount}`, domCount === expectedCount);
    logTest(`${stepName}: Layout correct`, layoutCorrect);
}

console.log('[TEST_START] No Contents Test');

const ids = [];
for (let i = 1; i <= 6; i++) {
    ids.push(addToast(`Toast ${i}`));
}

setTimeout(() => {
    checkState('Initial', 6);
    
    removeToast(ids[0]);
    setTimeout(() => {
        checkState('After remove 1', 5);
        
        removeToast(ids[1]);
        setTimeout(() => {
            checkState('After remove 2', 4);
            
            removeToast(ids[2]);
            setTimeout(() => {
                checkState('After remove 3', 3);
                console.log('[TEST_END]');
            }, 100);
        }, 100);
    }, 100);
}, 200);
