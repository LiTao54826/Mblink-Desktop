// 调试渲染树同步问题
import { h, render } from 'preact';

console.log("[TEST_START] Debug Render Tree Sync");

// 创建容器
const container = document.createElement('div');
container.id = 'container';
Object.assign(container.style, {
    position: 'fixed',
    top: '20px',
    left: '50%',
    transform: 'translateX(-50%)',
    display: 'flex',
    flexDirection: 'column',
    alignItems: 'center',
    gap: '8px',
    border: '2px dashed red'
});
document.body.appendChild(container);

function ToastItem({ content }) {
    return h('div', {
        className: 'toast',
        style: {
            padding: '10px 16px',
            backgroundColor: '#ffffff',
            borderRadius: '6px',
            fontSize: '14px',
            color: '#1e293b'
        }
    }, h('span', {}, content));
}

function renderToasts(toasts) {
    console.log(`[DEBUG] Rendering ${toasts.length} toasts`);
    render(
        h('div', { style: { display: 'contents' } },
            toasts.map((t) => h(ToastItem, { key: t.id, content: t.content }))
        ),
        container
    );
}

// 第一次渲染
renderToasts([{ id: 1, content: '这是一个很长很长很长很长很长的 Toast 消息内容' }]);

setTimeout(() => {
    console.log("[DEBUG] === After first render ===");
    
    // 检查 DOM 结构
    console.log(`[DEBUG] container children: ${container.children.length}`);
    for (let i = 0; i < container.children.length; i++) {
        const child = container.children[i];
        console.log(`[DEBUG] child[${i}]: tag=${child.tagName}, display=${child.style.display}`);
    }
    
    const toast1 = container.querySelector('.toast');
    if (toast1) {
        const rect1 = toast1.getBoundingClientRect();
        const style1 = getComputedStyle(toast1);
        console.log(`[DEBUG] toast1: width=${rect1.width}, bg=${style1.backgroundColor}, fontSize=${style1.fontSize}`);
    }
    
    // 第二次渲染
    renderToasts([
        { id: 1, content: '这是一个很长很长很长很长很长的 Toast 消息内容' },
        { id: 2, content: '短消息' }
    ]);
    
    setTimeout(() => {
        console.log("[DEBUG] === After second render ===");
        
        // 检查 DOM 结构
        console.log(`[DEBUG] container children: ${container.children.length}`);
        for (let i = 0; i < container.children.length; i++) {
            const child = container.children[i];
            console.log(`[DEBUG] child[${i}]: tag=${child.tagName}, display=${child.style.display}, children=${child.children.length}`);
        }
        
        const toasts = container.querySelectorAll('.toast');
        console.log(`[DEBUG] Found ${toasts.length} .toast elements`);
        
        toasts.forEach((toast, i) => {
            const rect = toast.getBoundingClientRect();
            const style = getComputedStyle(toast);
            console.log(`[DEBUG] toast[${i}]: width=${rect.width}, height=${rect.height}, top=${rect.top}`);
            console.log(`[DEBUG] toast[${i}]: bg=${style.backgroundColor}, fontSize=${style.fontSize}, padding=${style.padding}`);
            console.log(`[DEBUG] toast[${i}]: text="${toast.textContent}"`);
        });
        
        // 检查第二个 toast 的父元素链
        if (toasts.length >= 2) {
            let el = toasts[1];
            let depth = 0;
            console.log("[DEBUG] toast[1] parent chain:");
            while (el && depth < 5) {
                const style = getComputedStyle(el);
                console.log(`[DEBUG]   ${depth}: tag=${el.tagName}, id=${el.id}, display=${style.display}`);
                el = el.parentElement;
                depth++;
            }
        }
        
        console.log("[TEST_END]");
    }, 500);
}, 500);
