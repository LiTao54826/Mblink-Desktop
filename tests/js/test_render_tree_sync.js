// 测试渲染树同步问题
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
    toasts = toasts.filter(t => t.id !== id);
    renderToasts();
}

// 检查 DOM 和渲染树
function checkState(stepName, expectedCount) {
    const container = getContainer();
    
    // DOM 检查
    const domElements = container.querySelectorAll('[data-toast-id]');
    const domCount = domElements.length;
    
    // 渲染树检查 - 通过 getBoundingClientRect 检查元素是否真正渲染
    let renderedCount = 0;
    domElements.forEach(el => {
        const rect = el.getBoundingClientRect();
        if (rect.width > 0 && rect.height > 0) {
            renderedCount++;
        }
    });
    
    // 检查容器的子元素数量（渲染树视角）
    // display: contents 的 div 不应该有盒子，所以直接子元素应该是 toast 元素
    const containerRect = container.getBoundingClientRect();
    
    console.log(`[DEBUG] ${stepName}:`);
    console.log(`[DEBUG]   DOM elements: ${domCount}`);
    console.log(`[DEBUG]   Rendered (has size): ${renderedCount}`);
    console.log(`[DEBUG]   Container rect: ${containerRect.width}x${containerRect.height}`);
    
    // 打印每个元素的信息
    domElements.forEach(el => {
        const rect = el.getBoundingClientRect();
        const id = el.getAttribute('data-toast-id');
        console.log(`[DEBUG]   Toast ${id}: ${rect.width}x${rect.height} at (${rect.left}, ${rect.top})`);
    });
    
    logTest(`${stepName}: DOM count=${expectedCount}`, domCount === expectedCount);
    logTest(`${stepName}: Rendered count=${expectedCount}`, renderedCount === expectedCount);
}

console.log('[TEST_START] Render Tree Sync Test');

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
