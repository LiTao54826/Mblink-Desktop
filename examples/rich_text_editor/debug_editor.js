/**
 * 调试编辑器 - 最小化测试 contentEditable 和 innerHTML
 */

import { h, render } from 'preact';
import { useRef, useEffect, useState } from 'preact/hooks';

console.log('=== 调试编辑器测试 ===');

function DebugEditor() {
  const editorRef = useRef(null);
  const [sourceCode, setSourceCode] = useState('');

  // 更新源码显示
  const updateSourceCode = () => {
    if (editorRef.current) {
      setSourceCode(editorRef.current.innerHTML);
    }
  };

  useEffect(() => {
    if (editorRef.current) {
      console.log('编辑器 DOM 元素:', editorRef.current);
      console.log('contentEditable 属性:', editorRef.current.getAttribute('contenteditable'));
      console.log('isContentEditable:', editorRef.current.isContentEditable);
      console.log('innerHTML:', editorRef.current.innerHTML);
      console.log('子节点数量:', editorRef.current.childNodes.length);
      
      // 遍历子节点
      for (let i = 0; i < editorRef.current.childNodes.length; i++) {
        const child = editorRef.current.childNodes[i];
        console.log(`子节点 ${i}:`, child.nodeName, child.textContent);
      }
      
      // 初始化源码显示
      updateSourceCode();
    }
  }, []);

  return h('div', {
    style: {
      padding: '20px',
      fontFamily: 'Arial, sans-serif'
    }
  }, [
    h('h1', {}, '调试编辑器'),
    
    // 测试 1: 使用 dangerouslySetInnerHTML
    h('h3', {}, '测试 1: dangerouslySetInnerHTML + contentEditable'),
    h('div', {
      ref: editorRef,
      contentEditable: true,
      style: {
        border: '2px solid blue',
        padding: '10px',
        minHeight: '100px',
        marginBottom: '20px'
      },
      dangerouslySetInnerHTML: {
        __html: '<p>这是通过 dangerouslySetInnerHTML 设置的内容</p><p><strong>粗体</strong> 和 <em>斜体</em></p>'
      }
      // 暂时移除事件监听器以测试焦点问题
      // onInput: updateSourceCode,
      // onKeyUp: updateSourceCode,
      // onMouseUp: updateSourceCode
    }),
    
    // 刷新按钮
    h('button', {
      onClick: updateSourceCode,
      style: {
        padding: '8px 16px',
        marginBottom: '10px',
        cursor: 'pointer'
      }
    }, '刷新源码'),
    
    // 源码显示框
    h('h3', {}, 'innerHTML 源码:'),
    h('pre', {
      style: {
        border: '1px solid #ccc',
        padding: '10px',
        backgroundColor: '#f5f5f5',
        whiteSpace: 'pre-wrap',
        wordBreak: 'break-all',
        minHeight: '60px',
        marginBottom: '20px',
        fontFamily: 'monospace',
        fontSize: '12px'
      }
    }, sourceCode || '(空)')
  ]);
}

render(h(DebugEditor), document.body);
console.log('=== 渲染完成 ===');
