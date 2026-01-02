/**
 * LightUI Modal 组件
 *
 * Props:
 * - visible: boolean
 * - title: string | VNode
 * - width: number | string
 * - closable: boolean
 * - maskClosable: boolean
 * - footer: VNode | null
 * - onClose: () => void
 * - onOk: () => void
 * - okText: string
 * - cancelText: string
 * - okLoading: boolean
 */

import { h } from 'preact';
import { useEffect } from 'preact/hooks';
import { colors, radius, shadow, transition } from './theme.js';
import { Button } from './button.js';
import { mergeStyles } from './utils.js';

export function Modal(props) {
  const {
    visible = false,
    title,
    width = 480,
    closable = true,
    maskClosable = true,
    footer,
    onClose,
    onOk,
    okText = 'OK',
    cancelText = 'Cancel',
    okLoading = false,
    children,
    style,
    ...rest
  } = props;

  // ESC 关闭
  useEffect(() => {
    if (!visible) return;
    const handleKeyDown = (e) => {
      if (e.key === 'Escape' && closable) {
        onClose?.();
      }
    };
    // 安全检查：确保 addEventListener 存在
    if (typeof document.addEventListener === 'function') {
      document.addEventListener('keydown', handleKeyDown);
      return () => document.removeEventListener('keydown', handleKeyDown);
    }
  }, [visible, closable, onClose]);

  // 禁止背景滚动
  useEffect(() => {
    // if (visible) {
    //   document.body.style.overflow = 'hidden';
    // } else {
    //   document.body.style.overflow = '';
    // }
    // return () => {
    //   document.body.style.overflow = '';
    // };
  }, [visible]);

  if (!visible) return null;

  const maskStyle = {
    position: 'fixed',
    top: 0,
    left: 0,
    right: 0,
    bottom: 0,
    backgroundColor: colors.mask,
    display: 'flex',
    alignItems: 'center',
    justifyContent: 'center',
    zIndex: 1000,
  };

  const modalStyle = {
    width: typeof width === 'number' ? `${width}px` : width,
    maxWidth: '90vw',
    maxHeight: '80vh',
    backgroundColor: colors.bg,
    borderRadius: radius.lg,
    boxShadow: shadow.xl,
    display: 'flex',
    flexDirection: 'column',
    overflow: 'hidden',
  };

  const headerStyle = {
    display: 'flex',
    alignItems: 'center',
    justifyContent: 'space-between',
    padding: '16px 20px',
    borderBottom: `1px solid ${colors.border}`,
  };

  const titleStyle = {
    fontSize: '16px',
    fontWeight: '600',
    color: colors.text,
    margin: 0,
  };

  const closeStyle = {
    fontSize: '20px',
    color: colors.textTertiary,
    cursor: 'pointer',
    lineHeight: 1,
    padding: '4px',
  };

  const bodyStyle = {
    padding: '20px',
    flex: 1,
    overflowY: 'auto',
  };

  const footerStyle = {
    display: 'flex',
    justifyContent: 'flex-end',
    gap: '12px',
    padding: '16px 20px',
    borderTop: `1px solid ${colors.border}`,
  };

  const handleMaskClick = (e) => {
    if (maskClosable && e.target === e.currentTarget) {
      onClose?.();
    }
  };

  const handleModalClick = (e) => {
    e.stopPropagation();
  };

  // 默认 footer
  const defaultFooter = h('div', { style: footerStyle }, [
    h(Button, { key: 'cancel', variant: 'secondary', onClick: onClose }, cancelText),
    h(
      Button,
      { key: 'ok', variant: 'primary', loading: okLoading, onClick: onOk },
      okText
    ),
  ]);

  return h(
    'div',
    { style: maskStyle, onClick: handleMaskClick },
    h(
      'div',
      { style: mergeStyles(modalStyle, style), onClick: handleModalClick, ...rest },
      // Header
      (title || closable) &&
        h(
          'div',
          { style: headerStyle },
          title &&
            (typeof title === 'string' ? h('div', { style: titleStyle }, title) : title),
          closable &&
            h(
              'span',
              {
                style: closeStyle,
                onClick: onClose,
                onMouseEnter: (e) => (e.target.style.color = colors.text),
                onMouseLeave: (e) => (e.target.style.color = colors.textTertiary),
              },
              '×'
            )
        ),
      // Body
      h('div', { style: bodyStyle }, children),
      // Footer
      footer !== null && (footer || defaultFooter)
    )
  );
}

// 快捷方法（需要全局 Toast 容器支持）
Modal.confirm = function (options) {
  console.warn('Modal.confirm requires global modal container');
};

Modal.alert = function (options) {
  console.warn('Modal.alert requires global modal container');
};

export default Modal;
