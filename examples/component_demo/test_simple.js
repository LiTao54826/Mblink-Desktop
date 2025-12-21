/**
 * 简单组件测试
 */

import { h, render } from 'preact';
import { useState } from 'preact/hooks';

// 直接测试基础渲染
function App() {
  const [count, setCount] = useState(0);

  return h('div', { style: { padding: '20px', fontFamily: 'sans-serif' } }, [
    h('h1', { style: { color: '#333', marginBottom: '20px' } }, 'Component Test'),
    
    h('p', {}, `Count: ${count}`),
    
    h('button', {
      style: {
        padding: '8px 16px',
        backgroundColor: '#3b82f6',
        color: 'white',
        border: 'none',
        borderRadius: '4px',
        cursor: 'pointer',
        marginRight: '8px',
      },
      onClick: () => setCount(count + 1),
    }, 'Increment'),
    
    h('button', {
      style: {
        padding: '8px 16px',
        backgroundColor: '#ef4444',
        color: 'white',
        border: 'none',
        borderRadius: '4px',
        cursor: 'pointer',
      },
      onClick: () => setCount(0),
    }, 'Reset'),
  ]);
}

console.log('Rendering App...');
render(h(App), document.body);
console.log('App rendered');
