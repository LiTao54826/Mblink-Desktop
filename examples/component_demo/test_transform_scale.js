/**
 * Transform Scale 测试
 */

import { h, render } from 'preact';

function App() {
  return h('div', { style: { padding: '40px' } }, [
    h('h1', { key: 'title', style: { marginBottom: '20px' } }, 'Transform Scale Test'),
    
    // 正常大小
    h('div', { key: 'normal', style: { marginBottom: '40px' } }, [
      h('p', { style: { marginBottom: '8px' } }, 'Normal (scale 1):'),
      h('div', {
        style: {
          width: '50px',
          height: '50px',
          backgroundColor: '#3b82f6',
          borderRadius: '50%',
        }
      }),
    ]),
    
    // 放大
    h('div', { key: 'scale-up', style: { marginBottom: '40px' } }, [
      h('p', { style: { marginBottom: '8px' } }, 'Scale 2:'),
      h('div', {
        style: {
          width: '50px',
          height: '50px',
          backgroundColor: '#10b981',
          borderRadius: '50%',
          transform: 'scale(2)',
        }
      }),
    ]),
    
    // 缩小
    h('div', { key: 'scale-down', style: { marginBottom: '40px' } }, [
      h('p', { style: { marginBottom: '8px' } }, 'Scale 0.5:'),
      h('div', {
        style: {
          width: '50px',
          height: '50px',
          backgroundColor: '#f59e0b',
          borderRadius: '50%',
          transform: 'scale(0.5)',
        }
      }),
    ]),
    
    // 缩小到0
    h('div', { key: 'scale-zero', style: { marginBottom: '40px' } }, [
      h('p', { style: { marginBottom: '8px' } }, 'Scale 0 (should be invisible):'),
      h('div', {
        style: {
          width: '50px',
          height: '50px',
          backgroundColor: '#ef4444',
          borderRadius: '50%',
          transform: 'scale(0)',
        }
      }),
    ]),
  ]);
}

console.log('Starting render...');
render(h(App), document.body);
console.log('Render complete');
