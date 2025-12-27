/**
 * 最小化测试 - 确保基本渲染工作
 */

import { h, render } from 'preact';

console.log('=== 最小化测试 ===');

function App() {
  return h('div', {
    style: {
      padding: '40px',
      fontFamily: 'Arial, sans-serif',
      background: '#f0f0f0',
      minHeight: '100vh',
      margin: 0
    }
  }, [
    h('h1', { style: { color: '#27ae60' } }, '✓ 测试成功！'),
    h('p', {}, '如果你能看到这个页面，说明渲染正常工作。'),
    h('div', {
      style: {
        marginTop: '20px',
        padding: '15px',
        background: 'white',
        borderRadius: '8px'
      }
    }, [
      h('h2', {}, 'Range API 测试'),
      h('p', { id: 'range-result' }, '测试中...')
    ])
  ]);
}

// 渲染
render(h(App), document.body);
console.log('✓ 渲染完成');

// 测试 Range API
setTimeout(() => {
  try {
    const p = document.createElement('p');
    p.textContent = 'Hello World';
    document.body.appendChild(p);
    
    const range = document.createRange();
    range.setStart(p.firstChild, 0);
    range.setEnd(p.firstChild, 5);
    
    const result = document.getElementById('range-result');
    if (result) {
      result.textContent = `✓ Range API 工作正常: "${range.toString()}"`;
      result.style.color = '#27ae60';
    }
    
    console.log(`✓ Range API: "${range.toString()}"`);
  } catch (e) {
    console.error('✗ Range API 错误:', e);
    const result = document.getElementById('range-result');
    if (result) {
      result.textContent = `✗ 错误: ${e.message}`;
      result.style.color = '#dc3545';
    }
  }
}, 100);
