/**
 * 超简单测试
 */

import { h, render } from 'preact';

console.log('开始测试');

// 直接渲染
render(
  h('div', {
    style: {
      padding: '40px',
      background: '#f0f0f0',
      fontFamily: 'Arial'
    }
  }, [
    h('h1', { style: { color: 'green' } }, '✓ 渲染成功！'),
    h('p', {}, '这是一个简单的测试页面。')
  ]),
  document.body
);

console.log('渲染完成');
