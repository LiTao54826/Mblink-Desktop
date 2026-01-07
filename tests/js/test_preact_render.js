// 测试真正的 Preact render 行为
import { h, render } from 'preact';

function logTest(name, passed) {
    if (passed) {
        console.log(`[TEST_PASS] ${name}`);
    } else {
        console.log(`[TEST_FAIL] ${name}`);
    }
}

console.log("[TEST_START] Preact Render with Display Contents");

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
    gap: '8px'
});
document.body.appendChild(container);

// Toast 组件
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
    }, content);
}

// 渲染函数
function renderToasts(toasts) {
    render(
        h('div', { style: { display: 'contents' } },
            toasts.map((t, i) => h(ToastItem, { key: t.id, content: t.content }))
        ),
        container
    );
}

// 第一次渲染
renderToasts([
    { id: 1, content: '这是一个很长很长很长很长很长的 Toast 消息内容' }
]);

document.body.offsetHeight;

setTimeout(() => {
    const toast1 = container.querySelector('.toast');
    const toast1Width = toast1.getBoundingClientRect().width;
    console.log(`[DEBUG] After first render - toast1 width: ${toast1Width}`);
    
    // 第二次渲染 - 添加短消息
    renderToasts([
        { id: 1, content: '这是一个很长很长很长很长很长的 Toast 消息内容' },
        { id: 2, content: '短消息' }
    ]);
    
    document.body.offsetHeight;
    
    setTimeout(() => {
        const toasts = container.querySelectorAll('.toast');
        console.log(`[DEBUG] Found ${toasts.length} toasts`);
        
        if (toasts.length < 2) {
            console.log("[TEST_FAIL] Should have 2 toasts");
            console.log("[TEST_END]");
            return;
        }
        
        const toast1After = toasts[0];
        const toast2 = toasts[1];
        
        const toast1Rect = toast1After.getBoundingClientRect();
        const toast2Rect = toast2.getBoundingClientRect();
        
        console.log(`[DEBUG] toast1: width=${toast1Rect.width}, height=${toast1Rect.height}`);
        console.log(`[DEBUG] toast2: width=${toast2Rect.width}, height=${toast2Rect.height}`);
        
        const toast2Style = getComputedStyle(toast2);
        console.log(`[DEBUG] toast2 fontSize: ${toast2Style.fontSize}`);
        console.log(`[DEBUG] toast2 backgroundColor: ${toast2Style.backgroundColor}`);
        
        // 测试
        logTest("toast2 narrower than toast1", toast2Rect.width < toast1Rect.width);
        logTest("toast2 fontSize is 14px", parseFloat(toast2Style.fontSize) === 14);
        
        const bgOk = toast2Style.backgroundColor === '#ffffff' || 
                     toast2Style.backgroundColor === 'rgb(255, 255, 255)';
        logTest("toast2 backgroundColor is white", bgOk);
        
        logTest("toast2 below toast1", toast2Rect.top > toast1Rect.bottom - 10);
        
        console.log("[TEST_END]");
    }, 500);
}, 500);
