/**
 * Radio 组件测试
 */

import { h, render } from 'preact';
import { useState } from 'preact/hooks';
import { Radio } from '../../js/components/radio.js';

const fruitOptions = [
  { value: 'apple', label: '苹果' },
  { value: 'banana', label: '香蕉' },
  { value: 'orange', label: '橙子' },
  { value: 'grape', label: '葡萄', disabled: true },
];

const sizeOptions = [
  { value: 'small', label: 'Small' },
  { value: 'medium', label: 'Medium' },
  { value: 'large', label: 'Large' },
];

function App() {
  const [fruit, setFruit] = useState('banana');
  const [size, setSize] = useState('medium');

  return h('div', { style: { padding: '20px' } }, [
    h('h1', { key: 'title', style: { marginBottom: '20px' } }, 'Radio Test'),
    
    // 单个 Radio
    h('div', { key: 'single', style: { marginBottom: '16px' } }, [
      h('p', { style: { marginBottom: '8px' } }, 'Single Radio:'),
      h(Radio, { checked: true }, '已选中'),
      h('span', { style: { marginLeft: '16px' } }),
      h(Radio, { checked: false }, '未选中'),
    ]),
    
    // 禁用状态
    h('div', { key: 'disabled', style: { marginBottom: '16px' } }, [
      h('p', { style: { marginBottom: '8px' } }, 'Disabled:'),
      h(Radio, { disabled: true }, '禁用未选中'),
      h('span', { style: { marginLeft: '16px' } }),
      h(Radio, { checked: true, disabled: true }, '禁用已选中'),
    ]),
    
    // Radio Group 水平
    h('div', { key: 'group-h', style: { marginBottom: '16px' } }, [
      h('p', { style: { marginBottom: '8px' } }, 'Radio Group (Horizontal):'),
      h(Radio.Group, {
        options: fruitOptions,
        value: fruit,
        onChange: setFruit,
      }),
    ]),
    
    // Radio Group 垂直
    h('div', { key: 'group-v', style: { marginBottom: '16px' } }, [
      h('p', { style: { marginBottom: '8px' } }, 'Radio Group (Vertical):'),
      h(Radio.Group, {
        options: sizeOptions,
        value: size,
        onChange: setSize,
        direction: 'vertical',
      }),
    ]),
    
    // 状态显示
    h('div', { key: 'values', style: { marginTop: '20px', padding: '12px', background: '#f5f5f5', borderRadius: '4px' } }, [
      h('p', {}, `Fruit: ${fruit}`),
      h('p', {}, `Size: ${size}`),
    ]),
  ]);
}

console.log('Starting render...');
render(h(App), document.body);
console.log('Render complete');
