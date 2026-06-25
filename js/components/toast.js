/**
 * MBlink Toast 组件
 *
 * 使用方式：
 * Toast.show('Message')
 * Toast.success('Success!')
 * Toast.error('Error!')
 * Toast.warning('Warning!')
 * Toast.loading('Loading...')
 */

import { h, render } from 'preact';
import { colors, radius, shadow, transition } from './theme.js';

// Toast 容器
let container = null;
let toasts = [];
let toastId = 0;

function getContainer() {
  if (!container) {
    container = document.createElement('div');
    container.id = 'mblink-toast-container';
    Object.assign(container.style, {
      position: 'fixed',
      top: '20px',
      left: '50%',
      transform: 'translateX(-50%)',
      zIndex: '9999',
      display: 'flex',
      flexDirection: 'column',
      alignItems: 'center',
      gap: '8px',
      pointerEvents: 'none',
    });
    document.body.appendChild(container);
  }
  return container;
}

function renderToasts() {
  const container = getContainer();
  render(
    h(
      'div',
      {
        style: {
          display: 'flex',
          flexDirection: 'column',
          alignItems: 'center',
          gap: '8px',
          pointerEvents: 'none',
        },
      },
      toasts.map((toast) => h(ToastItem, { key: toast.id, ...toast }))
    ),
    container
  );
}

function ToastItem({ content, type, onClose }) {
  const iconMap = {
    success: '✓',
    error: '✕',
    warning: '⚠',
    info: 'ℹ',
    loading: '◌',
  };

  const colorMap = {
    success: colors.success,
    error: colors.error,
    warning: colors.warning,
    info: colors.info,
    loading: colors.primary,
  };

  const toastStyle = {
    display: 'flex',
    alignItems: 'center',
    gap: '8px',
    padding: '10px 16px',
    backgroundColor: colors.bg,
    borderRadius: radius.md,
    boxShadow: shadow.lg,
    fontSize: '14px',
    color: colors.text,
    pointerEvents: 'auto',
    animation: type === 'loading' ? 'none' : undefined,
  };

  const iconStyle = {
    fontSize: '16px',
    color: colorMap[type] || colors.text,
    animation: type === 'loading' ? 'mblink-spin 1s linear infinite' : 'none',
  };

  return h(
    'div',
    { style: toastStyle },
    type && h('span', { style: iconStyle }, iconMap[type]),
    h('span', {}, content)
  );
}

function show(options) {
  const config =
    typeof options === 'string' ? { content: options } : options;

  const {
    content,
    type,
    duration = type === 'loading' ? 0 : 3000,
    onClose,
  } = config;

  const id = ++toastId;
  const toast = { id, content, type, onClose };

  toasts.push(toast);
  renderToasts();

  const hide = () => {
    toasts = toasts.filter((t) => t.id !== id);
    renderToasts();
    onClose?.();
  };

  if (duration > 0) {
    setTimeout(hide, duration);
  }

  return hide;
}

export const Toast = {
  show: (options) => show(options),

  success: (content, duration) =>
    show({ content, type: 'success', duration }),

  error: (content, duration) =>
    show({ content, type: 'error', duration }),

  warning: (content, duration) =>
    show({ content, type: 'warning', duration }),

  info: (content, duration) =>
    show({ content, type: 'info', duration }),

  loading: (content) =>
    show({ content, type: 'loading', duration: 0 }),
};

export default Toast;
