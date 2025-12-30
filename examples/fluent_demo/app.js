/**
 * MBink Fluent Design 组件库演示
 */

import { h, render, createContext } from 'preact';
import { useState, useContext } from 'preact/hooks';

// 导入 Fluent 组件
import {
  Button, CompoundButton, ToggleButton,
  Input, SearchBox, Textarea, Select,
  Checkbox, Switch, Radio, RadioGroup,
  Text, Title, Subtitle, Body, Caption,
  Badge, CounterBadge, PresenceBadge,
  Avatar, AvatarGroup,
  Card, CardHeader, CardFooter,
  Divider, Spinner, Link,
  colors, spacing, shadow, borderRadius,
} from '../../js/fluent/index.js';

// ============================================
// 演示区块组件
// ============================================

function DemoSection({ title, children }) {
  return h('div', {
    style: {
      marginBottom: spacing.xxl,
      padding: spacing.l,
      backgroundColor: colors.colorNeutralBackground1,
      borderRadius: borderRadius.large,
      boxShadow: shadow.shadow4,
    }
  }, [
    h(Subtitle, { key: 'title', style: { marginBottom: spacing.m } }, title),
    h('div', { key: 'content' }, children),
  ]);
}

function DemoRow({ children }) {
  return h('div', {
    style: {
      display: 'flex',
      flexWrap: 'wrap',
      gap: spacing.m,
      alignItems: 'center',
      marginBottom: spacing.m,
    }
  }, children);
}

// ============================================
// 按钮演示
// ============================================

function ButtonDemo() {
  const [loading, setLoading] = useState(false);
  const [toggled, setToggled] = useState(false);

  return h(DemoSection, { title: 'Buttons' }, [
    // 外观变体
    h(Caption, { key: 'label1', style: { display: 'block', marginBottom: spacing.s } }, 'Appearances'),
    h(DemoRow, { key: 'appearances' }, [
      h(Button, { key: 'primary', appearance: 'primary' }, 'Primary'),
      h(Button, { key: 'secondary', appearance: 'secondary' }, 'Secondary'),
      h(Button, { key: 'outline', appearance: 'outline' }, 'Outline'),
      h(Button, { key: 'subtle', appearance: 'subtle' }, 'Subtle'),
      h(Button, { key: 'transparent', appearance: 'transparent' }, 'Transparent'),
    ]),

    // 尺寸
    h(Caption, { key: 'label2', style: { display: 'block', marginBottom: spacing.s } }, 'Sizes'),
    h(DemoRow, { key: 'sizes' }, [
      h(Button, { key: 'small', size: 'small', appearance: 'primary' }, 'Small'),
      h(Button, { key: 'medium', size: 'medium', appearance: 'primary' }, 'Medium'),
      h(Button, { key: 'large', size: 'large', appearance: 'primary' }, 'Large'),
    ]),

    // 形状
    h(Caption, { key: 'label3', style: { display: 'block', marginBottom: spacing.s } }, 'Shapes'),
    h(DemoRow, { key: 'shapes' }, [
      h(Button, { key: 'rounded', shape: 'rounded', appearance: 'primary' }, 'Rounded'),
      h(Button, { key: 'circular', shape: 'circular', appearance: 'primary', icon: '★' }),
      h(Button, { key: 'square', shape: 'square', appearance: 'primary' }, 'Square'),
    ]),

    // 状态
    h(Caption, { key: 'label4', style: { display: 'block', marginBottom: spacing.s } }, 'States'),
    h(DemoRow, { key: 'states' }, [
      h(Button, { key: 'disabled', appearance: 'primary', disabled: true }, 'Disabled'),
      h(ToggleButton, { 
        key: 'toggle',
        checked: toggled,
        onChange: (e, data) => setToggled(data.checked),
      }, toggled ? 'Toggled On' : 'Toggle Me'),
    ]),

    // 复合按钮
    h(Caption, { key: 'label5', style: { display: 'block', marginBottom: spacing.s } }, 'Compound Button'),
    h(DemoRow, { key: 'compound' }, [
      h(CompoundButton, {
        key: 'compound',
        appearance: 'secondary',
        secondaryContent: 'Secondary text here',
      }, 'Compound Button'),
    ]),
  ]);
}

// ============================================
// 输入组件演示
// ============================================

function InputDemo() {
  const [inputValue, setInputValue] = useState('');
  const [selectValue, setSelectValue] = useState('');
  const [checked, setChecked] = useState(false);
  const [switched, setSwitched] = useState(false);
  const [radioValue, setRadioValue] = useState('option1');

  const selectOptions = [
    { value: 'option1', label: 'Option 1' },
    { value: 'option2', label: 'Option 2' },
    { value: 'option3', label: 'Option 3' },
    { value: 'disabled', label: 'Disabled Option', disabled: true },
  ];

  return h(DemoSection, { title: 'Input Components' }, [
    // Input
    h(Caption, { key: 'label1', style: { display: 'block', marginBottom: spacing.s } }, 'Input'),
    h(DemoRow, { key: 'inputs' }, [
      h(Input, {
        key: 'outline',
        appearance: 'outline',
        placeholder: 'Outline input',
        style: { width: '200px' },
      }),
      h(Input, {
        key: 'underline',
        appearance: 'underline',
        placeholder: 'Underline input',
        style: { width: '200px' },
      }),
      h(Input, {
        key: 'filled',
        appearance: 'filledDarker',
        placeholder: 'Filled input',
        style: { width: '200px' },
      }),
    ]),

    // SearchBox
    h(Caption, { key: 'label2', style: { display: 'block', marginBottom: spacing.s } }, 'SearchBox'),
    h(DemoRow, { key: 'search' }, [
      h(SearchBox, {
        key: 'search',
        placeholder: 'Search...',
        value: inputValue,
        onChange: (e, data) => setInputValue(data.value),
        onClear: () => setInputValue(''),
        style: { width: '300px' },
      }),
    ]),

    // Textarea
    h(Caption, { key: 'label3', style: { display: 'block', marginBottom: spacing.s } }, 'Textarea'),
    h(DemoRow, { key: 'textarea' }, [
      h(Textarea, {
        key: 'textarea',
        placeholder: 'Enter multiple lines...',
        resize: 'vertical',
        style: { width: '300px' },
      }),
    ]),

    // Select
    h(Caption, { key: 'label4', style: { display: 'block', marginBottom: spacing.s } }, 'Select'),
    h(DemoRow, { key: 'select' }, [
      h(Select, {
        key: 'select',
        options: selectOptions,
        value: selectValue,
        onChange: (e, data) => setSelectValue(data.value),
        placeholder: 'Select an option',
        style: { width: '200px' },
      }),
    ]),

    // Checkbox & Switch
    h(Caption, { key: 'label5', style: { display: 'block', marginBottom: spacing.s } }, 'Checkbox & Switch'),
    h(DemoRow, { key: 'checks' }, [
      h(Checkbox, {
        key: 'checkbox',
        label: 'Checkbox',
        checked: checked,
        onChange: (e, data) => setChecked(data.checked),
      }),
      h(Checkbox, { key: 'mixed', label: 'Mixed', checked: 'mixed' }),
      h(Checkbox, { key: 'disabled', label: 'Disabled', disabled: true }),
      h(Switch, {
        key: 'switch',
        label: 'Switch',
        checked: switched,
        onChange: (e, data) => setSwitched(data.checked),
      }),
    ]),

    // Radio
    h(Caption, { key: 'label6', style: { display: 'block', marginBottom: spacing.s } }, 'Radio Group'),
    h(RadioGroup, {
      key: 'radio',
      value: radioValue,
      onChange: (e, data) => setRadioValue(data.value),
      layout: 'horizontal',
    }, [
      h(Radio, { key: 'r1', value: 'option1', label: 'Option 1' }),
      h(Radio, { key: 'r2', value: 'option2', label: 'Option 2' }),
      h(Radio, { key: 'r3', value: 'option3', label: 'Option 3' }),
    ]),
  ]);
}

// ============================================
// 数据展示演示
// ============================================

function DataDisplayDemo() {
  return h(DemoSection, { title: 'Data Display' }, [
    // Text
    h(Caption, { key: 'label1', style: { display: 'block', marginBottom: spacing.s } }, 'Typography'),
    h('div', { key: 'text', style: { marginBottom: spacing.m } }, [
      h(Title, { key: 't1' }, 'Title Text'),
      h(Subtitle, { key: 't2' }, 'Subtitle Text'),
      h(Body, { key: 't3' }, 'Body text for regular content.'),
      h(Caption, { key: 't4' }, 'Caption text for small details'),
    ]),

    // Badge
    h(Caption, { key: 'label2', style: { display: 'block', marginBottom: spacing.s } }, 'Badges'),
    h(DemoRow, { key: 'badges' }, [
      h(Badge, { key: 'brand', color: 'brand' }, 'Brand'),
      h(Badge, { key: 'danger', color: 'danger' }, 'Danger'),
      h(Badge, { key: 'success', color: 'success' }, 'Success'),
      h(Badge, { key: 'warning', color: 'warning' }, 'Warning'),
      h(CounterBadge, { key: 'counter', count: 42 }),
      h(CounterBadge, { key: 'overflow', count: 150, overflowCount: 99 }),
      h(PresenceBadge, { key: 'available', status: 'available', size: 'medium' }),
      h(PresenceBadge, { key: 'busy', status: 'busy', size: 'medium' }),
      h(PresenceBadge, { key: 'away', status: 'away', size: 'medium' }),
    ]),

    // Avatar
    h(Caption, { key: 'label3', style: { display: 'block', marginBottom: spacing.s } }, 'Avatars'),
    h(DemoRow, { key: 'avatars' }, [
      h(Avatar, { key: 'a1', name: 'John Doe', size: 32 }),
      h(Avatar, { key: 'a2', name: 'Jane Smith', size: 40 }),
      h(Avatar, { key: 'a3', name: 'Bob Wilson', size: 48, color: 'brand' }),
      h(Avatar, { key: 'a4', name: 'Alice Brown', size: 56, active: 'active' }),
      h(Avatar, { key: 'a5', size: 40, color: 'neutral' }), // 默认图标
    ]),

    // Avatar Group
    h(Caption, { key: 'label4', style: { display: 'block', marginBottom: spacing.s } }, 'Avatar Group'),
    h(DemoRow, { key: 'avatarGroup' }, [
      h(AvatarGroup, { key: 'group', layout: 'stack', maxAvatars: 4 }, [
        h(Avatar, { key: 'g1', name: 'User 1', size: 32 }),
        h(Avatar, { key: 'g2', name: 'User 2', size: 32 }),
        h(Avatar, { key: 'g3', name: 'User 3', size: 32 }),
        h(Avatar, { key: 'g4', name: 'User 4', size: 32 }),
        h(Avatar, { key: 'g5', name: 'User 5', size: 32 }),
        h(Avatar, { key: 'g6', name: 'User 6', size: 32 }),
      ]),
    ]),

    // Divider
    h(Caption, { key: 'label5', style: { display: 'block', marginBottom: spacing.s } }, 'Dividers'),
    h('div', { key: 'dividers', style: { marginBottom: spacing.m } }, [
      h(Divider, { key: 'd1' }),
      h('div', { key: 'spacer1', style: { height: spacing.m } }),
      h(Divider, { key: 'd2', appearance: 'brand' }, 'Brand Divider'),
      h('div', { key: 'spacer2', style: { height: spacing.m } }),
      h(Divider, { key: 'd3', alignContent: 'start' }, 'Start'),
    ]),
  ]);
}

// ============================================
// 卡片演示
// ============================================

function CardDemo() {
  const [selected, setSelected] = useState(false);

  return h(DemoSection, { title: 'Cards' }, [
    h(DemoRow, { key: 'cards' }, [
      // Filled Card
      h(Card, {
        key: 'filled',
        appearance: 'filled',
        style: { width: '280px' },
      }, [
        h(CardHeader, {
          key: 'header',
          header: 'Filled Card',
          description: 'This is a filled card',
        }),
        h(Body, { key: 'body' }, 'Card content goes here. This is some example text to show how the card looks with content.'),
        h(CardFooter, { key: 'footer' }, [
          h(Button, { key: 'btn1', appearance: 'primary', size: 'small' }, 'Action'),
          h(Button, { key: 'btn2', appearance: 'secondary', size: 'small' }, 'Cancel'),
        ]),
      ]),

      // Outline Card
      h(Card, {
        key: 'outline',
        appearance: 'outline',
        style: { width: '280px' },
      }, [
        h(CardHeader, {
          key: 'header',
          header: 'Outline Card',
          description: 'With outline appearance',
        }),
        h(Body, { key: 'body' }, 'Another card example with outline style.'),
      ]),

      // Selectable Card
      h(Card, {
        key: 'selectable',
        appearance: 'filled',
        selectable: true,
        selected: selected,
        onSelectionChange: (e, data) => setSelected(data.selected),
        style: { width: '280px' },
      }, [
        h(CardHeader, {
          key: 'header',
          header: 'Selectable Card',
          description: selected ? 'Selected!' : 'Click to select',
        }),
        h(Body, { key: 'body' }, 'This card can be selected.'),
      ]),
    ]),
  ]);
}

// ============================================
// 反馈组件演示
// ============================================

function FeedbackDemo() {
  return h(DemoSection, { title: 'Feedback' }, [
    // Spinner
    h(Caption, { key: 'label1', style: { display: 'block', marginBottom: spacing.s } }, 'Spinners'),
    h(DemoRow, { key: 'spinners' }, [
      h(Spinner, { key: 's1', size: 'tiny' }),
      h(Spinner, { key: 's2', size: 'small' }),
      h(Spinner, { key: 's3', size: 'medium' }),
      h(Spinner, { key: 's4', size: 'large', label: 'Loading...' }),
    ]),

    // Links
    h(Caption, { key: 'label2', style: { display: 'block', marginBottom: spacing.s } }, 'Links'),
    h(DemoRow, { key: 'links' }, [
      h(Link, { key: 'l1', href: '#' }, 'Default Link'),
      h(Link, { key: 'l2', href: '#', appearance: 'subtle' }, 'Subtle Link'),
      h(Link, { key: 'l3', disabled: true }, 'Disabled Link'),
      h(Text, { key: 't1' }, [
        'Inline ',
        h(Link, { key: 'inline', href: '#', inline: true }, 'link'),
        ' in text.',
      ]),
    ]),
  ]);
}

// ============================================
// 主应用
// ============================================

function App() {
  return h('div', {
    style: {
      padding: spacing.xxl,
      backgroundColor: colors.colorNeutralBackground2,
      minHeight: '100vh',
      boxSizing: 'border-box',
    }
  }, [
    // 标题
    h('div', {
      key: 'header',
      style: {
        marginBottom: spacing.xxl,
        textAlign: 'center',
      }
    }, [
      h(Title, { key: 'title', size: 700 }, 'MBink Fluent Design'),
      h(Body, { 
        key: 'subtitle',
        style: { color: colors.colorNeutralForeground2 }
      }, 'A lightweight Fluent UI component library for MBink'),
    ]),

    // 演示区块
    h(ButtonDemo, { key: 'buttons' }),
    h(InputDemo, { key: 'inputs' }),
    h(DataDisplayDemo, { key: 'data' }),
    h(CardDemo, { key: 'cards' }),
    h(FeedbackDemo, { key: 'feedback' }),
  ]);
}

// 渲染应用
console.log('Starting render...');
render(h(App), document.body);
console.log('Render complete');
console.log('document.body children count:', document.body.childNodes?.length);
console.log('document.body.innerHTML length:', document.body.innerHTML?.length);
