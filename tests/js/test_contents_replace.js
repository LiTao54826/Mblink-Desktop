// 测试 display: contents 替换场景（模拟 Preact render 行为）
// Preact 的 render 会替换整个 contents wrapper

function logTest(name, passed) {
    if (passed) {
        console.log(`[TEST_PASS] ${name}`);
    } else {
        console.log(`[TEST_FAIL] ${name}`);
    }
}

console.log("[TEST_START] Display Contents Replace Scenario");

// 创建 flex 容器（模拟 toast container）
const container = document.createElement('div');
container.id = 'toast-container';
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

// 模拟 Preact 的 render 函数
function renderToasts(toasts) {
    // Preact 会创建一个新的 display: contents wrapper
    const newWrapper = document.createElement('div');
    newWrapper.style.display = 'contents';
    
    toasts.forEach((toast, index) => {
        const toastEl = document.createElement('div');
        toastEl.className = 'toast';
        toastEl.textContent = toast.content;
        Object.assign(toastEl.style, {
            padding: '10px 16px',
            backgroundColor: '#ffffff',
            borderRadius: '6px',
            fontSize: '14px',
            color: '#1e293b'
        });
        newWrapper.appendChild(toastEl);
    });
    
    // 替换容器内容（这是 Preact render 的行为）
    container.innerHTML = '';
    container.appendChild(newWrapper);
}

// 第一次渲染：一个宽 toast
renderToasts([
    { id: 1, content: '这是一个很长很长很长很长很长的 Toast 消息内容' }
]);

document.body.offsetHeight;

setTimeout(() => {
    const toast1 = container.querySelector('.toast');
    const toast1Rect = toast1.getBoundingClientRect();
    console.log(`[DEBUG] After first render - toast1 width: ${toast1Rect.width}`);
    
    // 第二次渲染：添加一个窄 toast（模拟增量更新）
    renderToasts([
        { id: 1, content: '这是一个很长很长很长很长很长的 Toast 消息内容' },
        { id: 2, content: '短消息' }
    ]);
    
    document.body.offsetHeight;
    
    setTimeout(() => {
        const toasts = container.querySelectorAll('.toast');
        const toast1After = toasts[0];
        const toast2 = toasts[1];
        
        const toast1RectAfter = toast1After.getBoundingClientRect();
        const toast2Rect = toast2.getBoundingClientRect();
        
        console.log(`[DEBUG] After second render:`);
        console.log(`[DEBUG]   toast1 width: ${toast1RectAfter.width}, height: ${toast1RectAfter.height}`);
        console.log(`[DEBUG]   toast2 width: ${toast2Rect.width}, height: ${toast2Rect.height}`);
        console.log(`[DEBUG]   toast1 top: ${toast1RectAfter.top}, toast2 top: ${toast2Rect.top}`);
        
        // 获取样式
        const toast2Style = getComputedStyle(toast2);
        console.log(`[DEBUG]   toast2 fontSize: ${toast2Style.fontSize}`);
        console.log(`[DEBUG]   toast2 backgroundColor: ${toast2Style.backgroundColor}`);
        console.log(`[DEBUG]   toast2 color: ${toast2Style.color}`);
        
        // 测试1: toast2 应该比 toast1 窄
        const isNarrower = toast2Rect.width < toast1RectAfter.width;
        logTest("toast2 should be narrower than toast1", isNarrower);
        if (!isNarrower) {
            console.log(`  toast1 width: ${toast1RectAfter.width}`);
            console.log(`  toast2 width: ${toast2Rect.width}`);
        }
        
        // 测试2: toast2 字体大小应该是 14px
        const fontSize = parseFloat(toast2Style.fontSize);
        logTest("toast2 fontSize should be 14px", fontSize === 14);
        if (fontSize !== 14) {
            console.log(`  Expected: 14`);
            console.log(`  Actual: ${fontSize}`);
        }
        
        // 测试3: toast2 背景色应该是白色
        const bgColor = toast2Style.backgroundColor;
        const isWhite = bgColor === '#ffffff' || bgColor === 'rgb(255, 255, 255)' || bgColor === 'white';
        logTest("toast2 backgroundColor should be white", isWhite);
        if (!isWhite) {
            console.log(`  Expected: #ffffff`);
            console.log(`  Actual: ${bgColor}`);
        }
        
        // 测试4: toast2 应该在 toast1 下方
        const isBelow = toast2Rect.top > toast1RectAfter.bottom - 10; // 允许一点重叠
        logTest("toast2 should be below toast1", isBelow);
        if (!isBelow) {
            console.log(`  toast1 bottom: ${toast1RectAfter.bottom}`);
            console.log(`  toast2 top: ${toast2Rect.top}`);
        }
        
        // 测试5: 两个 toast 都应该有合理的宽度
        const toast1WidthOk = toast1RectAfter.width > 100 && toast1RectAfter.width < 500;
        const toast2WidthOk = toast2Rect.width > 50 && toast2Rect.width < 200;
        logTest("toast1 width in reasonable range", toast1WidthOk);
        logTest("toast2 width in reasonable range", toast2WidthOk);
        
        console.log("[TEST_END]");
    }, 300);
}, 300);
