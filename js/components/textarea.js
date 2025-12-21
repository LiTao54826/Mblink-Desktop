/**
 * LightUI Textarea 组件
 *
 * Props:
 * - value: string
 * - placeholder: string
 * - rows: number
 * - maxLength: number
 * - showCount: boolean
 * - disabled: boolean
 * - readonly: boolean
 * - status: 'success' | 'warning' | 'error'
 * - onChange: (value, e) => void
 * - onFocus: (e) => void
 * - onBlur: (e) => void
 */

import { h } from 'preact';
import { useState } from 'preact/hooks';
import { colors, radius, fontSize, transition } from './theme.js';
import { mergeStyles } from './utils.js';

const statusColors = {
  success: colors.success,
  warning: colors.warning,
  error: colors.error,
};

export function Textarea(props) {
  const {
    value,
    defaultValue = '',
    placeholder,
    rows = 3,
    maxLength,
    showCount = false,
    disabled = false,
    readonly = false,
    status,
    onChange,
    onFocus,
    onBlur,
    style,
    ...rest
  } = props;

  const [focused, setFocused] = useState(false);
  const [internalValue, setInternalValue] = useState(defaultValue);

  const isControlled = value !== undefined;
  const currentValue = isControlled ? value : internalValue;

  const borderColor = status
    ? statusColors[status]
    : focused
      ? colors.borderFocus
      : colors.border;

  const wrapperStyle = {
    display: 'flex',
    flexDirection: 'column',
    width: '100%',
  };

  const textareaStyle = {
    width: '100%',
    padding: '8px 12px',
    fontSize: fontSize.sm,
    lineHeight: '1.5',
    color: disabled ? colors.textDisabled : colors.text,
    backgroundColor: disabled ? colors.bgSecondary : colors.bg,
    border: `1px solid ${borderColor}`,
    borderRadius: radius.md,
    outline: 'none',
    resize: 'vertical',
    transition: `border-color ${transition.fast}`,
    boxSizing: 'border-box',
    cursor: disabled ? 'not-allowed' : 'text',
  };

  const countStyle = {
    alignSelf: 'flex-end',
    marginTop: '4px',
    fontSize: fontSize.xs,
    color: colors.textTertiary,
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

  return h(
    'div',
    { style: mergeStyles(wrapperStyle, style) },
    h('textarea', {
      value: currentValue,
      placeholder,
      rows,
      maxLength,
      disabled,
      readOnly: readonly,
      style: textareaStyle,
      onChange: handleChange,
      onFocus: handleFocus,
      onBlur: handleBlur,
      ...rest,
    }),
    showCount &&
      h(
        'span',
        { style: countStyle },
        maxLength
          ? `${currentValue.length} / ${maxLength}`
          : `${currentValue.length}`
      )
  );
}

export default Textarea;
