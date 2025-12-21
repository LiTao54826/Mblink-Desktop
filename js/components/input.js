/**
 * LightUI Input 组件
 *
 * Props:
 * - type: 'text' | 'password' | 'number' | 'email' | 'tel' | 'url'
 * - size: 'sm' | 'md' | 'lg'
 * - value: string | number
 * - placeholder: string
 * - disabled: boolean
 * - readonly: boolean
 * - maxLength: number
 * - prefix: VNode | string
 * - suffix: VNode | string
 * - status: 'success' | 'warning' | 'error'
 * - onChange: (value, e) => void
 * - onFocus: (e) => void
 * - onBlur: (e) => void
 * - onEnter: (value, e) => void
 */

import { h } from 'preact';
import { useState } from 'preact/hooks';
import { colors, radius, sizes, transition } from './theme.js';
import { mergeStyles } from './utils.js';

const statusColors = {
  success: colors.success,
  warning: colors.warning,
  error: colors.error,
};

export function Input(props) {
  const {
    type = 'text',
    size = 'md',
    value,
    defaultValue = '',
    placeholder,
    disabled = false,
    readonly = false,
    maxLength,
    prefix,
    suffix,
    status,
    onChange,
    onFocus,
    onBlur,
    onEnter,
    style,
    ...rest
  } = props;

  const [focused, setFocused] = useState(false);
  const [internalValue, setInternalValue] = useState(defaultValue);

  const isControlled = value !== undefined;
  const currentValue = isControlled ? value : internalValue;

  const sizeConfig = sizes[size] || sizes.md;
  const borderColor = status
    ? statusColors[status]
    : focused
      ? colors.borderFocus
      : colors.border;

  // 直接在 input 上应用样式（类似 Textarea）
  const inputStyle = {
    width: '100%',
    height: sizeConfig.height,
    padding: '0 12px',
    fontSize: sizeConfig.fontSize,
    lineHeight: sizeConfig.height,
    backgroundColor: disabled ? colors.bgSecondary : colors.bg,
    border: `1px solid ${borderColor}`,
    borderRadius: radius.md,
    transition: `border-color ${transition.fast}`,
    boxSizing: 'border-box',
    outline: 'none',
    color: disabled ? colors.textDisabled : colors.text,
    cursor: disabled ? 'not-allowed' : 'text',
  };

  const handleChange = (e) => {
    const newValue = e.target.value;
    if (!isControlled) {
      setInternalValue(newValue);
    }
    onChange?.(newValue, e);
  };

  const handleFocus = (e) => {
    setFocused(true);
    onFocus?.(e);
  };

  const handleBlur = (e) => {
    setFocused(false);
    onBlur?.(e);
  };

  const handleKeyDown = (e) => {
    if (e.key === 'Enter') {
      onEnter?.(currentValue, e);
    }
  };

  return h('input', {
    type,
    value: currentValue,
    placeholder,
    disabled,
    readOnly: readonly,
    maxLength,
    style: mergeStyles(inputStyle, style),
    onInput: handleChange,
    onFocus: handleFocus,
    onBlur: handleBlur,
    onKeyDown: handleKeyDown,
    ...rest,
  });
}

export default Input;
