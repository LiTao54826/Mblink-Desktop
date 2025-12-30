/**
 * MBink Fluent Design - Textarea 组件
 * 
 * @description 基于 Microsoft Fluent UI 设计规范的多行文本输入组件
 * 
 * Props:
 * - size: 'small' | 'medium' | 'large'
 * - appearance: 'outline' | 'filledDarker' | 'filledLighter'
 * - resize: 'none' | 'horizontal' | 'vertical' | 'both'
 * - value: string
 * - defaultValue: string
 * - placeholder: string
 * - disabled: boolean
 * - rows: number
 * - onChange: (e, data) => void
 */

import { h } from 'preact';
import { useState } from 'preact/hooks';
import { 
  colors, borderRadius, transition, fontFamily, fontSize, spacing 
} from './theme.js';
import { mergeStyles } from './utils.js';

// 外观样式
const appearanceStyles = {
  outline: {
    base: {
      backgroundColor: colors.colorNeutralBackground1,
      border: `1px solid ${colors.colorNeutralStroke1}`,
      borderBottom: `1px solid ${colors.colorNeutralStrokeAccessible}`,
    },
    hover: {
      borderColor: colors.colorNeutralStroke1Hover,
    },
    focus: {
      borderColor: colors.colorCompoundBrandStroke,
      borderBottomColor: colors.colorCompoundBrandStroke,
      borderBottomWidth: '2px',
    },
    disabled: {
      backgroundColor: colors.colorNeutralBackgroundDisabled,
      borderColor: colors.colorNeutralStrokeDisabled,
    },
  },
  filledDarker: {
    base: {
      backgroundColor: colors.colorNeutralBackground3,
      border: 'none',
      borderBottom: `1px solid transparent`,
    },
    hover: {
      backgroundColor: colors.colorNeutralBackground3,
    },
    focus: {
      borderBottomColor: colors.colorCompoundBrandStroke,
      borderBottomWidth: '2px',
    },
    disabled: {
      backgroundColor: colors.colorNeutralBackgroundDisabled,
    },
  },
  filledLighter: {
    base: {
      backgroundColor: colors.colorNeutralBackground1,
      border: 'none',
      borderBottom: `1px solid transparent`,
    },
    hover: {
      backgroundColor: colors.colorNeutralBackground1Hover,
    },
    focus: {
      borderBottomColor: colors.colorCompoundBrandStroke,
      borderBottomWidth: '2px',
    },
    disabled: {
      backgroundColor: colors.colorNeutralBackgroundDisabled,
    },
  },
};

// 尺寸配置
const sizeStyles = {
  small: {
    fontSize: fontSize.base200,
    padding: spacing.s,
    minHeight: '52px',
  },
  medium: {
    fontSize: fontSize.base300,
    padding: spacing.mNudge,
    minHeight: '64px',
  },
  large: {
    fontSize: fontSize.base400,
    padding: spacing.m,
    minHeight: '76px',
  },
};

export function Textarea(props) {
  const {
    size = 'medium',
    appearance = 'outline',
    resize = 'none',
    value,
    defaultValue = '',
    placeholder,
    disabled = false,
    rows = 3,
    onChange,
    onFocus,
    onBlur,
    style,
    ...rest
  } = props;

  const [focused, setFocused] = useState(false);
  const [hovered, setHovered] = useState(false);
  const [internalValue, setInternalValue] = useState(defaultValue);

  const isControlled = value !== undefined;
  const currentValue = isControlled ? value : internalValue;

  const sizeConfig = sizeStyles[size] || sizeStyles.medium;
  const appearanceConfig = appearanceStyles[appearance] || appearanceStyles.outline;

  // 获取当前状态样式
  const getStateStyle = () => {
    if (disabled) return appearanceConfig.disabled || {};
    if (focused) return { ...appearanceConfig.base, ...appearanceConfig.focus };
    if (hovered) return { ...appearanceConfig.base, ...appearanceConfig.hover };
    return appearanceConfig.base;
  };

  const textareaStyle = {
    width: '100%',
    minHeight: sizeConfig.minHeight,
    padding: sizeConfig.padding,
    fontSize: sizeConfig.fontSize,
    fontFamily: fontFamily.base,
    lineHeight: '1.5',
    color: disabled ? colors.colorNeutralForegroundDisabled : colors.colorNeutralForeground1,
    borderRadius: borderRadius.medium,
    transition: `all ${transition.fast}`,
    boxSizing: 'border-box',
    outline: 'none',
    resize: resize,
    cursor: disabled ? 'not-allowed' : 'text',
    ...getStateStyle(),
  };

  const handleChange = (e) => {
    const newValue = e.target.value;
    if (!isControlled) {
      setInternalValue(newValue);
    }
    onChange?.(e, { value: newValue });
  };

  const handleFocus = (e) => {
    setFocused(true);
    onFocus?.(e);
  };

  const handleBlur = (e) => {
    setFocused(false);
    onBlur?.(e);
  };

  return h('textarea', {
    value: currentValue,
    placeholder,
    disabled,
    rows,
    style: mergeStyles(textareaStyle, style),
    onInput: handleChange,
    onFocus: handleFocus,
    onBlur: handleBlur,
    onMouseEnter: () => setHovered(true),
    onMouseLeave: () => setHovered(false),
    ...rest,
  });
}

export default Textarea;
