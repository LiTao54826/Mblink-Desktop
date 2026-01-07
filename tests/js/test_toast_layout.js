// 测试 Toast 布局 - 验证 display: contents 增量更新
import { h, render } from 'preact';

document.body.style.margin = '0';
document.body.style.padding = '20px';
document.body.style.backgroundColor = '#f0f0f0';

// Toast 容器
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
  gap: '8px',
  border: '2px dashed red',
});
document.body.appendChild(container);

let toasts = [];
let toastId = 0;

function ToastItem({ content }) {
  return h('div', { 
    style: {
      display: 'flex',
      alignItems: 'center',
      padding: '10px 16px',
      backgroundColor: '#ffffff',
      borderRadius: '6px',
      boxShadow: '0 4px 12px rgba(0,0,0,0.15)',
      fontSize: '14px',
      color: '#1e293b',
    }
  }, h('span', {}, content));
}

function renderToasts() {
  render(
    h('div', { style: { display: 'contents' } },
      toasts.map((toast) => h(ToastItem, { key: toast.id, content: toast.content }))
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

function checkLayout() {
  // 获取 contents 包装器
  const wrapper = container.children[0];
  if (!wrapper) {
    console.log('[TEST_FAIL] No wrapper found');
    return false;
  }
  
  const toastElements = wrapper.children;
  console.log('[DEBUG] Toast count:', toastElements.length, 'Expected:', toasts.length);
  
  if (toastElements.length !== toasts.length) {
    console.log('[TEST_FAIL] Wrong toast count');
    return false;
  }
  
  let prevBottom = 0;
  let allPassed = true;
  
  for (let i = 0; i < toastElements.length; i++) {
    const rect = toastElements[i].getBoundingClientRect();
    console.log('[DEBUG] Toast ' + i + ': y=' + Math.round(rect.y) + 
      ' h=' + Math.round(rect.height) + ' w=' + Math.round(rect.width));
    
    // 检查宽度是否合理（不应该是 0 或异常大）
    if (rect.width <= 0 || rect.width > 800) {
      console.log('[TEST_FAIL] Toast ' + i + ' has invalid width: ' + rect.width);
      allPassed = false;
    }
    
    // 检查高度是否合理
    if (rect.height <= 0 || rect.height > 200) {
      console.log('[TEST_FAIL] Toast ' + i + ' has invalid height: ' + rect.height);
      allPassed = false;
    }
    
    // 检查是否在前一个下面（不重叠）
    if (i > 0 && rect.y < prevBottom - 1) {
      console.log('[TEST_FAIL] Toast ' + i + ' overlaps with previous');
      allPassed = false;
    }
    
    prevBottom = rect.y + rect.height;
  }
  
  return allPassed;
}

console.log('[TEST_START] Toast Layout Test');

setTimeout(() => {
  // Step 1: 添加宽 Toast
  console.log('[DEBUG] Step 1: Add wide toast');
  addToast('这是一个很长很长很长很长很长的 Toast 消息内容');
  
  setTimeout(() => {
    if (checkLayout()) {
      console.log('[TEST_PASS] Step 1 layout correct');
    }
    
    // Step 2: 添加窄 Toast
    console.log('[DEBUG] Step 2: Add narrow toast');
    addToast('短消息');
    
    setTimeout(() => {
      if (checkLayout()) {
        console.log('[TEST_PASS] Step 2 layout correct');
      }
      
      // Step 3: 再添加一个
      console.log('[DEBUG] Step 3: Add another toast');
      addToast('第三条消息');
      
      setTimeout(() => {
        if (checkLayout()) {
          console.log('[TEST_PASS] Step 3 layout correct');
        }
        
        // 验证每个 Toast 的宽度是独立的（不应该被拉伸）
        const wrapper = container.children[0];
        if (wrapper && wrapper.children.length >= 2) {
          const wide = wrapper.children[0].getBoundingClientRect();
          const narrow = wrapper.children[1].getBoundingClientRect();
          
          console.log('[DEBUG] Wide toast width:', Math.round(wide.width));
          console.log('[DEBUG] Narrow toast width:', Math.round(narrow.width));
          
          // 窄 Toast 不应该和宽 Toast 一样宽
          if (narrow.width < wide.width * 0.9) {
            console.log('[TEST_PASS] Toast widths are independent');
          } else {
            console.log('[TEST_FAIL] Narrow toast is stretched to match wide toast');
          }
        }
        
        console.log('[TEST_END]');
      }, 300);
    }, 300);
  }, 300);
}, 100);
