/**
 * LightUI Modal 弹窗组件测试 - ES Module 版本
 */

import { h, render } from 'preact';
import { useState } from 'preact/hooks';

// 导入组件
import { Button } from '../../js/components/button.js';
import { Modal } from '../../js/components/modal.js';
import { Row, Column } from '../../js/components/layout.js';
import { Text } from '../../js/components/text.js';
import { Toast } from '../../js/components/toast.js';

function ModalTestApp() {
  const [modalVisible, setModalVisible] = useState(false);
  const [loading, setLoading] = useState(false);

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
    h(Text, { size: '2xl', weight: 'bold', key: 'title' }, 'Modal Test'),

    // Buttons
    h(Row, { gap: 8, key: 'buttons' }, [
      h(Button, { onClick: () => setModalVisible(true) }, 'Open Modal'),
      h(Button, { variant: 'secondary', onClick: () => Toast.success('Success!') }, 'Toast'),
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

render(h(ModalTestApp), document.body);

