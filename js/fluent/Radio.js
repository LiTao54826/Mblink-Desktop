/**
 * MBink Fluent Design - Radio 组件
 * 
 * @description 基于 Microsoft Fluent UI 设计规范的单选框组件
 * 
 * RadioGroup Props:
 * - value: string
 * - defaultValue: string
 * - name: string
 * - disabled: boolean
 * - layout: 'vertical' | 'horizontal' | 'horizontalStacked'
 * - onChange: (e, data) => void
 * 
 * Radio Props:
 * - value: string (必需)
 * - label: string | VNode
 * - disabled: boolean
 */

import { h, createContext } from 'preact';
import { useState, useContext } from 'preact/hooks';
import { 
  colors, borderRadius, transition, fontFamily, fontSize, spacing 
} from './theme.js';
import { mergeStyles } from './utils.js';

// Radio Group 上下文
const RadioGroupContext = createContext(null);

// 尺寸配置
const RADIO_SIZE = 16;
const DOT_SIZE = 8;

export function RadioGroup(props) {
  const {
    value,
    defaultValue = '',
    name,
    disabled = false,
    layout = 'vertical',
    onChange,
    children,
    style,
    ...rest
  } = props;

  const [internalValue, setInternalValue] = useState(defaultValue);
  const isControlled = value !== undefined;
  const currentValue = isControlled ? value : internalValue;

  const handleChange = (newValue, e) => {
    if (!isControlled) {
      setInternalValue(newValue);
    }
    onChange?.(e, { value: newValue });
  };

  // 布局样式
  const layoutStyles = {
    vertical: {
      display: 'flex',
      flexDirection: 'column',
      gap: spacing.s,
    },
    horizontal: {
      display: 'flex',
      flexDirection: 'row',
      gap: spacing.l,
      flexWrap: 'wrap',
    },
    horizontalStacked: {
      display: 'flex',
      flexDirection: 'row',
      gap: spacing.xxl,
    },
  };

  const containerStyle = {
    ...layoutStyles[layout],
  };

  const contextValue = {
    name,
    value: currentValue,
    disabled,
    onChange: handleChange,
  };

  return h(
    RadioGroupContext.Provider,
    { value: contextValue },
    h('div', {
      role: 'radiogroup',
      style: mergeStyles(containerStyle, style),
      ...rest,
    }, children)
  );
}

export function Radio(props) {
  const {
    value,
    label,
    disabled: propDisabled = false,
    style,
    ...rest
  } = props;

  const context = useContext(RadioGroupContext);
  const [hovered, setHovered] = useState(false);
  const [pressed, setPressed] = useState(false);

  const disabled = propDisabled || context?.disabled || false;
  const isChecked = context ? context.value === value : false;
  const name = context?.name;

  // 容器样式
  const containerStyle = {
    display: 'inline-flex',
    alignItems: 'center',
    gap: spacing.s,
    cursor: disabled ? 'not-allowed' : 'pointer',
    userSelect: 'none',
    opacity: disabled ? 0.5 : 1,
  };

  // 圆圈样式
  const getCircleStyle = () => {
    const base = {
      position: 'relative',
      width: `${RADIO_SIZE}px`,
      height: `${RADIO_SIZE}px`,
      borderRadius: borderRadius.circular,
      border: `1px solid ${colors.colorNeutralStrokeAccessible}`,
      backgroundColor: colors.colorNeutralBackground1,
      transition: `all ${transition.fast}`,
      flexShrink: 0,
      boxSizing: 'border-box',
      display: 'flex',
      alignItems: 'center',
      justifyContent: 'center',
    };

    if (disabled) {
      return {
        ...base,
        borderColor: colors.colorNeutralStrokeDisabled,
      };
    }

    if (isChecked) {
      return {
        ...base,
        borderColor: pressed 
          ? colors.colorCompoundBrandStrokePressed
          : hovered 
            ? colors.colorCompoundBrandStrokeHover
            : colors.colorCompoundBrandStroke,
      };
    }

    return {
      ...base,
      borderColor: hovered 
        ? colors.colorNeutralStroke1Hover 
        : colors.colorNeutralStrokeAccessible,
      backgroundColor: hovered 
        ? colors.colorNeutralBackground1Hover 
        : colors.colorNeutralBackground1,
    };
  };

  // 内部圆点样式
  const getDotStyle = () => {
    if (!isChecked) return { display: 'none' };

    return {
      width: `${DOT_SIZE}px`,
      height: `${DOT_SIZE}px`,
      borderRadius: borderRadius.circular,
      backgroundColor: disabled 
        ? colors.colorNeutralForegroundDisabled
        : pressed 
          ? colors.colorCompoundBrandBackgroundPressed
          : hovered 
            ? colors.colorCompoundBrandBackgroundHover
            : colors.colorCompoundBrandBackground,
      transition: `all ${transition.fast}`,
    };
  };

  // 标签样式
  const labelStyle = {
    fontSize: fontSize.base300,
    fontFamily: fontFamily.base,
    color: disabled ? colors.colorNeutralForegroundDisabled : colors.colorNeutralForeground1,
    lineHeight: '1.4',
  };

  const handleClick = (e) => {
    if (disabled) return;
    context?.onChange(value, e);
  };

  return h(
    'label',
    {
      style: mergeStyles(containerStyle, style),
      onClick: handleClick,
      onMouseEnter: () => setHovered(true),
      onMouseLeave: () => { setHovered(false); setPressed(false); },
      onMouseDown: () => setPressed(true),
      onMouseUp: () => setPressed(false),
      ...rest,
    },
    [
      h('span', { key: 'circle', style: getCircleStyle() },
        h('span', { style: getDotStyle() })
      ),
      label && h('span', { key: 'label', style: labelStyle }, label),
    ]
  );
}

export default Radio;
