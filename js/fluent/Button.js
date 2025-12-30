/**
 * MBink Fluent Design - Button 组件
 * 
 * @description 基于 Microsoft Fluent UI 设计规范的按钮组件
 * 
 * Props:
 * - appearance: 'primary' | 'secondary' | 'outline' | 'subtle' | 'transparent'
 * - size: 'small' | 'medium' | 'large'
 * - shape: 'rounded' | 'circular' | 'square'
 * - disabled: boolean
 * - disabledFocusable: boolean
 * - icon: VNode
 * - iconPosition: 'before' | 'after'
 * - onClick: (e) => void
 */

import { h } from 'preact';
import { useState } from 'preact/hooks';
import { colors, borderRadius, shadow, componentSizes, transition, fontFamily, fontWeight } from './theme.js';
import { mergeStyles } from './utils.js';

// 外观样式配置
const appearanceStyles = {
  // 主要按钮 - 品牌色背景
  primary: {
    base: {
      backgroundColor: colors.colorBrandBackground,
      color: colors.colorNeutralForegroundOnBrand,
      border: 'none',
    },
    hover: {
      backgroundColor: colors.colorBrandBackgroundHover,
    },
    pressed: {
      backgroundColor: colors.colorBrandBackgroundPressed,
    },
    disabled: {
      backgroundColor: colors.colorNeutralBackgroundDisabled,
      color: colors.colorNeutralForegroundDisabled,
    },
  },

  // 次要按钮 - 中性背景
  secondary: {
    base: {
      backgroundColor: colors.colorNeutralBackground1,
      color: colors.colorNeutralForeground1,
      border: `1px solid ${colors.colorNeutralStroke1}`,
    },
    hover: {
      backgroundColor: colors.colorNeutralBackground1Hover,
      borderColor: colors.colorNeutralStroke1Hover,
    },
    pressed: {
      backgroundColor: colors.colorNeutralBackground1Pressed,
      borderColor: colors.colorNeutralStroke1Pressed,
    },
    disabled: {
      backgroundColor: colors.colorNeutralBackgroundDisabled,
      color: colors.colorNeutralForegroundDisabled,
      borderColor: colors.colorNeutralStrokeDisabled,
    },
  },

  // 轮廓按钮 - 透明背景 + 品牌色边框
  outline: {
    base: {
      backgroundColor: colors.colorTransparentBackground,
      color: colors.colorBrandForeground1,
      border: `1px solid ${colors.colorBrandStroke1}`,
    },
    hover: {
      backgroundColor: colors.colorSubtleBackgroundHover,
      color: colors.colorBrandForeground2,
    },
    pressed: {
      backgroundColor: colors.colorSubtleBackgroundPressed,
    },
    disabled: {
      backgroundColor: colors.colorTransparentBackground,
      color: colors.colorNeutralForegroundDisabled,
      borderColor: colors.colorNeutralStrokeDisabled,
    },
  },

  // 微妙按钮 - 透明背景，悬停显示
  subtle: {
    base: {
      backgroundColor: colors.colorSubtleBackground,
      color: colors.colorNeutralForeground1,
      border: 'none',
    },
    hover: {
      backgroundColor: colors.colorSubtleBackgroundHover,
    },
    pressed: {
      backgroundColor: colors.colorSubtleBackgroundPressed,
    },
    disabled: {
      backgroundColor: colors.colorTransparentBackground,
      color: colors.colorNeutralForegroundDisabled,
    },
  },

  // 透明按钮 - 完全透明
  transparent: {
    base: {
      backgroundColor: colors.colorTransparentBackground,
      color: colors.colorBrandForeground1,
      border: 'none',
    },
    hover: {
      color: colors.colorBrandForeground2,
    },
    pressed: {
      color: colors.colorCompoundBrandForeground1Pressed,
    },
    disabled: {
      color: colors.colorNeutralForegroundDisabled,
    },
  },
};

// 形状样式
const shapeStyles = {
  rounded: (size) => ({
    borderRadius: borderRadius.medium,
  }),
  circular: (size) => ({
    borderRadius: borderRadius.circular,
    minWidth: componentSizes[size].height,
    padding: '0',
  }),
  square: (size) => ({
    borderRadius: borderRadius.none,
  }),
};

export function Button(props) {
  const {
    appearance = 'secondary',
    size = 'medium',
    shape = 'rounded',
    disabled = false,
    disabledFocusable = false,
    icon = null,
    iconPosition = 'before',
    onClick,
    children,
    style,
    ...rest
  } = props;

  const [isHovered, setIsHovered] = useState(false);
  const [isPressed, setIsPressed] = useState(false);

  const sizeConfig = componentSizes[size] || componentSizes.medium;
  const appearanceConfig = appearanceStyles[appearance] || appearanceStyles.secondary;
  const shapeConfig = shapeStyles[shape] || shapeStyles.rounded;

  // 计算当前状态的样式
  const getStateStyle = () => {
    if (disabled) return appearanceConfig.disabled || {};
    if (isPressed) return { ...appearanceConfig.base, ...appearanceConfig.pressed };
    if (isHovered) return { ...appearanceConfig.base, ...appearanceConfig.hover };
    return appearanceConfig.base;
  };

  // 基础样式
  const baseStyle = {
    display: 'inline-flex',
    alignItems: 'center',
    justifyContent: 'center',
    gap: sizeConfig.gap,
    height: sizeConfig.height,
    minWidth: sizeConfig.minWidth,
    padding: sizeConfig.padding,
    fontSize: sizeConfig.fontSize,
    fontFamily: fontFamily.base,
    fontWeight: fontWeight.semibold,
    lineHeight: 'normal',  // 使用 normal 而不是 '1'，避免文字被裁剪
    cursor: disabled ? 'not-allowed' : 'pointer',
    transition: `all ${transition.fast}`,
    outline: 'none',
    textDecoration: 'none',
    whiteSpace: 'nowrap',
    userSelect: 'none',
    boxSizing: 'border-box',
    verticalAlign: 'middle',
    ...shapeConfig(size),
    ...getStateStyle(),
  };

  // 图标只有按钮的特殊处理
  const isIconOnly = icon && !children;
  if (isIconOnly) {
    baseStyle.minWidth = sizeConfig.height;
    baseStyle.padding = '0';
  }

  const handleClick = (e) => {
    if (disabled && !disabledFocusable) {
      e.preventDefault();
      return;
    }
    onClick?.(e);
  };

  const handleMouseEnter = () => !disabled && setIsHovered(true);
  const handleMouseLeave = () => {
    setIsHovered(false);
    setIsPressed(false);
  };
  const handleMouseDown = () => !disabled && setIsPressed(true);
  const handleMouseUp = () => setIsPressed(false);

  // 构建内容
  const content = [];
  
  if (icon && iconPosition === 'before') {
    content.push(h('span', { 
      key: 'icon-before',
      style: { 
        display: 'inline-flex', 
        alignItems: 'center',
        fontSize: sizeConfig.iconSize,
      } 
    }, icon));
  }
  
  if (children) {
    content.push(h('span', { key: 'content' }, children));
  }
  
  if (icon && iconPosition === 'after') {
    content.push(h('span', { 
      key: 'icon-after',
      style: { 
        display: 'inline-flex', 
        alignItems: 'center',
        fontSize: sizeConfig.iconSize,
      } 
    }, icon));
  }

  return h(
    'button',
    {
      type: 'button',
      disabled: disabled && !disabledFocusable,
      'aria-disabled': disabled,
      style: mergeStyles(baseStyle, style),
      onClick: handleClick,
      onMouseEnter: handleMouseEnter,
      onMouseLeave: handleMouseLeave,
      onMouseDown: handleMouseDown,
      onMouseUp: handleMouseUp,
      ...rest,
    },
    ...content
  );
}

/**
 * CompoundButton - 复合按钮，带有主文本和次要文本
 */
export function CompoundButton(props) {
  const {
    secondaryContent,
    children,
    size = 'medium',
    ...rest
  } = props;

  const content = h('div', {
    style: {
      display: 'flex',
      flexDirection: 'column',
      alignItems: 'flex-start',
      textAlign: 'left',
    }
  }, [
    h('span', { 
      key: 'primary',
      style: { fontWeight: fontWeight.semibold } 
    }, children),
    secondaryContent && h('span', { 
      key: 'secondary',
      style: { 
        fontSize: '12px',
        fontWeight: fontWeight.regular,
        opacity: 0.8,
      } 
    }, secondaryContent),
  ]);

  return h(Button, {
    ...rest,
    size: 'large',
    style: {
      height: 'auto',
      minHeight: '52px',
      padding: '8px 12px',
      ...rest.style,
    },
  }, content);
}

/**
 * ToggleButton - 切换按钮
 */
export function ToggleButton(props) {
  const {
    checked = false,
    defaultChecked = false,
    onChange,
    appearance = 'secondary',
    ...rest
  } = props;

  const [internalChecked, setInternalChecked] = useState(defaultChecked);
  const isControlled = checked !== undefined && onChange !== undefined;
  const isChecked = isControlled ? checked : internalChecked;

  const handleClick = (e) => {
    const newValue = !isChecked;
    if (!isControlled) {
      setInternalChecked(newValue);
    }
    onChange?.(e, { checked: newValue });
    rest.onClick?.(e);
  };

  // 选中状态使用 primary 外观
  const effectiveAppearance = isChecked ? 'primary' : appearance;

  return h(Button, {
    ...rest,
    appearance: effectiveAppearance,
    onClick: handleClick,
    'aria-pressed': isChecked,
  });
}

export default Button;
