/**
 * Fluent 组件完整测试
 */

import { h, render } from 'preact';
import { useState } from 'preact/hooks';

console.log('=== Fluent Complete Test ===');

// 导入 Fluent 组件
import {
  Button,
  Input,
  Checkbox,
  Switch,
  Card, CardHeader, CardFooter,
  Text, Title, Body, Caption,
  Badge,
  Avatar,
  Divider,
  Spinner,
  colors, spacing, shadow, borderRadius,
} from '../../js/fluent/index.js';

console.log('All components imported!');

// 简单的 Section 组件
function Section({ title, children }) {
  return h('div', {
    style: {
      marginBottom: spacing.l,
      padding: spacing.m,
      backgroundColor: colors.colorNeutralBackground1,
      borderRadius: borderRadius.large,
      boxShadow: shadow.shadow4,
    }
  }, [
    h('h3', { key: 'title', style: { marginBottom: spacing.m, color: colors.colorNeutralForeground1 } }, title),
    h('div', { key: 'content' }, children),
  ]);
}
console.log('Section component created!');
function App() {
  const [count, setCount] = useState(0);
  const [checked, setChecked] = useState(false);
  const [switched, setSwitched] = useState(false);
  const [inputValue, setInputValue] = useState('');

  return h('div', {
    style: {
      padding: spacing.xxl,
      backgroundColor: colors.colorNeutralBackground2,
      minHeight: '100vh',
      fontFamily: "'Segoe UI', sans-serif",
    }
  }, [
    // 标题
    h(Title, { key: 'title', style: { textAlign: 'center', marginBottom: spacing.l } }, 
      'MBink Fluent Design Demo'),
    
    // 按钮区
    h(Section, { key: 'buttons', title: 'Buttons' }, [
      h('div', { key: 'row', style: { display: 'flex', gap: spacing.s, flexWrap: 'wrap' } }, [
        h(Button, { key: 'b1', appearance: 'primary', onClick: () => setCount(c => c + 1) }, 'Primary (' + count + ')'),
        h(Button, { key: 'b2', appearance: 'secondary' }, 'Secondary'),
        h(Button, { key: 'b3', appearance: 'outline' }, 'Outline'),
        h(Button, { key: 'b4', appearance: 'subtle' }, 'Subtle'),
        h(Button, { key: 'b5', disabled: true }, 'Disabled'),
      ]),
    ]),

    // 输入区
    h(Section, { key: 'inputs', title: 'Inputs' }, [
      h('div', { key: 'row', style: { display: 'flex', gap: spacing.m, alignItems: 'center', flexWrap: 'wrap' } }, [
        h(Input, { 
          key: 'input',
          placeholder: 'Type something...',
          value: inputValue,
          onChange: (e, data) => setInputValue(data.value),
          style: { width: '200px' },
        }),
        h(Checkbox, { 
          key: 'check',
          label: 'Checkbox',
          checked: checked,
          onChange: (e, data) => setChecked(data.checked),
        }),
        h(Switch, { 
          key: 'switch',
          label: 'Switch',
          checked: switched,
          onChange: (e, data) => setSwitched(data.checked),
        }),
      ]),
      inputValue && h(Caption, { key: 'value', style: { marginTop: spacing.s } }, 'Value: ' + inputValue),
    ]),

    // 数据展示
    h(Section, { key: 'display', title: 'Data Display' }, [
      h('div', { key: 'row', style: { display: 'flex', gap: spacing.m, alignItems: 'center', flexWrap: 'wrap' } }, [
        h(Badge, { key: 'badge1', color: 'brand' }, 'Brand'),
        h(Badge, { key: 'badge2', color: 'success' }, 'Success'),
        h(Badge, { key: 'badge3', color: 'danger' }, 'Danger'),
        h(Avatar, { key: 'avatar1', name: 'John Doe', size: 32 }),
        h(Avatar, { key: 'avatar2', name: 'Jane Smith', size: 40 }),
        h(Spinner, { key: 'spinner', size: 'small' }),
      ]),
    ]),

    // 卡片
    h(Section, { key: 'cards', title: 'Cards' }, [
      h('div', { key: 'row', style: { display: 'flex', gap: spacing.m, flexWrap: 'wrap' } }, [
        h(Card, { key: 'card1', appearance: 'filled', style: { width: '250px' } }, [
          h(CardHeader, { key: 'header', header: 'Card Title', description: 'Card description' }),
          h(Body, { key: 'body' }, 'This is the card content.'),
          h(CardFooter, { key: 'footer' }, [
            h(Button, { key: 'action', appearance: 'primary', size: 'small' }, 'Action'),
          ]),
        ]),
        h(Card, { key: 'card2', appearance: 'outline', style: { width: '250px' } }, [
          h(CardHeader, { key: 'header', header: 'Outline Card' }),
          h(Body, { key: 'body' }, 'Another card with outline style.'),
        ]),
      ]),
    ]),

    // 分割线
    h(Divider, { key: 'divider', style: { marginTop: spacing.l } }, 'End of Demo'),
  ]);
}

console.log('Starting render...');
render(h(App), document.body);
console.log('Render complete');
