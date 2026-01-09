/**
 * 直接导入 fluent 组件测试
 */

import { h, render } from 'preact';
import { Button } from '../../js/fluent/button.js';
import { colors, spacing, borderRadius } from '../../js/fluent/theme.js';

console.log('[TEST_START] Fluent Demo Buttons Test');

// 模拟 DemoRow
function DemoRow({ children }) {
  return h('div', {
    style: {
      display: 'flex',
      flexWrap: 'wrap',
      gap: spacing.m,
      alignItems: 'center',
      marginBottom: spacing.m,
      backgroundColor: '#e0e0e0',
      padding: '10px',
    }
  }, children);
}

// 创建测试 App
function App() {
  return h('div', { style: { padding: '24px' } }, [
    h(DemoRow, { key: 'sizes' }, [
      h(Button, { key: 'small', size: 'small', appearance: 'primary' }, 'Small'),
      h(Button, { key: 'medium', size: 'medium', appearance: 'primary' }, 'Medium'),
      h(Button, { key: 'large', size: 'large', appearance: 'primary' }, 'Large'),
    ]),
  ]);
}

render(h(App), document.body);

// 检测
setTimeout(() => {
    // 打印 DOM 结构
    console.log('[DEBUG] DOM structure:');
    console.log(document.body.innerHTML.substring(0, 500));
    
    const buttons = document.querySelectorAll('button');
    console.log('[DEBUG] Found ' + buttons.length + ' buttons');
    
    buttons.forEach((btn, i) => {
        const span = btn.querySelector('span');
        if (!span) {
            console.log('[DEBUG] Button ' + i + ' has no span');
            return;
        }
        
        const btnRect = btn.getBoundingClientRect();
        const spanRect = span.getBoundingClientRect();
        
        const actualOffset = spanRect.top - btnRect.top;
        const expectedOffset = (btnRect.height - spanRect.height) / 2;
        const diff = actualOffset - expectedOffset;
        
        console.log('[DEBUG] Button ' + i + ' (' + span.textContent + '):');
        console.log('  Button: height=' + btnRect.height + ', top=' + btnRect.top);
        console.log('  Span: height=' + spanRect.height + ', top=' + spanRect.top);
        console.log('  Actual offset: ' + actualOffset);
        console.log('  Expected offset: ' + expectedOffset);
        console.log('  Diff: ' + diff.toFixed(2));
        
        if (Math.abs(diff) < 1) {
            console.log('[TEST_PASS] Button ' + i + ' centered');
        } else {
            console.log('[TEST_FAIL] Button ' + i + ' NOT centered, diff=' + diff.toFixed(2));
        }
    });
    
    console.log('[TEST_END]');
}, 200);
