/**
 * Toast 渲染问题测试
 * 
 * 问题描述：Toast 出现时左上角有多余绘制，滚动鼠标后消失
 */

import { h, render } from 'preact';
import { useState } from 'preact/hooks';
import { Toast } from '../../js/components/toast.js';
import { Button } from '../../js/components/button.js';
import { Column, Row } from '../../js/components/layout.js';
import { Text } from '../../js/components/text.js';

function TestApp() {
  const [count, setCount] = useState(0);

  const showToast = () => {
    console.log('[DEBUG] showToast called, count:', count);
    Toast.success('Success! Count: ' + count);
    // setCount(c => c + 1);
  };

  const showMultipleToasts = () => {
    console.log('[DEBUG] showMultipleToasts called');
    Toast.success('First toast');
    setTimeout(() => Toast.info('Second toast'), 500);
    setTimeout(() => Toast.warning('Third toast'), 1000);
  };

  return h(Column, { gap: 20, style: { padding: '40px' } }, [
    h(Text, { size: 'xl', weight: 'bold' }, '     '),
    
    h(Row, { gap: 12 }, [
      h(Button, { onClick: showToast }, 'Show Toast'),
      // h(Button, { variant: 'secondary', onClick: showMultipleToasts }, 'Multiple Toasts'),
    ]),
    
    // h(Text, { size: 'sm' }, `Toast count: ${count}`),
    
    // 添加一些内容用于滚动测试
    h('div', { style: { height: '200px', background: '#f0f0f0', marginTop: '20px' } },
      h(Text, {}, 'Scroll area placeholder')
    ),
  ]);
}

console.log('[TEST_START] Toast Render Test');
console.log('[DEBUG] Rendering TestApp...');

render(h(TestApp), document.body);

console.log('[DEBUG] TestApp rendered');

// 自动显示 Toast 以便调试
setTimeout(() => {
  console.log('[DEBUG] Auto-showing toast...');
  Toast.success('Auto Toast for debugging!');
}, 100);
