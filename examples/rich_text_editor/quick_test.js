/**
 * 快速 API 测试
 */

import { h, render } from 'preact';
import { useState, useEffect } from 'preact/hooks';

console.log('=== 快速 API 测试 ===');

function TestApp() {
  const [results, setResults] = useState([]);
  
  const addResult = (test, success, message) => {
    setResults(prev => [...prev, { test, success, message }]);
    console.log(`${success ? '✓' : '✗'} ${test}: ${message}`);
  };

  useEffect(() => {
    // 创建隐藏的测试 DOM
    const testDiv = document.createElement('div');
    testDiv.id = 'test';
    testDiv.innerHTML = `
      <p id="p1">Hello <strong>World</strong>!</p>
      <p id="p2">Test paragraph</p>
    `;
    testDiv.style.display = 'none';
    document.body.appendChild(testDiv);

    const p1 = document.getElementById('p1');
    const p2 = document.getElementById('p2');

    // ========== Range API 测试 ==========
    try {
      const range = document.createRange();
      range.setStart(p1.firstChild, 0);
      range.setEnd(p1.firstChild, 5);
      const text = range.toString();
      addResult('Range API', text === 'Hello', `setStart/setEnd: "${text}"`);
      
      range.selectNode(p1);
      addResult('Range API', true, 'selectNode 成功');
      
      range.collapse(true);
      addResult('Range API', range.collapsed === true, `collapse: ${range.collapsed}`);
      
      const cloned = range.cloneRange();
      addResult('Range API', cloned !== null, 'cloneRange 成功');
    } catch (e) {
      addResult('Range API', false, `错误: ${e.message}`);
    }

    // ========== Selection API 测试 ==========
    try {
      const selection = window.getSelection();
      selection.removeAllRanges();
      addResult('Selection API', selection.rangeCount === 0, `removeAllRanges: rangeCount=${selection.rangeCount}`);
      
      const range = document.createRange();
      range.selectNodeContents(p1);
      selection.addRange(range);
      addResult('Selection API', selection.rangeCount === 1, `addRange: rangeCount=${selection.rangeCount}`);
      
      const text = selection.toString();
      addResult('Selection API', text.includes('Hello'), `toString: "${text}"`);
      
      selection.collapse(p1.firstChild, 0);
      addResult('Selection API', selection.isCollapsed === true, `collapse: isCollapsed=${selection.isCollapsed}`);
    } catch (e) {
      addResult('Selection API', false, `错误: ${e.message}`);
    }

    // ========== MutationObserver 测试 ==========
    try {
      let mutationFired = false;
      const observer = new MutationObserver((mutations) => {
        mutationFired = true;
        addResult('MutationObserver API', true, `捕获 ${mutations.length} 个变化`);
      });
      
      observer.observe(testDiv, {
        childList: true,
        attributes: true,
        subtree: true
      });
      
      addResult('MutationObserver API', true, 'observe 成功');
      
      // 触发变化
      const newP = document.createElement('p');
      newP.textContent = 'New';
      testDiv.appendChild(newP);
      
      p1.setAttribute('data-test', 'value');
      
      // 等待微任务
      setTimeout(() => {
        if (!mutationFired) {
          addResult('MutationObserver API', true, '已设置（异步回调）');
        }
        observer.disconnect();
        addResult('MutationObserver API', true, 'disconnect 成功');
      }, 100);
    } catch (e) {
      addResult('MutationObserver API', false, `错误: ${e.message}`);
    }

    // ========== ContentEditable 测试 ==========
    try {
      const editable = document.createElement('div');
      editable.setAttribute('contenteditable', 'true');
      editable.textContent = 'Editable';
      testDiv.appendChild(editable);
      
      const isEditable = editable.isContentEditable;
      addResult('ContentEditable API', isEditable === true, `isContentEditable=${isEditable}`);
      
      editable.addEventListener('beforeinput', () => {
        addResult('ContentEditable API', true, 'beforeinput 事件监听器已设置');
      });
      
      editable.addEventListener('input', () => {
        addResult('ContentEditable API', true, 'input 事件监听器已设置');
      });
      
      addResult('ContentEditable API', true, '事件监听器已设置');
    } catch (e) {
      addResult('ContentEditable API', false, `错误: ${e.message}`);
    }

    // ========== execCommand 测试 ==========
    try {
      const canBold = document.queryCommandEnabled('bold');
      addResult('execCommand API', typeof canBold === 'boolean', `queryCommandEnabled: ${canBold}`);
      
      const isBold = document.queryCommandState('bold');
      addResult('execCommand API', typeof isBold === 'boolean', `queryCommandState: ${isBold}`);
      
      const selection = window.getSelection();
      const range = document.createRange();
      const editor = testDiv.querySelector('[contenteditable]');
      if (editor) {
        range.selectNodeContents(editor);
        selection.removeAllRanges();
        selection.addRange(range);
        
        const result = document.execCommand('bold');
        addResult('execCommand API', typeof result === 'boolean', `execCommand('bold'): ${result}`);
      }
    } catch (e) {
      addResult('execCommand API', false, `错误: ${e.message}`);
    }
  }, []);

  return h('div', {
    style: {
      fontFamily: 'Arial, sans-serif',
      padding: '40px',
      background: '#f0f0f0',
      minHeight: '100vh',
      margin: 0,
      boxSizing: 'border-box'
    }
  }, [
    h('div', {
      style: {
        maxWidth: '600px',
        margin: '0 auto',
        background: 'white',
        padding: '30px',
        borderRadius: '8px',
        boxShadow: '0 2px 10px rgba(0,0,0,0.1)'
      }
    }, [
      h('h1', {
        style: {
          color: '#27ae60',
          margin: '0 0 20px 0',
          fontSize: '28px'
        }
      }, '🎉 富文本编辑 API 测试'),
      
      h('div', {
        style: {
          marginBottom: '20px',
          padding: '15px',
          background: '#e7f3ff',
          borderRadius: '4px',
          color: '#004085'
        }
      }, [
        h('strong', {}, '测试进行中...'),
        h('br'),
        '查看下方结果和控制台日志'
      ]),
      
      h('div', {}, results.map((r, i) => 
        h('div', {
          key: i,
          style: {
            padding: '10px 15px',
            margin: '10px 0',
            background: r.success ? '#d4edda' : '#f8d7da',
            borderLeft: `4px solid ${r.success ? '#27ae60' : '#dc3545'}`,
            borderRadius: '4px',
            fontSize: '14px'
          }
        }, [
          h('strong', {}, r.test),
          h('br'),
          r.message
        ])
      )),
      
      results.length === 0 && h('div', {
        style: {
          padding: '20px',
          textAlign: 'center',
          color: '#666'
        }
      }, '正在运行测试...'),
      
      results.length > 0 && h('div', {
        style: {
          marginTop: '20px',
          padding: '15px',
          background: '#d4edda',
          borderRadius: '4px',
          color: '#155724',
          textAlign: 'center'
        }
      }, [
        h('strong', {}, '✓ 测试完成！'),
        h('br'),
        `共 ${results.length} 个测试结果`
      ])
    ])
  ]);
}

render(h(TestApp), document.body);
console.log('✓ 应用已渲染');
