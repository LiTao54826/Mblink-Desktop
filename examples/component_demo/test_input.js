/**
 * Input 组件测试
 */

import { h, render } from 'preact';
import { useState } from 'preact/hooks';
import { Input } from '../../js/components/input.js';
import { Textarea } from '../../js/components/textarea.js';

function App() {
  const [value1, setValue1] = useState('');
  const [value2, setValue2] = useState('');
  const [textareaValue, setTextareaValue] = useState('');

  return h('div', { style: { padding: '20px' } }, [
    h('h1', { key: 'title', style: { marginBottom: '20px' } }, 'Input Test'),
    
    h('div', { key: 'basic', style: { marginBottom: '16px' } }, [
      h('p', { style: { marginBottom: '8px' } }, 'Basic Input:'),
      h(Input, {
        placeholder: 'Enter text...',
        value: value1,
        onChange: setValue1,
      }),
    ]),
    
    h('div', { key: 'password', style: { marginBottom: '16px' } }, [
      h('p', { style: { marginBottom: '8px' } }, 'Password Input:'),
      h(Input, {
        type: 'password',
        placeholder: 'Enter password...',
        value: value2,
        onChange: setValue2,
      }),
    ]),
    
    h('div', { key: 'disabled', style: { marginBottom: '16px' } }, [
      h('p', { style: { marginBottom: '8px' } }, 'Disabled Input:'),
      h(Input, {
        placeholder: 'Disabled',
        disabled: true,
      }),
    ]),
    
    h('div', { key: 'textarea', style: { marginBottom: '16px' } }, [
      h('p', { style: { marginBottom: '8px' } }, 'Textarea:'),
      h(Textarea, {
        placeholder: 'Enter description...',
        value: textareaValue,
        onChange: setTextareaValue,
        showCount: true,
        maxLength: 200,
      }),
    ]),
    
    h('div', { key: 'values', style: { marginTop: '20px', padding: '12px', background: '#f5f5f5', borderRadius: '4px' } }, [
      h('p', {}, `Input 1: ${value1}`),
      h('p', {}, `Input 2: ${value2}`),
      h('p', {}, `Textarea: ${textareaValue}`),
    ]),
  ]);
}

console.log('Starting render...');
render(h(App), document.body);
console.log('Render complete');
