// 测试连续添加多个 toast 然后自动移除
import { h, render } from 'preact';

document.body.style.margin = '0';
document.body.style.padding = '20px';
document.body.style.backgroundColor = '#f0f0f0';

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
      padding: '10px 16px',
      backgroundColor: '#ffffff',
      borderRadius: '6px',
      boxShadow: '0 4px 12px rgba(0,0,0,0.15)',
    }
  }, h('span', {}, content));
}

function renderToasts() {
  console.log('[DEBUG] renderToasts called, toasts.length=' + toasts.length);
  render(
    h('div', { style: { display: 'contents' } },
      toasts.map((toast) => h(ToastItem, { key: toast.id, content: toast.content }))
    ),
    container
  );
}

function addToast(content, duration = 1000) {
  const id = ++toastId;
  toasts.push({ id, content });
  console.log('[DEBUG] Added toast ' + id + ', total=' + toasts.length);
  renderToasts();
  
  setTimeout(() => {
    console.log('[DEBUG] Removing toast ' + id);
    const before = toasts.length;
    toasts = toasts.filter(t => t.id !== id);
    console.log('[DEBUG] After filter: ' + before + ' -> ' + toasts.length);
    renderToasts();
  }, duration);
  
  return id;
}

console.log('[TEST_START] Toast Multi Remove Test');

// 模拟快速连续点击 3 次
setTimeout(() => {
  console.log('[DEBUG] === Adding 3 toasts with 100ms interval ===');
  addToast('Toast 1', 1000);
}, 100);

setTimeout(() => {
  addToast('Toast 2', 1000);
}, 200);

setTimeout(() => {
  addToast('Toast 3', 1000);
}, 300);

// 在 