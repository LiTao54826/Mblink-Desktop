// 调试渲染树重复问题
import { h, render } from 'preact';

console.log("[TEST_START] Debug Render Tree Duplication");

const container = document.createElement('div');
container.id = 'container';
Object.assign(container.style, {
    display: 'flex',
    flexDirection: 'column',
    alignItems: 'center',
    gap: '8px',
    padding: '20px'
});
document.body.appendChild(container);

let renderCount = 0;

function ToastItem({ content }) {
    return h('div', {
        className: 'toast',
        style: {
            padding: '10px 16px',
            backgroundColor: '#ffffff',
            fontSize: '14px'
        }
    }, content);
}

function renderToasts(toasts) {
    renderCount++;
    console.log(`[DEBUG] Render #${renderCount}: ${toasts.length} toasts`);
    
    render(
        h('div', { style: { display: 'contents' } },
            toasts.map((t) => h(ToastItem, { key: t.id, content: t.content }))
        ),
        container
    );
    
    // 检查渲染后的状态
    setTimeout(() => {
        const toastElements = container.querySelectorAll('.toast');
        console.log(`[DEBUG] After render #${renderCount}: found ${toastElements.length} .toast elements in DOM`);
        
        // 检查每个 toast 的内容
        toastElements.forEach((el, i) => {
            console.log(`[DEBUG]   toast[${i}]: "${el.textContent}"`);
        });
    }, 100);
}

// 第一次渲染
renderToasts([{ id: 1, content: 'Toast 1' }]);

setTimeout(() => {
    // 第二次渲染 - 添加一个
    renderToasts([
        { id: 1, content: 'Toast 1' },
        { id: 2, content: 'Toast 2' }
    ]);
    
    setTimeout(() => {
        // 第三次渲染 - 再添加一个
        renderToasts([
            { id: 1, content: 'Toast 1' },
            { id: 2, content: 'Toast 2' },
            { id: 3, content: 'Toast 3' }
        ]);
        
        setTimeout(() => {
            console.log("[DEBUG] Final check:");
            const toasts = container.querySelectorAll('.toast');
            console.log(`[DEBUG] Total .toast elements: ${toasts.length}`);
            
            if (toasts.length === 3) {
                console.log("[TEST_PASS] Correct number of toasts");
            } else {
                console.log(`[TEST_FAIL] Expected 3 toasts, got ${toasts.length}`);
            }
            
            console.log("[TEST_END]");
        }, 500);
    }, 500);
}, 500);
