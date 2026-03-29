/**
 * MBink 组件库演示 - ES Module 版本
 */

import { h, render } from 'preact';
import { useState } from 'preact/hooks';

// 导入组件
import { Button } from '../../js/components/button.js';
import { Input } from '../../js/components/input.js';
import { Textarea } from '../../js/components/textarea.js';
import { Select } from '../../js/components/select.js';
import { Checkbox } from '../../js/components/checkbox.js';
import { Radio } from '../../js/components/radio.js';
import { Switch } from '../../js/components/switch.js';
import { Row, Column, Grid, Spacer } from '../../js/components/layout.js';
import { Text } from '../../js/components/text.js';
import { Card } from '../../js/components/card.js';
import { Modal } from '../../js/components/modal.js';
import { Toast } from '../../js/components/toast.js';
import { colors } from '../../js/components/theme.js';

let toastCounter = 0;

function App() {
  const [inputValue, setInputValue] = useState('');
  const [selectValue, setSelectValue] = useState(null);
  const [checkboxValue, setCheckboxValue] = useState([]);
  const [radioValue, setRadioValue] = useState(null);
  const [switchValue, setSwitchValue] = useState(false);
  const [modalVisible, setModalVisible] = useState(false);
  const [loading, setLoading] = useState(false);

  const selectOptions = [
    { value: 1, label: 'Option 1' },
    { value: 2, label: 'Option 2' },
    { value: 3, label: 'Option 3' },
  ];

  const checkboxOptions = [
    { value: 'apple', label: 'Apple' },
    { value: 'banana', label: 'Banana' },
    { value: 'orange', label: 'Orange' },
  ];

  const radioOptions = [
    { value: 'small', label: 'Small' },
    { value: 'medium', label: 'Medium' },
    { value: 'large', label: 'Large' },
  ];

  const handleSubmit = () => {
    setLoading(true);
    setTimeout(() => {
      setLoading(false);
      Toast.success('Submitted successfully!');
      setModalVisible(false);
    }, 1500);
  };

  return h(Column, { gap: 24, style: { padding: '20px', maxWidth: '800px' } }, [
    // Title
    h(Text, { size: '2xl', weight: 'bold', key: 'title' }, 'MBink Components'),

    // Buttons
    h(Card, { key: 'buttons', title: 'Buttons' }, [
      h(Column, { gap: 12 }, [
        h(Row, { gap: 8, wrap: true }, [
          h(Button, { variant: 'primary' }, 'Primary'),
          h(Button, { variant: 'secondary' }, 'Secondary'),
          h(Button, { variant: 'outline' }, 'Outline'),
          h(Button, { variant: 'ghost' }, 'Ghost'),
          h(Button, { variant: 'danger' }, 'Danger'),
        ]),
        h(Row, { gap: 8, align: 'center' }, [
          h(Button, { size: 'sm' }, 'Small'),
          h(Button, { size: 'md' }, 'Medium'),
          h(Button, { size: 'lg' }, 'Large'),
        ]),
        h(Row, { gap: 8 }, [
          h(Button, { disabled: true }, 'Disabled'),
          h(Button, { loading: true }, 'Loading'),
        ]),
      ]),
    ]),

    // Inputs
    h(Card, { key: 'inputs', title: 'Inputs' }, [
      h(Column, { gap: 12 }, [
        h(Input, {
          placeholder: 'Enter text...',
          value: inputValue,
          onChange: setInputValue,
        }),
        h(Row, { gap: 12 }, [
          h(Input, { placeholder: 'With prefix', prefix: '$', style: { flex: 1 } }),
          h(Input, { type: 'password', placeholder: 'Password', style: { flex: 1 } }),
        ]),
        h(Textarea, {
          placeholder: 'Enter description...',
          showCount: true,
          maxLength: 200,
        }),
      ]),
    ]),

    // Select
    h(Card, { key: 'select', title: 'Select' }, [
      h(Row, { gap: 12 }, [
        h(Select, {
          options: selectOptions,
          value: selectValue,
          onChange: setSelectValue,
          placeholder: 'Select option',
          style: { flex: 1 },
        }),
        h(Select, {
          options: selectOptions,
          clearable: true,
          placeholder: 'Clearable',
          style: { flex: 1 },
        }),
      ]),
    ]),

    // Checkbox & Radio
    h(Card, { key: 'checkbox-radio', title: 'Checkbox & Radio' }, [
      h(Row, { gap: 32 }, [
        h(Column, { gap: 8 }, [
          h(Text, { size: 'sm', color: colors.textSecondary }, 'Checkbox'),
          h(Checkbox.Group, {
            options: checkboxOptions,
            value: checkboxValue,
            onChange: setCheckboxValue,
            direction: 'vertical',
          }),
        ]),
        h(Column, { gap: 8 }, [
          h(Text, { size: 'sm', color: colors.textSecondary }, 'Radio'),
          h(Radio.Group, {
            options: radioOptions,
            value: radioValue,
            onChange: setRadioValue,
            direction: 'vertical',
          }),
        ]),
      ]),
    ]),

    // Switch
    h(Card, { key: 'switch', title: 'Switch' }, [
      h(Row, { gap: 16, align: 'center' }, [
        h(Switch, { checked: switchValue, onChange: setSwitchValue }),
        h(Switch, { checked: switchValue, onChange: setSwitchValue, size: 'sm' }),
        h(Switch, { disabled: true }),
      ]),
    ]),

    // Layout
    h(Card, { key: 'layout', title: 'Layout' }, [
      h(Column, { gap: 12 }, [
        h(Text, { size: 'sm', color: colors.textSecondary }, 'Grid (3 columns)'),
        h(Grid, { columns: 3, gap: 8 }, [
          h('div', { style: { padding: '12px', background: colors.bgSecondary, textAlign: 'center' } }, '1'),
          h('div', { style: { padding: '12px', background: colors.bgSecondary, textAlign: 'center' } }, '2'),
          h('div', { style: { padding: '12px', background: colors.bgSecondary, textAlign: 'center' } }, '3'),
        ]),
        h(Text, { size: 'sm', color: colors.textSecondary }, 'Row with Spacer'),
        h(Row, { style: { padding: '8px', background: colors.bgSecondary } }, [
          h(Button, { size: 'sm' }, 'Left'),
          h(Spacer),
          h(Button, { size: 'sm' }, 'Right'),
        ]),
      ]),
    ]),

    // Modal & Toast
    h(Card, { key: 'feedback', title: 'Feedback' }, [
      h(Row, { gap: 8 }, [
        h(Button, { onClick: () => setModalVisible(true) }, 'Open Modal'),
        h(Button, { variant: 'secondary', onClick: () => {
          toastCounter++;
          Toast.success('Toast #' + toastCounter);
        } }, 'Toast'),
      ]),
    ]),

    // Modal
    h(Modal, {
      key: 'modal',
      visible: modalVisible,
      title: 'Confirm',
      onClose: () => setModalVisible(false),
      onOk: handleSubmit,
      okLoading: loading,
    }, [
      h(Text, {}, 'Are you sure you want to proceed?'),
    ]),
  ]);
}

render(h(App), document.body);
