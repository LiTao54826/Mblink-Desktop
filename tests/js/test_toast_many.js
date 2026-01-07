// 测试多个 Toast 删除问题
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

function checkToasts(expectedIds, stepName) {
    const container = getContainer();
    const rendered = container.querySelectorAll('[data-toast-id]');
    const actualIds = Array.from(rendered).map(el => parseInt(el.getAttribute('data-toast-id')));
    
    console.log(`[DEBUG] ${stepName}: expected [${expectedIds.join(',')}], actual [${actualIds.join(',')}]`);
    
    // 检查数量
    const countOk = rendered.length === expectedIds.length;
    logTest(`${stepName}: count=${expectedIds.length}`, countOk);
    
    // 检查每个 ID 是否存在
    for (const id of expectedIds) {
        const found = actualIds.includes(id);
        logTest(`${stepName}: has id=${id}`, found);
    }
    
    return countOk && expectedIds.every(id => actualIds.includes(id));
}

console.log('[TEST_START] Toast Many Test');

// 添加 6 个 toast
const ids = [];
for (let i = 1; i <= 6; i++) {
    ids.push(addToast(`Toast ${i}`));
}

console.log(`[DEBUG] Added 6 toasts: ${ids.join(', ')}`);

// 等待渲染完成
setTimeout(() => {
    checkToasts(ids, 'Initial');
    
    // 删除第 1 个
    console.log('[DEBUG] === Removing toast 1 ===');
    removeToast(ids[0]);
    
    setTimeout(() => {
        checkToasts([ids[1], ids[2], ids[3], ids[4], ids[5]], 'After remove 1');
        
        // 删除第 3 个
        console.log('[DEBUG] === Removing toast 3 ===');
        removeToast(ids[2]);
        
        setTimeout(() => {
            checkToasts([ids[1], ids[3], ids[4], ids[5]], 'After remove 3');
            
            // 删除第 5 个
            console.log('[DEBUG] === Removing toast 5 ===');
            removeToast(ids[4]);
            
            setTimeout(() => {
                checkToasts([ids[1], ids[3], ids[5]], 'After remove 5');
                
                // 删除第 2 个
                console.log('[DEBUG] === Removing toast 2 ===');
                removeToast(ids[1]);
                
                setTimeout(() => {
                    checkToasts([ids[3], ids[5]], 'After remove 2');
                    
                    // 删除第 4 个
                    console.log('[DEBUG] === Removing toast 4 ===');
                    removeToast(ids[3]);
                    
                    setTimeout(() => {
                        checkToasts([ids[5]], 'After remove 4');
                        
                        // 删除最后一个
                        console.log('[DEBUG] === Removing toast 6 ===');
                        removeToast(ids[5]);
                        
                        setTimeout(() => {
                            checkToasts([], 'After remove 6');
                            console.log('[TEST_END]');
                        }, 50);
                    }, 50);
                }, 50);
            }, 50);
        }, 50);
    }, 50);
}, 100);
