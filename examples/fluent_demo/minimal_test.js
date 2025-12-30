/**
 * 最小化测试 - 模仿 terminal_logview_demo 的方式
 */

import { h, render } from 'preact';
import { useState } from 'preact/hooks';

console.log('=== Minimal Test ===');

// 添加样式（和 terminal_logview_demo 一样的方式）
const style = document.createElement('style');
style.textContent = `
    .app-container {
        padding: 40px;
        background: #fafafa;
        min-height: 100vh;
        font-family: 'Segoe UI', sans-serif;
    }
    .title {
        color: #0078d4;
        font-size: 24px;
        margin-bottom: 16px;
    }
    .btn {
        padding: 10px 20px;
        background: #0078d4;
        color: white;
        border: none;
        border-radius: 4px;
        font-size: 14px;
        cursor: pointer;
    }
    .btn:hover {
        background: #106ebe;
    }
`;
document.head.appendChild(style);

function App() {
  const [count, setCount] = useState(0);
  
  return h('div', { className: 'app-container' }, [
    h('h1', { key: 'title', className: 'title' }, 'Minimal Test'),
    h('p', { key: 'p1' }, 'If you see this, basic rendering works!'),
    h('button', {
      key: 'btn',
      className: 'btn',
      onClick: () => {
        console.log('Button clicked! count:', count + 1);
        setCount(count + 1);
      }
    }, 'Clicked: ' + count),
  ]);
}

console.log('Rendering to document.body...');
console.log('document.body:', document.body);
render(h(App), document.body);
console.log('Render complete');
console.log('document.body.innerHTML:', document.body.innerHTML);
