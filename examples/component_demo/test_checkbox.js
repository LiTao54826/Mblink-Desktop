/**
 * Checkbox 组件测试
 */

import { h, render } from 'preact';
import { useState } from 'preact/hooks';
import { Checkbox } from '../../js/components/checkbox.js';

const fruitOptions = [
  { value: 'apple', label: '苹果' },
  { value: 'banana', label: '香蕉' },
  { value: 'orange', label: '橙子' },
  { value: 'grape', label: '葡萄', disabled: true },
];

function App() {
  const [checked1, setChecked1] = useState(false);
  const [checked2, setChecked2] = useState(true);
  const [groupValue, setGroupValue] = useState(['apple', 'orange']);

  return h('div', { style: { padding: '20px' } }, [
    h('h1', { key: 'title', style: { marginBottom: '20px' } }, 'Checkbox Test'),
    
    // 基础 Checkbox
    h('div', { key: 'basic', style: { marginBottom: '16px' } }, [
      h('p', { style: { marginBottom: '8px' } }, 'Basic Checkbox:'),
      h(Checkbox, {
        checked: checked1,
        onChange: setChecked1,
      }, '同意用户协议'),
    ]),
    
    // 默认选中
    h('div', { key: 'checked', style: { marginBottom: '16px' } }, [
      h('p', { style: { marginBottom: '8px' } }, 'Default Checked:'),
      h(Checkbox, {
        checked: checked2,
        onChange: setChecked2,
      }, '记住密码'),
    ]),
    
    // 禁用状态
    h('div', { key: 'disabled', style: { marginBottom: '16px' } }, [
      h('p', { style: { marginBottom: '8px' } }, 'Disabled:'),
      h(Checkbox, { disabled: true }, '禁用未选中'),
      h('span', { style: { marginLeft: '16px' } }),
      h(Checkbox, { checked: true, disabled: true }, '禁用已选中'),
    ]),
    
    // 半选状态
    h('div', { key: 'indeterminate', style: { marginBottom: '16px' } }, [
      h('p', { style: { marginBottom: '8px' } }, 'Indeterminate:'),
      h(Checkbox, { indeterminate: true }, '部分选中'),
    ]),
    
    // Checkbox Group 水平
    h('div', { key: 'group-h', style: { marginBottom: '16px' } }, [
      h('p', { style: { marginBottom: '8px' } }, 'Checkbox Group (Horizontal):'),
      h(Checkbox.Group, {
        options: fruitOptions,
        value: groupValue,
        onChange: setGroupValue,
      }),
    ]),
    
    // Checkbox Group 垂直
    h('div', { key: 'group-v', style: { marginBottom: '16px' } }, [
      h('p', { style: { marginBottom: '8px' } }, 'Checkbox Group (Vertical):'),
      h(Checkbox.Group, {
        options: fruitOptions,
        value: groupValue,
        onChange: setGroupValue,
        direction: 'vertical',
      }),
    ]),
    
    // 状态显示
    h('div', { key: 'values', style: { marginTop: '20px', padding: '12px', background: '#f5f5f5', borderRadius: '4px' } }, [
      h('p', {}, `Checkbox 1: ${checked1}`),
      h('p', {}, `Checkbox 2: ${checked2}`),
      h('p', {}, `Group: [${groupValue.join(', ')}]`),
    ]),
  ]);
}

console.log('Starting render...');
render(h(App), document.body);
console.log('Render complete');
