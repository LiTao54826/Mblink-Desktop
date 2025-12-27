/**
 * 简单测试 - 单行 contentEditable
 */

import { h, render } from 'preact';

console.log('=== 简单 contentEditable 测试 ===');

function SimpleEditor() {
  return h('div', {
    style: {
      padding: '20px',
      fontFamily: 'Arial, sans-serif'
    }
  }, [
    h('h1', {}, '简单测试'),
    
    // 单行文本
    h('div', {
      contentEditable: true,
      style: {
        border: '2px solid blue',
        padding: '10px',
        fontSize: '16px'
      }
    }, 'Hello World 你好世界')
  ]);
}

render(h(SimpleEditor), document.body);
console.log('=== 渲染完成 ===');
