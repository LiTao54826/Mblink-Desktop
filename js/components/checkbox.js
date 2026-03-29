/**
 * MBink Checkbox 组件
 *
 * Props:
 * - checked: boolean
 * - indeterminate: boolean
 * - disabled: boolean
 * - onChange: (checked, e) => void
 * - children: label 文字
 */

import { h } from 'preact';
import { useState, useEffect, useRef } from 'preact/hooks';
import { colors, radius, transition } from './theme.js';
import { mergeStyles } from './utils.js';

export function Checkbox(props) {
  const {
    checked,
    defaultChecked = false,
    indeterminate = false,
    disabled = false,
    onChange,
    children,
    style,
    ...rest
  } = props;

  const [internalChecked, setInternalChecked] = useState(defaultChecked);
  const inputRef = useRef(null);

  const isControlled = checked !== undefined;
  const isChecked = isControlled ? checked : internalChecked;

  useEffect(() => {
    if (inputRef.current) {
      inputRef.current.indeterminate = indeterminate;
    }
  }, [indeterminate]);

  const wrapperStyle = {
    display: 'inline-flex',
    alignItems: 'center',
    gap: '8px',
    cursor: disabled ? 'not-allowed' : 'pointer',
    opacity: disabled ? 0.5 : 1,
    userSelect: 'none',
  };

  const boxStyle = {
    position: 'relative',
    width: '16px',
    height: '16px',
    borderRadius: radius.sm,
    border: `1px solid ${isChecked || indeterminate ? colors.primary : colors.border}`,
    backgroundColor:
      isChecked || indeterminate ? colors.primary : colors.bg,
    transition: `all ${transition.fast}`,
    display: 'flex',
    alignItems: 'center',
    justifyContent: 'center',
  };

  const checkmarkStyle = {
    color: colors.textInverse,
    fontSize: '12px',
    lineHeight: 1,
  };

  const labelStyle = {
    fontSize: '14px',
    color: disabled ? colors.textDisabled : colors.text,
  };

  const handleClick = (e) => {
    if (disabled) return;
    e.preventDefault();
    const newChecked = !isChecked;
    if (!isControlled) {
      setInternalChecked(newChecked);
    }
    onChange?.(newChecked, e);
  };

  // 勾选图标
  const checkIcon = isChecked
    ? '✓'
    : indeterminate
      ? '−'
      : null;

  return h(
    'div',
    { style: mergeStyles(wrapperStyle, style), onClick: handleClick },
    h('span', { style: boxStyle }, checkIcon && h('span', { style: checkmarkStyle }, checkIcon)),
    children && h('span', { style: labelStyle }, children)
  );
}

/**
 * Checkbox.Group 组件
 *
 * Props:
 * - value: Array
 * - options: Array<{ value, label, disabled? }>
 * - direction: 'horizontal' | 'vertical'
 * - onChange: (values) => void
 */
Checkbox.Group = function CheckboxGroup(props) {
  const {
    value,
    defaultValue = [],
    options = [],
    direction = 'horizontal',
    disabled = false,
    onChange,
    style,
  } = props;

  const [internalValue, setInternalValue] = useState(defaultValue);

  const isControlled = value !== undefined;
  const currentValue = isControlled ? value : internalValue;

  const groupStyle = {
    display: 'flex',
    flexDirection: direction === 'vertical' ? 'column' : 'row',
    gap: direction === 'vertical' ? '8px' : '16px',
    flexWrap: 'wrap',
  };

  const handleChange = (optionValue, checked) => {
    let newValue;
    if (checked) {
      newValue = [...currentValue, optionValue];
    } else {
      newValue = currentValue.filter((v) => v !== optionValue);
    }
    if (!isControlled) {
      setInternalValue(newValue);
    }
    onChange?.(newValue);
  };

  return h(
    'div',
    { style: mergeStyles(groupStyle, style) },
    options.map((option) =>
      h(
        Checkbox,
        {
          key: option.value,
          checked: currentValue.includes(option.value),
          disabled: disabled || option.disabled,
          onChange: (checked) => handleChange(option.value, checked),
        },
        option.label
      )
    )
  );
};

export default Checkbox;
