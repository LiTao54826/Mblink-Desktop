// 详细测试渲染树同步问题
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
    render(
        h('div', { style: { display: 'contents' } },
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
    console.log(`[DEBUG] === Removing toast ${id} ===`);
    console.log(`[DEBUG] Before: toasts = [${toasts.map(t => t.id).join(', ')}]`);
    toasts = toasts.filter(t => t.id !== id);
    console.log(`[DEBUG] After: toasts = [${toasts.map(t => t.id).join(', ')}]`);
    renderToasts();
}

function dumpDOMTree(node, indent = '') {
    if (!node) return;
    
    const type = node.nodeType === 1 ? node.tagName : '#text';
    const id = node.getAttribute ? node.getAttribute('data-toast-id') : null;
    const display = node.style ? node.style.display : '';
    
    console.log(`[DEBUG] ${indent}${type}${id ? ` [toast-id=${id}]` : ''}${display ? ` display=${display}` : ''}`);
    
    if (node.childNodes) {
        for (let i = 0; i < node.childNodes.length; i++) {
            dumpDOMTree(node.childNodes[i], indent + '  ');
        }
    }
}

function checkState(stepName, expectedCount) {
    const container = getContainer();
    
    console.log(`[DEBUG] --- ${stepName} ---`);
    console.log(`[DEBUG] DOM Tree:`);
    dumpDOMTree(container, '  ');
    
    // DOM 检查
    const domElements = container.querySelectorAll('[data-toast-id]');
    const domCount = domElements.length;
    
    // 检查每个元素的位置
    console.log(`[DEBUG] Element positions:`);
    let prevBottom = 20; // container top
    let layoutCorrect = true;
    
    domElements.forEach((el, index) => {
        const rect = el.getBoundingClientRect();
        const id = el.getAttribute('data-toast-id');
        const expectedTop = index === 0 ? 20 : prevBottom + 8; // 8px gap
        const topDiff = Math.abs(rect.top - expectedTop);
        
        console.log(`[DEBUG]   Toast ${id}: top=${rect.top.toFixed(1)}, expected=${expectedTop.toFixed(1)}, diff=${topDiff.toFixed(1)}`);
        
        if (topDiff > 1) { // 允许 1px 误差
            layoutCorrect = false;
        }
        
        prevBottom = rect.bottom;
    });
    
    logTest(`${stepName}: DOM count=${expectedCount}`, domCount === expectedCount);
    logTest(`${stepName}: Layout correct`, layoutCorrect);
    
    return domCount === expectedCount && layoutCorrect;
}

console.log('[TEST_START] Render Tree Detail Test');

// 添加 6 个 toast
const ids = [];
for (let i = 1; i <= 6; i++) {
    ids.push(addToast(`Toast ${i}`));
}

setTimeout(() => {
    checkState('Initial (6 toasts)', 6);
    
    // 删除第 1 个
    removeToast(ids[0]);
    
    setTimeout(() => {
        checkState('After remove 1 (5 toasts)', 5);
        
        // 删除第 2 个
        removeToast(ids[1]);
        
        setTimeout(() => {
            checkState('After remove 2 (4 toasts)', 4);
            
            // 删除第 3 个
            removeToast(ids[2]);
            
            setTimeout(() => {
                checkState('After remove 3 (3 toasts)', 3);
                
                // 删除第 4 个
                removeToast(ids[3]);
                
                setTimeout(() => {
                    checkState('After remove 4 (2 toasts)', 2);
                    
                    // 删除第 5 个
                    removeToast(ids[4]);
                    
                    setTimeout(() => {
                        checkState('After remove 5 (1 toast)', 1);
                        
                        // 删除第 6 个
                        removeToast(ids[5]);
                        
                        setTimeout(() => {
                            checkState('After remove 6 (0 toasts)', 0);
                            console.log('[TEST_END]');
                        }, 100);
                    }, 100);
                }, 100);
            }, 100);
        }, 100);
    }, 100);
}, 200);
