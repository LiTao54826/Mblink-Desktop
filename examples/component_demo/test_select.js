/**
 * Select 组件测试
 */

import { h, render } from 'preact';
import { useState } from 'preact/hooks';
import { Select } from '../../js/components/select.js';

const options = [
  { value: 'apple', label: '苹果' },
  { value: 'banana', label: '香蕉' },
  { value: 'orange', label: '橙子' },
  { value: 'grape', label: '葡萄', disabled: true },
  { value: 'watermelon', label: '西瓜' },
];

function App() {
  const [value1, setValue1] = useState(null);
  const [value2, setValue2] = useState('banana');

  return h('div', { style: { padding: '20px' } }, [
    h('h1', { key: 'title', style: { marginBottom: '20px' } }, 'Select Test'),
    
    h('div', { key: 'basic', style: { marginBottom: '16px', width: '200px' } }, [
      h('p', { style: { marginBottom: '8px' } }, 'Basic Select:'),
      h(Select, {
        options,
        placeholder: '请选择水果...',
        value: value1,
        onChange: setValue1,
      }),
    ]),
    
    h('div', { key: 'clearable', style: { marginBottom: '16px', width: '200px' } }, [
      h('p', { style: { marginBottom: '8px' } }, 'Clearable Select:'),
      h(Select, {
        options,
        placeholder: '请选择水果...',
        value: value2,
        onChange: setValue2,
        clearable: true,
      }),
    ]),
    
    h('div', { key: 'disabled', style: { marginBottom: '16px', width: '200px' } }, [
      h('p', { style: { marginBottom: '8px' } }, 'Disabled Select:'),
      h(Select, {
        options,
        placeholder: '已禁用',
        disabled: true,
      }),
    ]),
    
    h('div', { key: 'values', style: { marginTop: '20px', padding: '12px', background: '#f5f5f5', borderRadius: '4px' } }, [
      h('p', {}, `Select 1: ${value1 || '(empty)'}`),
      h('p', {}, `Select 2: ${value2 || '(empty)'}`),
    ]),
  ]);
}

console.log('Starting render...');
render(h(App), document.body);
console.log('Render complete');
