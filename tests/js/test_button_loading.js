/**
 * 测试按钮 loading 状态下图标和文字的顺序
 * 模拟 Modal 中 Button 的实际使用场景
 */

import { h, render } from 'preact';
import { useState, useEffect } from 'preact/hooks';

// 模拟 Spinner 组件
function Spinner({ size = 14 }) {
  const spinnerStyle = {
    width: `${size}px`,
    height: `${size}px`,
    border: '2px solid transparent',
    borderTopColor: 'currentColor',
    borderRadius: '50%',
    display: 'inline-block',
  };
  return h('span', { style: spinnerStyle, className: 'spinner' });
}

// 模拟 Button 组件 - 和实际 Button 组件一样的逻辑
function Button({ loading = false, children }) {
  const baseStyle = {
    display: 'inline-flex',
    alignItems: 'center',
    justifyContent: 'center',
    gap: '6px',
    padding: '8px 16px',
    backgroundColor: '#3b82f6',
    color: 'white',
    border: 'none',
    borderRadius: '6px',
  };

  const iconElement = loading ? h(Spinner, { size: 14 }) : null;

  const content = [];
  if (iconElement) {
    content.push(iconElement);  // 没有 key
  }
  if (children) {
    content.push(children);  // children 是字符串 "OK"，没有 key
  }

  return h('button', { style: baseStyle }, ...content);
}

function TestApp() {
  const [loading, setLoading] = useState(false);

  useEffect(() => {
    console.log('[TEST_START] Button Loading Order Test (Modal Scenario)');
    
    setTimeout(() => {
      const button = document.querySelector('button');
      if (button) {
        console.log(`Initial: Button has ${button.childNodes.length} child nodes`);
        Array.from(button.childNodes).forEach((child, i) => {
          console.log(`  Node ${i}: nodeType=${child.nodeType}, nodeName=${child.nodeName}, text="${child.textContent}"`);
        });
      }
      
      console.log('Setting loading to true...');
      setLoading(true);
      
      setTimeout(() => {
        const button = document.querySelector('button');
        if (button) {
          console.log(`After: Button has ${button.childNodes.length} child nodes`);
          
          Array.from(button.childNodes).forEach((child, i) => {
            console.log(`  Node ${i}: nodeType=${child.nodeType}, nodeName=${child.nodeName}, text="${child.textContent}"`);
            if (child.nodeType === 1) {
              console.log(`    className="${child.className}"`);
              const rect = child.getBoundingClientRect();
              console.log(`    rect: x=${rect.x}, y=${rect.y}, w=${rect.width}, h=${rect.height}`);
            }
          });
          
          // 检查渲染树顺序
          console.log('Checking render children order...');
          const buttonRect = button.getBoundingClientRect();
          console.log(`Button rect: x=${buttonRect.x}, y=${buttonRect.y}, w=${buttonRect.width}, h=${buttonRect.height}`);
          
          // 获取所有子元素的位置，按 x 坐标排序
          const childPositions = [];
          Array.from(button.childNodes).forEach((child, i) => {
            let x = 0;
            if (child.nodeType === 1) {
              x = child.getBoundingClientRect().x;
            } else if (child.nodeType === 3) {
              const range = document.createRange();
              range.selectNodeContents(child);
              const rect = range.getBoundingClientRect();
              x = rect.x;
            }
            childPositions.push({ index: i, type: child.nodeName, x });
          });
          childPositions.sort((a, b) => a.x - b.x);
          console.log('Children sorted by x position:');
          childPositions.forEach(p => console.log(`  ${p.type} at x=${p.x} (DOM index ${p.index})`));
          
          // 检查顺序
          let spinnerX = -1, textX = -1;
          Array.from(button.childNodes).forEach((child, i) => {
            if (child.nodeType === 1) {  // Element
              const rect = child.getBoundingClientRect();
              if (child.className === 'spinner') spinnerX = rect.x;
              else textX = rect.x;  // 假设另一个元素包含文本
            } else if (child.nodeType === 3) {  // Text
              const range = document.createRange();
              range.selectNodeContents(child);
              const rect = range.getBoundingClientRect();
              if (child.textContent.trim() === 'OK') textX = rect.x;
            }
          });
          
          console.log(`spinnerX=${spinnerX}, textX=${textX}`);
          
          if (spinnerX >= 0 && textX >= 0) {
            if (spinnerX < textX) {
              console.log('[TEST_PASS] Spinner is on the left of text');
            } else {
              console.log('[TEST_FAIL] Spinner should be on the left of text');
            }
          } else {
            console.log('[TEST_FAIL] Could not find spinner or text');
          }
        }
        console.log('[TEST_END]');
      }, 500);
    }, 500);
  }, []);

  // 模拟 Modal 中的用法：children 是字符串
  return h('div', { style: { padding: '20px' } }, [
    h(Button, { loading }, 'OK'),
  ]);
}

render(h(TestApp), document.body);
