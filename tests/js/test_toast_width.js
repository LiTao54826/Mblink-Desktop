// 测试 Toast 宽度问题 - 使用 Preact 模拟实际 Toast 组件
import { h, render } from 'preact';

document.body.style.margin = '0';
document.body.style.padding = '20px';
document.body.style.backgroundColor = '#f0f0f0';

// Toast 容器（与实际 Toast 组件相同的样式）
let container = null;
let toasts = [];
let toastId = 0;

function getContainer() {
  if (!container) {
    container = document.createElement('div');
    container.id = 'lightui-toast-container';
    Object.assign(container.style, {
      position: 'fixed',
      top: '20px',
      left: '50%',
      transform: 'translateX(-50%)',
      zIndex: '9999',
      display: 'flex',
      flexDirection: 'column',
      alignItems: 'center',
      gap: '8px',
      pointerEvents: 'none',
      // 调试边框
      border: '2px dashed red',
    });
    document.body.appendChild(container);
  }
  return container;
}

// Toast 项组件
function ToastItem({ content }) {
  const toastStyle = {
    display: 'flex',
    alignItems: 'center',
    gap: '8px',
    padding: '10px 16px',
    backgroundColor: '#ffffff',
    borderRadius: '6px',
    boxShadow: '0 4px 12px rgba(0,0,0,0.15)',
    fontSize: '14px',
    color: '#1e293b',
    pointerEvents: 'auto',
  };

  return h('div', { style: toastStyle }, h('span', {}, content));
}

// 渲染所有 Toast（与实际 Toast 组件相同）
function renderToasts() {
  const container = getContainer();
  render(
    h('div', { style: { display: 'contents' } },
      toasts.map((toast) => h(ToastItem, { key: toast.id, content: toast.content }))
    ),
    container
  );
}

// 添加 Toast
function addToast(content) {
  const id = ++toastId;
  toasts.push({ id, content });
  renderToasts();
  return id;
}

// 清空
function clearToasts() {
  toasts = [];
  renderToasts();
}

// 添加说明和按钮
const info = document.createElement('div');
info.innerHTML = `
  <h2></h2>
  <p></p>
  <p></p>
  <button id="addWide">添加宽 Toast</button>
  <button id="addNarrow">添加窄 Toast</button>
  <button id="clear">清空</button>
`;
document.body.appendChild(info);

document.getElementById('addWide').onclick = () => {
  addToast('这是一个很长很长很长很长很长的 Toast 消息内容');
};

document.getElementById('addNarrow').onclick = () => {
  addToast('短消息');
};

document.getElementById('clear').onclick = () => {
  clearToasts();
};

// 自动测试
setTimeout(() => {
  console.log('[TEST] 添加宽 Toast');
  addToast('这是一个很长很长很长很长很长的 Toast 消息内容');

  setTimeout(() => {
    console.log('[TEST] 添加窄 Toast');
    addToast('短消息');
  }, 500);
}, 500);

