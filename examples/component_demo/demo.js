/**
 * LightUI 组件库演示
 */

const { h, render } = preact;
const { useState } = preactHooks;

// 导入组件（实际使用时从 components/index.js 导入）
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

function App() {
  const [inputValue, setInputValue] = useState('');
  const [textareaValue, setTextareaValue] = useState('');
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
    { value: 4, label: 'Option 4 (disabled)', disabled: true },
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

  return h(Column, { gap: 32, style: { padding: '24px', maxWidth: '800px', margin: '0 auto' } }, [
    // Title
    h(Text.Title, { key: 'title' }, 'LightUI Component Demo'),

    // Buttons Section
    h(Card, { key: 'buttons', title: 'Buttons' }, [
      h(Column, { gap: 16 }, [
        h(Text.Caption, {}, 'Variants'),
        h(Row, { gap: 12, wrap: true }, [
          h(Button, { variant: 'primary' }, 'Primary'),
          h(Button, { variant: 'secondary' }, 'Secondary'),
          h(Button, { variant: 'outline' }, 'Outline'),
          h(Button, { variant: 'ghost' }, 'Ghost'),
          h(Button, { variant: 'link' }, 'Link'),
          h(Button, { variant: 'danger' }, 'Danger'),
        ]),

        h(Text.Caption, {}, 'Sizes'),
        h(Row, { gap: 12, align: 'center' }, [
          h(Button, { size: 'sm' }, 'Small'),
          h(Button, { size: 'md' }, 'Medium'),
          h(Button, { size: 'lg' }, 'Large'),
        ]),

        h(Text.Caption, {}, 'States'),
        h(Row, { gap: 12 }, [
          h(Button, { disabled: true }, 'Disabled'),
          h(Button, { loading: true }, 'Loading'),
        ]),
      ]),
    ]),

    // Inputs Section
    h(Card, { key: 'inputs', title: 'Inputs' }, [
      h(Column, { gap: 16 }, [
        h(Row, { gap: 16 }, [
          h(Column, { gap: 8, style: { flex: 1 } }, [
            h(Text.Caption, {}, 'Text Input'),
            h(Input, {
              placeholder: 'Enter text...',
              value: inputValue,
              onChange: setInputValue,
            }),
          ]),
          h(Column, { gap: 8, style: { flex: 1 } }, [
            h(Text.Caption, {}, 'With Prefix/Suffix'),
            h(Input, {
              placeholder: 'Amount',
              prefix: '$',
              suffix: '.00',
            }),
          ]),
        ]),

        h(Row, { gap: 16 }, [
          h(Column, { gap: 8, style: { flex: 1 } }, [
            h(Text.Caption, {}, 'Password'),
            h(Input, { type: 'password', placeholder: 'Password' }),
          ]),
          h(Column, { gap: 8, style: { flex: 1 } }, [
            h(Text.Caption, {}, 'Error Status'),
            h(Input, { placeholder: 'Error', status: 'error' }),
          ]),
        ]),

        h(Text.Caption, {}, 'Textarea'),
        h(Textarea, {
          placeholder: 'Enter description...',
          value: textareaValue,
          onChange: setTextareaValue,
          showCount: true,
          maxLength: 200,
        }),
      ]),
    ]),

    // Select Section
    h(Card, { key: 'select', title: 'Select' }, [
      h(Column, { gap: 16 }, [
        h(Row, { gap: 16 }, [
          h(Column, { gap: 8, style: { flex: 1 } }, [
            h(Text.Caption, {}, 'Basic Select'),
            h(Select, {
              options: selectOptions,
              value: selectValue,
              onChange: setSelectValue,
              placeholder: 'Select an option',
            }),
          ]),
          h(Column, { gap: 8, style: { flex: 1 } }, [
            h(Text.Caption, {}, 'Clearable'),
            h(Select, {
              options: selectOptions,
              clearable: true,
              placeholder: 'Clearable select',
            }),
          ]),
        ]),
      ]),
    ]),

    // Checkbox & Radio Section
    h(Card, { key: 'checkbox-radio', title: 'Checkbox & Radio' }, [
      h(Row, { gap: 32 }, [
        h(Column, { gap: 12, style: { flex: 1 } }, [
          h(Text.Caption, {}, 'Checkbox Group'),
          h(Checkbox.Group, {
            options: checkboxOptions,
            value: checkboxValue,
            onChange: setCheckboxValue,
            direction: 'vertical',
          }),
          h(Text, { size: 'sm', color: colors.textSecondary }, 
            `Selected: ${checkboxValue.join(', ') || 'none'}`),
        ]),
        h(Column, { gap: 12, style: { flex: 1 } }, [
          h(Text.Caption, {}, 'Radio Group'),
          h(Radio.Group, {
            options: radioOptions,
            value: radioValue,
            onChange: setRadioValue,
            direction: 'vertical',
          }),
          h(Text, { size: 'sm', color: colors.textSecondary }, 
            `Selected: ${radioValue || 'none'}`),
        ]),
      ]),
    ]),

    // Switch Section
    h(Card, { key: 'switch', title: 'Switch' }, [
      h(Row, { gap: 24, align: 'center' }, [
        h(Switch, {
          checked: switchValue,
          onChange: setSwitchValue,
        }),
        h(Switch, {
          checked: switchValue,
          onChange: setSwitchValue,
          size: 'sm',
        }),
        h(Switch, {
          checked: switchValue,
          onChange: setSwitchValue,
          checkedText: 'ON',
          uncheckedText: 'OFF',
        }),
        h(Switch, { disabled: true }),
      ]),
    ]),

    // Layout Section
    h(Card, { key: 'layout', title: 'Layout' }, [
      h(Column, { gap: 16 }, [
        h(Text.Caption, {}, 'Grid (3 columns)'),
        h(Grid, { columns: 3, gap: 12 }, [
          h('div', { style: { padding: '16px', backgroundColor: colors.bgSecondary, textAlign: 'center' } }, '1'),
          h('div', { style: { padding: '16px', backgroundColor: colors.bgSecondary, textAlign: 'center' } }, '2'),
          h('div', { style: { padding: '16px', backgroundColor: colors.bgSecondary, textAlign: 'center' } }, '3'),
        ]),

        h(Text.Caption, {}, 'Row with Spacer'),
        h(Row, { style: { padding: '12px', backgroundColor: colors.bgSecondary } }, [
          h(Button, { size: 'sm' }, 'Left'),
          h(Spacer),
          h(Button, { size: 'sm' }, 'Right'),
        ]),
      ]),
    ]),

    // Modal & Toast Section
    h(Card, { key: 'feedback', title: 'Modal & Toast' }, [
      h(Row, { gap: 12 }, [
        h(Button, { onClick: () => setModalVisible(true) }, 'Open Modal'),
        h(Button, { variant: 'secondary', onClick: () => Toast.success('Success message!') }, 'Success Toast'),
        h(Button, { variant: 'secondary', onClick: () => Toast.error('Error message!') }, 'Error Toast'),
        h(Button, { variant: 'secondary', onClick: () => Toast.warning('Warning message!') }, 'Warning Toast'),
      ]),
    ]),

    // Modal
    h(Modal, {
      key: 'modal',
      visible: modalVisible,
      title: 'Confirm Action',
      onClose: () => setModalVisible(false),
      onOk: handleSubmit,
      okLoading: loading,
    }, [
      h(Text, {}, 'Are you sure you want to proceed with this action?'),
    ]),
  ]);
}

render(h(App), document.body);
