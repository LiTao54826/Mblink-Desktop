/**
 * 调试编辑器 - 最小化测试 contentEditable 和 innerHTML
 */

import { h, render } from 'preact';
import { useRef, useEffect } from 'preact/hooks';

console.log('=== 调试编辑器测试 ===');

function DebugEditor() {
  const editorRef = useRef(null);

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
    }),
    
    // 测试 2: 直接使用子元素
    h('h3', {}, '测试 2: 直接子元素 + contentEditable'),
    h('div', {
      contentEditable: true,
      style: {
        border: '2px solid green',
        padding: '10px',
        minHeight: '100px',
        marginBottom: '20px'
      }
    }, [
      h('p', {}, '这是直接作为子元素的内容'),
      h('p', {}, [
        h('strong', {}, '粗体'),
        ' 和 ',
        h('em', {}, '斜体')
      ])
    ]),
    
    // 测试 3: 纯文本
    h('h3', {}, '测试 3: 纯文本 + contentEditable'),
    h('div', {
      contentEditable: true,
      style: {
        border: '2px solid red',
        padding: '10px',
        minHeight: '100px'
      }
    }, '这是纯文本内容，可以编辑')
  ]);
}

render(h(DebugEditor), document.body);
console.log('=== 渲染完成 ===');
