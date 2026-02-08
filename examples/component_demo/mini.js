/**
 * LightUI 组件库演示 - ES Module 版本
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
  const [modalVisible, setModalVisible] = useState(true);
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
  return h(Modal, {
      key: 'modal',
      visible: modalVisible,
      title: 'Confirm',
      onClose: () => setModalVisible(false),
      onOk: handleSubmit,
      okLoading: loading,
    }, [
      h(Text, {}, 'Are you sure you want to proceed?'),
    ])
}

render(h(App), document.body);
