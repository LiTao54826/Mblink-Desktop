/**
 * MBink Fluent Design - Input 组件
 * 
 * @description 基于 Microsoft Fluent UI 设计规范的输入框组件
 * 
 * Props:
 * - size: 'small' | 'medium' | 'large'
 * - appearance: 'outline' | 'underline' | 'filledDarker' | 'filledLighter'
 * - type: 'text' | 'password' | 'email' | 'number' | 'tel' | 'url' | 'search'
 * - value: string
 * - defaultValue: string
 * - placeholder: string
 * - disabled: boolean
 * - contentBefore: VNode (前置内容/图标)
 * - contentAfter: VNode (后置内容/图标)
 * - onChange: (e, data) => void
 * - onFocus: (e) => void
 * - onBlur: (e) => void
 */

import { h } from 'preact';
import { useState, useRef } from 'preact/hooks';
import { 
  colors, borderRadius, componentSizes, transition, 
  fontFamily, fontSize, spacing 
} from './theme.js';
import { mergeStyles } from './utils.js';

// 外观样式
const appearanceStyles = {
  // 轮廓样式 - 默认
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

  // 下划线样式
  underline: {
    base: {
      backgroundColor: 'transparent',
      border: 'none',
      borderBottom: `1px solid ${colors.colorNeutralStrokeAccessible}`,
      borderRadius: '0',
    },
    hover: {
      borderBottomColor: colors.colorNeutralStroke1Hover,
    },
    focus: {
      borderBottomColor: colors.colorCompoundBrandStroke,
      borderBottomWidth: '2px',
    },
    disabled: {
      borderBottomColor: colors.colorNeutralStrokeDisabled,
    },
  },

  // 深色填充
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

  // 浅色填充
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
    height: '24px',
    fontSize: fontSize.base200,
    padding: `0 ${spacing.s}`,
    iconSize: '16px',
  },
  medium: {
    height: '32px',
    fontSize: fontSize.base300,
    padding: `0 ${spacing.mNudge}`,
    iconSize: '20px',
  },
  large: {
    height: '40px',
    fontSize: fontSize.base400,
    padding: `0 ${spacing.m}`,
    iconSize: '24px',
  },
};

export function Input(props) {
  const {
    size = 'medium',
    appearance = 'outline',
    type = 'text',
    value,
    defaultValue = '',
    placeholder,
    disabled = false,
    contentBefore,
    contentAfter,
    onChange,
    onFocus,
    onBlur,
    style,
    inputStyle,
    ...rest
  } = props;

  const [focused, setFocused] = useState(false);
  const [hovered, setHovered] = useState(false);
  const [internalValue, setInternalValue] = useState(defaultValue);
  const inputRef = useRef(null);

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

  // 容器样式
  const containerStyle = {
    display: 'inline-flex',
    alignItems: 'center',
    height: sizeConfig.height,
    borderRadius: borderRadius.medium,
    transition: `all ${transition.fast}`,
    boxSizing: 'border-box',
    cursor: disabled ? 'not-allowed' : 'text',
    ...getStateStyle(),
  };

  // 输入框样式
  const inputBaseStyle = {
    flex: 1,
    height: '100%',
    border: 'none',
    outline: 'none',
    backgroundColor: 'transparent',
    fontSize: sizeConfig.fontSize,
    fontFamily: fontFamily.base,
    color: disabled ? colors.colorNeutralForegroundDisabled : colors.colorNeutralForeground1,
    padding: sizeConfig.padding,
    boxSizing: 'border-box',
    minWidth: 0,
  };

  // 图标/内容样式
  const contentStyle = {
    display: 'inline-flex',
    alignItems: 'center',
    justifyContent: 'center',
    color: colors.colorNeutralForeground3,
    fontSize: sizeConfig.iconSize,
    flexShrink: 0,
  };

  const contentBeforeStyle = {
    ...contentStyle,
    paddingLeft: spacing.s,
  };

  const contentAfterStyle = {
    ...contentStyle,
    paddingRight: spacing.s,
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

  const handleContainerClick = () => {
    inputRef.current?.focus();
  };

  return h(
    'div',
    {
      style: mergeStyles(containerStyle, style),
      onClick: handleContainerClick,
      onMouseEnter: () => setHovered(true),
      onMouseLeave: () => setHovered(false),
    },
    [
      contentBefore && h('span', { key: 'before', style: contentBeforeStyle }, contentBefore),
      h('input', {
        key: 'input',
        ref: inputRef,
        type,
        value: currentValue,
        placeholder,
        disabled,
        style: mergeStyles(inputBaseStyle, inputStyle),
        onInput: handleChange,
        onFocus: handleFocus,
        onBlur: handleBlur,
        ...rest,
      }),
      contentAfter && h('span', { key: 'after', style: contentAfterStyle }, contentAfter),
    ]
  );
}

/**
 * SearchBox - 搜索框
 */
export function SearchBox(props) {
  const { onSearch, onClear, ...rest } = props;

  // 搜索图标
  const searchIcon = h('span', { style: { fontSize: '16px' } }, '🔍');
  
  // 清除按钮
  const clearButton = props.value ? h('span', {
    style: { 
      cursor: 'pointer',
      fontSize: '14px',
      opacity: 0.6,
    },
    onClick: (e) => {
      e.stopPropagation();
      onClear?.();
    },
  }, '✕') : null;

  const handleKeyDown = (e) => {
    if (e.key === 'Enter') {
      onSearch?.(e.target.value);
    }
  };

  return h(Input, {
    type: 'search',
    contentBefore: searchIcon,
    contentAfter: clearButton,
    onKeyDown: handleKeyDown,
    ...rest,
  });
}

export default Input;
