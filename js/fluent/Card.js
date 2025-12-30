/**
 * MBink Fluent Design - Card 组件
 * 
 * @description 基于 Microsoft Fluent UI 设计规范的卡片组件
 * 
 * Props:
 * - appearance: 'filled' | 'filledAlternative' | 'outline' | 'subtle'
 * - size: 'small' | 'medium' | 'large'
 * - orientation: 'horizontal' | 'vertical'
 * - selected: boolean
 * - selectable: boolean
 * - onClick: (e) => void
 * - onSelectionChange: (e, data) => void
 */

import { h } from 'preact';
import { useState } from 'preact/hooks';
import { 
  colors, borderRadius, shadow, transition, spacing 
} from './theme.js';
import { mergeStyles } from './utils.js';

// 外观样式
const appearanceStyles = {
  filled: {
    base: {
      backgroundColor: colors.colorNeutralBackground1,
      boxShadow: shadow.shadow4,
      border: 'none',
    },
    hover: {
      boxShadow: shadow.shadow8,
    },
    pressed: {
      boxShadow: shadow.shadow2,
    },
  },
  filledAlternative: {
    base: {
      backgroundColor: colors.colorNeutralBackground2,
      boxShadow: shadow.shadow4,
      border: 'none',
    },
    hover: {
      boxShadow: shadow.shadow8,
    },
    pressed: {
      boxShadow: shadow.shadow2,
    },
  },
  outline: {
    base: {
      backgroundColor: colors.colorNeutralBackground1,
      boxShadow: 'none',
      border: `1px solid ${colors.colorNeutralStroke1}`,
    },
    hover: {
      borderColor: colors.colorNeutralStroke1Hover,
    },
    pressed: {
      borderColor: colors.colorNeutralStroke1Pressed,
    },
  },
  subtle: {
    base: {
      backgroundColor: colors.colorSubtleBackground,
      boxShadow: 'none',
      border: 'none',
    },
    hover: {
      backgroundColor: colors.colorSubtleBackgroundHover,
    },
    pressed: {
      backgroundColor: colors.colorSubtleBackgroundPressed,
    },
  },
};

// 尺寸配置
const sizeStyles = {
  small: {
    padding: spacing.s,
    borderRadius: borderRadius.medium,
    gap: spacing.s,
  },
  medium: {
    padding: spacing.m,
    borderRadius: borderRadius.large,
    gap: spacing.m,
  },
  large: {
    padding: spacing.l,
    borderRadius: borderRadius.xLarge,
    gap: spacing.l,
  },
};

export function Card(props) {
  const {
    appearance = 'filled',
    size = 'medium',
    orientation = 'vertical',
    selected = false,
    selectable = false,
    onClick,
    onSelectionChange,
    children,
    style,
    ...rest
  } = props;

  const [hovered, setHovered] = useState(false);
  const [pressed, setPressed] = useState(false);

  const appearanceConfig = appearanceStyles[appearance] || appearanceStyles.filled;
  const sizeConfig = sizeStyles[size] || sizeStyles.medium;

  const isInteractive = onClick || selectable;

  // 获取当前状态样式
  const getStateStyle = () => {
    if (pressed && isInteractive) {
      return { ...appearanceConfig.base, ...appearanceConfig.pressed };
    }
    if (hovered && isInteractive) {
      return { ...appearanceConfig.base, ...appearanceConfig.hover };
    }
    return appearanceConfig.base;
  };

  // 选中状态样式
  const getSelectedStyle = () => {
    if (!selected) return {};
    return {
      outline: `2px solid ${colors.colorCompoundBrandStroke}`,
      outlineOffset: '2px',
    };
  };

  // 基础样式
  const cardStyle = {
    display: 'flex',
    flexDirection: orientation === 'horizontal' ? 'row' : 'column',
    gap: sizeConfig.gap,
    padding: sizeConfig.padding,
    borderRadius: sizeConfig.borderRadius,
    transition: `all ${transition.normal}`,
    cursor: isInteractive ? 'pointer' : 'default',
    boxSizing: 'border-box',
    ...getStateStyle(),
    ...getSelectedStyle(),
  };

  const handleClick = (e) => {
    if (selectable) {
      onSelectionChange?.(e, { selected: !selected });
    }
    onClick?.(e);
  };

  return h(
    'div',
    {
      role: selectable ? 'button' : undefined,
      'aria-selected': selectable ? selected : undefined,
      tabIndex: isInteractive ? 0 : undefined,
      style: mergeStyles(cardStyle, style),
      onClick: handleClick,
      onMouseEnter: () => setHovered(true),
      onMouseLeave: () => { setHovered(false); setPressed(false); },
      onMouseDown: () => setPressed(true),
      onMouseUp: () => setPressed(false),
      ...rest,
    },
    children
  );
}

/**
 * CardHeader - 卡片头部
 */
export function CardHeader(props) {
  const {
    image,
    header,
    description,
    action,
    style,
    children,
    ...rest
  } = props;

  const containerStyle = {
    display: 'flex',
    alignItems: 'flex-start',
    gap: spacing.m,
  };

  const imageStyle = {
    flexShrink: 0,
    width: '32px',
    height: '32px',
    borderRadius: borderRadius.medium,
    objectFit: 'cover',
  };

  const contentStyle = {
    flex: 1,
    minWidth: 0,
    display: 'flex',
    flexDirection: 'column',
    gap: spacing.xxs,
  };

  const headerStyle = {
    fontSize: '14px',
    fontWeight: '600',
    color: colors.colorNeutralForeground1,
    margin: 0,
  };

  const descriptionStyle = {
    fontSize: '12px',
    color: colors.colorNeutralForeground2,
    margin: 0,
  };

  const actionStyle = {
    flexShrink: 0,
  };

  return h(
    'div',
    { style: mergeStyles(containerStyle, style), ...rest },
    [
      image && h('img', { key: 'image', src: image, style: imageStyle, alt: '' }),
      h('div', { key: 'content', style: contentStyle }, [
        header && h('div', { key: 'header', style: headerStyle }, header),
        description && h('div', { key: 'desc', style: descriptionStyle }, description),
      ]),
      action && h('div', { key: 'action', style: actionStyle }, action),
      children,
    ]
  );
}

/**
 * CardPreview - 卡片预览区域（图片/媒体）
 */
export function CardPreview(props) {
  const { children, style, ...rest } = props;

  const previewStyle = {
    margin: `-${spacing.m}`,
    marginBottom: spacing.m,
    overflow: 'hidden',
    borderRadius: `${borderRadius.large} ${borderRadius.large} 0 0`,
  };

  return h('div', { style: mergeStyles(previewStyle, style), ...rest }, children);
}

/**
 * CardFooter - 卡片底部
 */
export function CardFooter(props) {
  const { children, style, ...rest } = props;

  const footerStyle = {
    display: 'flex',
    alignItems: 'center',
    justifyContent: 'flex-end',
    gap: spacing.s,
    marginTop: 'auto',
  };

  return h('div', { style: mergeStyles(footerStyle, style), ...rest }, children);
}

export default Card;
