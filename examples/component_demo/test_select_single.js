/**
 * 单个 Select 组件测试
 * 用于调试滚动时布局错误的问题
 */

import { h, render } from 'preact';
import { useState } from 'preact/hooks';
import { Select } from '../../js/components/select.js';

function App() {
  const [value, setValue] = useState('banana');

  const options = [
    { value: 'apple', label: '苹果' },
    { value: 'banana', label: '香蕉' },
    { value: 'orange', label: '橙子' },
  ];

  return h('div', { 
    style: { 
      padding: '20px',
      height: '500px',  // 足够高以产生滚动条
    } 
  }, [
    h('h3', { key: 'title' }, 'Select 布局测试'),
    h('p', { key: 'desc' }, '在 select 上滚动鼠标，观察布局是否正确'),
    h('div', { 
      key: 'select-container',
      style: { width: '200px', marginTop: '20px' } 
    }, [
      h(Select, {
        key: 'select',
        value: value,
        options: options,
        onChange: setValue,
        clearable: true,
      }),
    ]),
    // 添加一些内容使页面可以滚动
    h('div', { 
      key: 'spacer',
      style: { height: '400px', marginTop: '20px', background: '#f0f0f0' } 
    }, '滚动区域'),
  ]);
}

render(h(App), document.body);
