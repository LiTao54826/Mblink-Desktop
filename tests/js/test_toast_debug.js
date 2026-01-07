// 测试 Toast 删除问题的调试脚本
import { h, render } from 'preact';

function logTest(name, passed) {
    if (passed) {
        console.log(`[TEST_PASS] ${name}`);
    } else {
        console.log(`[TEST_FAIL] ${name}`);
    }
}

// 模拟 Toast 的结构
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
    console.log(`[DEBUG] Rendering ${toasts.length} toasts: ${toasts.map(t => t.id).join(', ')}`);
    
    render(
        h('div', { style: { display: 'contents' } },
            toasts.map(toast => h(ToastItem, { key: toast.id, ...toast }))
        ),
        container
    );
    
    // 检查渲染结果
    setTimeout(() => {
        const rendered = container.querySelectorAll('[data-toast-id]');
        console.log(`[DEBUG] Rendered ${rendered.length} toast elements`);
        rendered.forEach(el => {
            console.log(`[DEBUG]   - Toast ${el.getAttribute('data-toast-id')}: "${el.textContent}"`);
        });
    }, 50);
}

function addToast(content) {
    const id = ++toastId;
    toasts.push({ id, content: `${content} (id=${id})` });
    renderToasts();
    return id;
}

function removeToast(id) {
    console.log(`[DEBUG] Removing toast ${id}`);
    console.log(`[DEBUG] Before: ${toasts.map(t => t.id).join(', ')}`);
    toasts = toasts.filter(t => t.id !== id);
    console.log(`[DEBUG] After: ${toasts.map(t => t.id).join(', ')}`);
    renderToasts();
}

// 测试场景
console.log('[TEST_START] Toast Debug Test');

// 添加 3 个 toast
const id1 = addToast('First');
const id2 = addToast('Second');
const id3 = addToast('Third');

console.log(`[DEBUG] Added toasts: ${id1}, ${id2}, ${id3}`);

// 等待渲染完成后连续删除
setTimeout(() => {
    console.log('[DEBUG] === Step 1: Removing first toast ===');
    removeToast(id1);
    
    setTimeout(() => {
        const container = getContainer();
        let rendered = container.querySelectorAll('[data-toast-id]');
        let ids = Array.from(rendered).map(el => el.getAttribute('data-toast-id'));
        console.log(`[DEBUG] After removing 1: ${ids.join(', ')}`);
        
        logTest('Step1: Toast 1 removed', !ids.includes(String(id1)));
        logTest('Step1: Toast 2 remains', ids.includes(String(id2)));
        logTest('Step1: Toast 3 remains', ids.includes(String(id3)));
        logTest('Step1: Count is 2', rendered.length === 2);
        
        // 删除第二个
        console.log('[DEBUG] === Step 2: Removing second toast ===');
        removeToast(id2);
        
        setTimeout(() => {
            rendered = container.querySelectorAll('[data-toast-id]');
            ids = Array.from(rendered).map(el => el.getAttribute('data-toast-id'));
            console.log(`[DEBUG] After removing 2: ${ids.join(', ')}`);
            
            logTest('Step2: Toast 2 removed', !ids.includes(String(id2)));
            logTest('Step2: Toast 3 remains', ids.includes(String(id3)));
            logTest('Step2: Count is 1', rendered.length === 1);
            
            // 删除第三个
            console.log('[DEBUG] === Step 3: Removing third toast ===');
            removeToast(id3);
            
            setTimeout(() => {
                rendered = container.querySelectorAll('[data-toast-id]');
                ids = Array.from(rendered).map(el => el.getAttribute('data-toast-id'));
                console.log(`[DEBUG] After removing 3: ${ids.join(', ')}`);
                
                logTest('Step3: Toast 3 removed', !ids.includes(String(id3)));
                logTest('Step3: Count is 0', rendered.length === 0);
                
                console.log('[TEST_END]');
            }, 100);
        }, 100);
    }, 100);
}, 200);
