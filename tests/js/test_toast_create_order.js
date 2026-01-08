/**
 * 测试 Toast 创建时的渲染顺序
 * 预期：先创建的 toast 应该在上面（y 值小），后创建的在下面（y 值大）
 */

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
            transform: 'translateX(-50%)',
            display: 'flex',
            flexDirection: 'column',
            alignItems: 'center',
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
            padding: '10px 16px', 
            backgroundColor: '#fff', 
            color: '#333',
            borderRadius: '8px',
            boxShadow: '0 2px 8px rgba(0,0,0,0.15)',
            display: 'flex',
            alignItems: 'center',
            gap: '8px',
        } 
    }, [
        h('span', { style: { color: '#22c55e' } }, '✓'),
        h('span', {}, content)
    ]);
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
    toasts.push({ id, content });
    renderToasts();
    return id;
}

console.log('[TEST_START] Toast Create Order Test');

// 检查渲染树顺序
function checkRenderTreeOrder() {
    const container = getContainer();
    console.log('Container style:', container.style.display, container.style.flexDirection);
    // 检查 display:contents 包装器的子元素
    const wrapper = container.firstChild;
    if (wrapper) {
        console.log('Wrapper style:', wrapper.style ? wrapper.style.display : 'no style');
        console.log('Wrapper children (DOM order):');
        Array.from(wrapper.childNodes).forEach((child, i) => {
            const id = child.getAttribute ? child.getAttribute('data-toast-id') : 'N/A';
            console.log(`  DOM[${i}]: data-toast-id=${id}`);
        });
    }
    
    // 直接检查 container 的子元素
    console.log('Container direct children:');
    Array.from(container.childNodes).forEach((child, i) => {
        const id = child.getAttribute ? child.getAttribute('data-toast-id') : 'N/A';
        const display = child.style ? child.style.display : 'no style';
        console.log(`  Container child[${i}]: data-toast-id=${id}, display=${display}`);
    });
}

// 依次添加 4 个 toast
addToast('Toast #1');

setTimeout(() => {
    addToast('Toast #2');
    
    setTimeout(() => {
        addToast('Toast #3');
        
        setTimeout(() => {
            addToast('Toast #4');
            
            // 等待渲染完成后检查顺序
            setTimeout(() => {
                checkRenderTreeOrder();
                
                const container = getContainer();
                const toastElements = container.querySelectorAll('[data-toast-id]');
                
                console.log(`Found ${toastElements.length} toasts`);
                
                // 收集每个 toast 的 y 位置
                const positions = [];
                toastElements.forEach(el => {
                    const id = el.getAttribute('data-toast-id');
                    const rect = el.getBoundingClientRect();
                    positions.push({ id, y: rect.y, content: el.textContent });
                    console.log(`Toast #${id}: y=${rect.y}, content="${el.textContent}"`);
                });
                
                // 按 y 位置排序
                positions.sort((a, b) => a.y - b.y);
                
                console.log('Order by y position (top to bottom):');
                positions.forEach((p, i) => {
                    console.log(`  ${i + 1}. Toast #${p.id} at y=${p.y}`);
                });
                
                // 检查顺序：Toast #1 应该在最上面（y 最小），Toast #4 在最下面（y 最大）
                const expectedOrder = ['1', '2', '3', '4'];
                const actualOrder = positions.map(p => p.id);
                
                const orderCorrect = expectedOrder.every((id, i) => actualOrder[i] === id);
                
                if (orderCorrect) {
                    logTest('Toast order is correct (1,2,3,4 from top to bottom)', true);
                } else {
                    logTest(`Toast order is wrong: expected [${expectedOrder.join(',')}], got [${actualOrder.join(',')}]`, false);
                }
                
                // 额外检查：确保 y 值是递增的
                let yIncreasing = true;
                for (let i = 1; i < positions.length; i++) {
                    if (positions[i].y <= positions[i-1].y) {
                        yIncreasing = false;
                        break;
                    }
                }
                logTest('Y positions are increasing', yIncreasing);
                
                console.log('[TEST_END]');
            }, 200);
        }, 100);
    }, 100);
}, 100);
