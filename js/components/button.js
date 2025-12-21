/**
 * LightUI Button 组件
 *
 * Props:
 * - variant: 'primary' | 'secondary' | 'outline' | 'ghost' | 'link' | 'danger'
 * - size: 'sm' | 'md' | 'lg'
 * - disabled: boolean
 * - loading: boolean
 * - block: boolean (全宽)
 * - icon: VNode (图标)
 * - iconPosition: 'left' | 'right'
 * - onClick: function
 */

import { h } from 'preact';
import { useState, useEffect } from 'preact/hooks';
import { colors, radius, sizes, transition } from './theme.js';
import { mergeStyles } from './utils.js';

// 变体样式
const variantStyles = {
  primary: {
    base: {
      backgroundColor: colors.primary,
      color: colors.textInverse,
      border: 'none',
    },
    hover: {
      backgroundColor: colors.primaryHover,
    },
    active: {
      backgroundColor: colors.primaryActive,
    },
  },
  secondary: {
    base: {
      backgroundColor: colors.bgSecondary,
      color: colors.text,
      border: `1px solid ${colors.border}`,
    },
    hover: {
      backgroundColor: colors.bgHover,
      borderColor: colors.borderHover,
    },
    active: {
      backgroundColor: colors.bgActive,
    },
  },
  outline: {
    base: {
      backgroundColor: 'transparent',
      color: colors.primary,
      border: `1px solid ${colors.primary}`,
    },
    hover: {
      backgroundColor: colors.primary,
      color: colors.textInverse,
    },
    active: {
      backgroundColor: colors.primaryHover,
    },
  },
  ghost: {
    base: {
      backgroundColor: 'transparent',
      color: colors.text,
      border: 'none',
    },
    hover: {
      backgroundColor: colors.bgHover,
    },
    active: {
      backgroundColor: colors.bgActive,
    },
  },
  link: {
    base: {
      backgroundColor: 'transparent',
      color: colors.primary,
      border: 'none',
      padding: '0',
      height: 'auto',
    },
    hover: {
      color: colors.primaryHover,
      textDecoration: 'underline',
    },
    active: {
      color: colors.primaryActive,
    },
  },
  danger: {
    base: {
      backgroundColor: colors.error,
      color: colors.textInverse,
      border: 'none',
    },
    hover: {
      backgroundColor: colors.errorHover,
    },
    active: {
      backgroundColor: '#b91c1c',
    },
  },
};

// Spinner 组件 - 使用 useState 触发重渲染实现旋转动画
function Spinner({ size = 14 }) {
  const [angle, setAngle] = useState(0);

  useEffect(() => {
    const interval = setInterval(() => {
      setAngle((prev) => (prev + 30) % 360);
    }, 50);
    return () => clearInterval(interval);
  }, []);

  const spinnerStyle = {
    width: `${size}px`,
    height: `${size}px`,
    border: '2px solid transparent',
    borderTopColor: 'currentColor',
    borderRightColor: 'currentColor',
    borderRadius: '50%',
    display: 'inline-block',
    verticalAlign: 'middle',
    transform: `rotate(${angle}deg)`,
  };

  return h('span', { style: spinnerStyle });
}

export function Button(props) {
  const {
    variant = 'primary',
    size = 'md',
    disabled = false,
    loading = false,
    block = false,
    icon = null,
    iconPosition = 'left',
    onClick,
    children,
    style,
    ...rest
  } = props;

  const sizeConfig = sizes[size] || sizes.md;
  const variantConfig = variantStyles[variant] || variantStyles.primary;

  const baseStyle = {
    display: 'inline-flex',
    alignItems: 'center',
    justifyContent: 'center',
    gap: '6px',
    height: sizeConfig.height,
    padding: sizeConfig.padding,
    fontSize: sizeConfig.fontSize,
    fontWeight: '500',
    lineHeight: '1',
    borderRadius: radius.md,
    cursor: disabled || loading ? 'not-allowed' : 'pointer',
    opacity: disabled ? 0.5 : 1,
    transition: `all ${transition.fast}`,
    outline: 'none',
    textDecoration: 'none',
    whiteSpace: 'nowrap',
    userSelect: 'none',
    boxSizing: 'border-box',
    ...(block && { display: 'flex', width: '100%' }),
    ...variantConfig.base,
  };

  const handleClick = (e) => {
    if (disabled || loading) {
      e.preventDefault();
      return;
    }
    onClick?.(e);
  };

  const handleMouseEnter = (e) => {
    if (!disabled && !loading) {
      Object.assign(e.target.style, variantConfig.hover);
    }
  };

  const handleMouseLeave = (e) => {
    if (!disabled && !loading) {
      Object.assign(e.target.style, variantConfig.base);
    }
  };

  const handleMouseDown = (e) => {
    if (!disabled && !loading) {
      Object.assign(e.target.style, variantConfig.active);
    }
  };

  const handleMouseUp = (e) => {
    if (!disabled && !loading) {
      Object.assign(e.target.style, variantConfig.hover);
    }
  };

  const iconElement = loading
    ? h(Spinner, { size: parseInt(sizeConfig.iconSize) })
    : icon;

  const content = [];
  if (iconElement && iconPosition === 'left') {
    content.push(iconElement);
  }
  if (children) {
    content.push(children);
  }
  if (iconElement && iconPosition === 'right') {
    content.push(iconElement);
  }

  return h(
    'button',
    {
      type: 'button',
      disabled: disabled || loading,
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

export default Button;
