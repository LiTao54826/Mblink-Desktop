/**
 * MBlink Radio 组件
 *
 * Props:
 * - checked: boolean
 * - disabled: boolean
 * - value: any
 * - onChange: (checked, e) => void
 * - children: label 文字
 */

import { h } from 'preact';
import { useState } from 'preact/hooks';
import { colors, transition } from './theme.js';
import { mergeStyles } from './utils.js';

export function Radio(props) {
  const {
    checked,
    defaultChecked = false,
    disabled = false,
    value,
    name,
    onChange,
    children,
    style,
    ...rest
  } = props;

  const [internalChecked, setInternalChecked] = useState(defaultChecked);

  const isControlled = checked !== undefined;
  const isChecked = isControlled ? checked : internalChecked;

  const wrapperStyle = {
    display: 'inline-flex',
    alignItems: 'center',
    gap: '8px',
    cursor: disabled ? 'not-allowed' : 'pointer',
    opacity: disabled ? 0.5 : 1,
    userSelect: 'none',
  };

  const circleStyle = {
    position: 'relative',
    width: '16px',
    height: '16px',
    borderRadius: '50%',
    border: `2px solid ${isChecked ? colors.primary : colors.border}`,
    backgroundColor: isChecked ? colors.primary : colors.bg,
    transition: `all ${transition.fast}`,
    display: 'flex',
    alignItems: 'center',
    justifyContent: 'center',
  };

  // 选中时显示白色内圈
  const dotStyle = isChecked
    ? {
        width: '6px',
        height: '6px',
        borderRadius: '50%',
        backgroundColor: '#ffffff',
      }
    : {
        width: '0px',
        height: '0px',
      };

  const labelStyle = {
    fontSize: '14px',
    color: disabled ? colors.textDisabled : colors.text,
  };

  const handleClick = (e) => {
    if (disabled) return;
    e.preventDefault();
    if (!isControlled) {
      setInternalChecked(true);
    }
    onChange?.(true, e);
  };

  return h(
    'div',
    { style: mergeStyles(wrapperStyle, style), onClick: handleClick },
    h('span', { style: circleStyle }, h('span', { style: dotStyle })),
    children && h('span', { style: labelStyle }, children)
  );
}

/**
 * Radio.Group 组件
 *
 * Props:
 * - value: any
 * - options: Array<{ value, label, disabled? }>
 * - direction: 'horizontal' | 'vertical'
 * - onChange: (value) => void
 */
Radio.Group = function RadioGroup(props) {
  const {
    value,
    defaultValue,
    options = [],
    direction = 'horizontal',
    disabled = false,
    name,
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

  const handleChange = (optionValue) => {
    if (!isControlled) {
      setInternalValue(optionValue);
    }
    onChange?.(optionValue);
  };

  return h(
    'div',
    { style: mergeStyles(groupStyle, style) },
    options.map((option) =>
      h(
        Radio,
        {
          key: option.value,
          checked: currentValue === option.value,
          disabled: disabled || option.disabled,
          name,
          value: option.value,
          onChange: () => handleChange(option.value),
        },
        option.label
      )
    )
  );
};

export default Radio;
