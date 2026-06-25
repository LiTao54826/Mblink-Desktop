/**
 * MBlink Switch 组件
 *
 * Props:
 * - checked: boolean
 * - disabled: boolean
 * - size: 'sm' | 'md'
 * - checkedText: string
 * - uncheckedText: string
 * - onChange: (checked) => void
 */

import { h } from 'preact';
import { useState } from 'preact/hooks';
import { colors, transition } from './theme.js';
import { mergeStyles } from './utils.js';

const sizeConfig = {
  sm: {
    width: 28,
    height: 16,
    dotSize: 12,
    dotOffset: 2,
  },
  md: {
    width: 44,
    height: 22,
    dotSize: 18,
    dotOffset: 2,
  },
};

export function Switch(props) {
  const {
    checked,
    defaultChecked = false,
    disabled = false,
    size = 'md',
    checkedText,
    uncheckedText,
    onChange,
    style,
    ...rest
  } = props;

  const [internalChecked, setInternalChecked] = useState(defaultChecked);

  const isControlled = checked !== undefined;
  const isChecked = isControlled ? checked : internalChecked;

  const config = sizeConfig[size] || sizeConfig.md;

  const wrapperStyle = {
    display: 'inline-flex',
    alignItems: 'center',
    gap: '8px',
    cursor: disabled ? 'not-allowed' : 'pointer',
    opacity: disabled ? 0.5 : 1,
  };

  const trackStyle = {
    position: 'relative',
    width: `${config.width}px`,
    height: `${config.height}px`,
    borderRadius: `${config.height / 2}px`,
    backgroundColor: isChecked ? colors.primary : colors.textTertiary,
    transition: `background-color ${transition.normal}`,
    flexShrink: 0,
  };

  const dotStyle = {
    position: 'absolute',
    top: `${config.dotOffset}px`,
    left: isChecked
      ? `${config.width - config.dotSize - config.dotOffset}px`
      : `${config.dotOffset}px`,
    width: `${config.dotSize}px`,
    height: `${config.dotSize}px`,
    borderRadius: '50%',
    backgroundColor: colors.bg,
    boxShadow: '0 1px 2px rgba(0, 0, 0, 0.2)',
    transition: `left ${transition.normal}`,
  };

  const textStyle = {
    fontSize: size === 'sm' ? '12px' : '14px',
    color: colors.text,
  };

  const handleClick = () => {
    if (disabled) return;
    const newChecked = !isChecked;
    if (!isControlled) {
      setInternalChecked(newChecked);
    }
    onChange?.(newChecked);
  };

  const text = isChecked ? checkedText : uncheckedText;

  return h(
    'div',
    {
      style: mergeStyles(wrapperStyle, style),
      onClick: handleClick,
      ...rest,
    },
    h('div', { style: trackStyle }, h('div', { style: dotStyle })),
    text && h('span', { style: textStyle }, text)
  );
}

export default Switch;
