/**
 * MBink Fluent Design - Badge 组件
 * 
 * @description 基于 Microsoft Fluent UI 设计规范的徽章组件
 * 
 * Badge Props:
 * - appearance: 'filled' | 'ghost' | 'outline' | 'tint'
 * - color: 'brand' | 'danger' | 'important' | 'informative' | 'severe' | 'subtle' | 'success' | 'warning'
 * - size: 'tiny' | 'extraSmall' | 'small' | 'medium' | 'large' | 'extraLarge'
 * - shape: 'circular' | 'rounded' | 'square'
 * - icon: VNode
 * - iconPosition: 'before' | 'after'
 * 
 * CounterBadge Props:
 * - count: number
 * - overflowCount: number (默认 99)
 * - dot: boolean
 * - showZero: boolean
 */

import { h } from 'preact';
import { 
  colors, borderRadius, fontFamily, fontSize, fontWeight, spacing 
} from './theme.js';
import { mergeStyles } from './utils.js';

// 颜色配置
const colorStyles = {
  brand: {
    filled: {
      backgroundColor: colors.colorBrandBackground,
      color: colors.colorNeutralForegroundOnBrand,
    },
    ghost: {
      backgroundColor: 'transparent',
      color: colors.colorBrandForeground1,
    },
    outline: {
      backgroundColor: 'transparent',
      color: colors.colorBrandForeground1,
      border: `1px solid ${colors.colorBrandStroke1}`,
    },
    tint: {
      backgroundColor: colors.colorBrandBackground + '20',
      color: colors.colorBrandForeground1,
    },
  },
  danger: {
    filled: {
      backgroundColor: colors.colorStatusDangerBackground2,
      color: colors.colorNeutralForegroundOnBrand,
    },
    ghost: {
      backgroundColor: 'transparent',
      color: colors.colorStatusDangerForeground1,
    },
    outline: {
      backgroundColor: 'transparent',
      color: colors.colorStatusDangerForeground1,
      border: `1px solid ${colors.colorStatusDangerBorder1}`,
    },
    tint: {
      backgroundColor: colors.colorStatusDangerBackground1,
      color: colors.colorStatusDangerForeground1,
    },
  },
  success: {
    filled: {
      backgroundColor: colors.colorStatusSuccessBackground2,
      color: colors.colorNeutralForegroundOnBrand,
    },
    ghost: {
      backgroundColor: 'transparent',
      color: colors.colorStatusSuccessForeground1,
    },
    outline: {
      backgroundColor: 'transparent',
      color: colors.colorStatusSuccessForeground1,
      border: `1px solid ${colors.colorStatusSuccessBorder1}`,
    },
    tint: {
      backgroundColor: colors.colorStatusSuccessBackground1,
      color: colors.colorStatusSuccessForeground1,
    },
  },
  warning: {
    filled: {
      backgroundColor: colors.colorStatusWarningBackground2,
      color: colors.colorNeutralForeground1,
    },
    ghost: {
      backgroundColor: 'transparent',
      color: colors.colorStatusWarningForeground1,
    },
    outline: {
      backgroundColor: 'transparent',
      color: colors.colorStatusWarningForeground1,
      border: `1px solid ${colors.colorStatusWarningBorder1}`,
    },
    tint: {
      backgroundColor: colors.colorStatusWarningBackground1,
      color: colors.colorStatusWarningForeground1,
    },
  },
  informative: {
    filled: {
      backgroundColor: colors.colorNeutralBackground5,
      color: colors.colorNeutralForeground1,
    },
    ghost: {
      backgroundColor: 'transparent',
      color: colors.colorNeutralForeground2,
    },
    outline: {
      backgroundColor: 'transparent',
      color: colors.colorNeutralForeground2,
      border: `1px solid ${colors.colorNeutralStroke1}`,
    },
    tint: {
      backgroundColor: colors.colorNeutralBackground4,
      color: colors.colorNeutralForeground2,
    },
  },
  subtle: {
    filled: {
      backgroundColor: colors.colorNeutralBackground1,
      color: colors.colorNeutralForeground1,
    },
    ghost: {
      backgroundColor: 'transparent',
      color: colors.colorNeutralForeground3,
    },
    outline: {
      backgroundColor: 'transparent',
      color: colors.colorNeutralForeground3,
      border: `1px solid ${colors.colorNeutralStroke2}`,
    },
    tint: {
      backgroundColor: colors.colorNeutralBackground2,
      color: colors.colorNeutralForeground3,
    },
  },
};

// 尺寸配置
const sizeStyles = {
  tiny: {
    height: '12px',
    minWidth: '12px',
    fontSize: '8px',
    padding: '0 2px',
    iconSize: '8px',
    gap: '2px',
  },
  extraSmall: {
    height: '16px',
    minWidth: '16px',
    fontSize: '10px',
    padding: '0 4px',
    iconSize: '10px',
    gap: '2px',
  },
  small: {
    height: '18px',
    minWidth: '18px',
    fontSize: '10px',
    padding: '0 4px',
    iconSize: '12px',
    gap: '4px',
  },
  medium: {
    height: '20px',
    minWidth: '20px',
    fontSize: '12px',
    padding: '0 6px',
    iconSize: '12px',
    gap: '4px',
  },
  large: {
    height: '24px',
    minWidth: '24px',
    fontSize: '12px',
    padding: '0 6px',
    iconSize: '14px',
    gap: '4px',
  },
  extraLarge: {
    height: '32px',
    minWidth: '32px',
    fontSize: '14px',
    padding: '0 8px',
    iconSize: '16px',
    gap: '6px',
  },
};

// 形状配置
const shapeStyles = {
  circular: borderRadius.circular,
  rounded: borderRadius.medium,
  square: borderRadius.none,
};

export function Badge(props) {
  const {
    appearance = 'filled',
    color = 'brand',
    size = 'medium',
    shape = 'circular',
    icon,
    iconPosition = 'before',
    children,
    style,
    ...rest
  } = props;

  const colorConfig = colorStyles[color]?.[appearance] || colorStyles.brand.filled;
  const sizeConfig = sizeStyles[size] || sizeStyles.medium;

  const badgeStyle = {
    display: 'inline-flex',
    alignItems: 'center',
    justifyContent: 'center',
    gap: sizeConfig.gap,
    height: sizeConfig.height,
    minWidth: sizeConfig.minWidth,
    padding: children ? sizeConfig.padding : '0',
    fontSize: sizeConfig.fontSize,
    fontFamily: fontFamily.base,
    fontWeight: fontWeight.semibold,
    lineHeight: '1',
    borderRadius: shapeStyles[shape],
    boxSizing: 'border-box',
    whiteSpace: 'nowrap',
    verticalAlign: 'middle',
    ...colorConfig,
  };

  const iconStyle = {
    display: 'inline-flex',
    alignItems: 'center',
    fontSize: sizeConfig.iconSize,
  };

  const content = [];
  
  if (icon && iconPosition === 'before') {
    content.push(h('span', { key: 'icon-before', style: iconStyle }, icon));
  }
  
  if (children) {
    content.push(h('span', { key: 'content' }, children));
  }
  
  if (icon && iconPosition === 'after') {
    content.push(h('span', { key: 'icon-after', style: iconStyle }, icon));
  }

  return h('span', {
    style: mergeStyles(badgeStyle, style),
    ...rest,
  }, ...content);
}

/**
 * CounterBadge - 计数徽章
 */
export function CounterBadge(props) {
  const {
    count = 0,
    overflowCount = 99,
    dot = false,
    showZero = false,
    color = 'danger',
    size = 'medium',
    style,
    ...rest
  } = props;

  // 不显示的情况
  if (!dot && count === 0 && !showZero) {
    return null;
  }

  // 点状徽章
  if (dot) {
    const dotStyle = {
      width: '8px',
      height: '8px',
      minWidth: '8px',
      padding: '0',
    };
    return h(Badge, {
      color,
      size: 'tiny',
      style: mergeStyles(dotStyle, style),
      ...rest,
    });
  }

  // 计数显示
  const displayCount = count > overflowCount ? `${overflowCount}+` : String(count);

  return h(Badge, {
    color,
    size,
    style,
    ...rest,
  }, displayCount);
}

/**
 * PresenceBadge - 在线状态徽章
 */
export function PresenceBadge(props) {
  const {
    status = 'available', // available, away, busy, doNotDisturb, offline, outOfOffice, unknown
    size = 'medium',
    outOfOffice = false,
    style,
    ...rest
  } = props;

  const statusColors = {
    available: colors.colorStatusSuccessBackground2,
    away: colors.colorStatusWarningBackground2,
    busy: colors.colorStatusDangerBackground2,
    doNotDisturb: colors.colorStatusDangerBackground2,
    offline: colors.colorNeutralForeground4,
    outOfOffice: colors.colorNeutralForeground4,
    unknown: colors.colorNeutralForeground4,
  };

  const sizeMap = {
    tiny: '6px',
    extraSmall: '8px',
    small: '10px',
    medium: '12px',
    large: '16px',
    extraLarge: '20px',
  };

  const badgeSize = sizeMap[size] || sizeMap.medium;

  const presenceStyle = {
    width: badgeSize,
    height: badgeSize,
    minWidth: badgeSize,
    padding: '0',
    backgroundColor: statusColors[status] || statusColors.unknown,
    border: outOfOffice ? `2px solid ${colors.colorNeutralBackground1}` : 'none',
    boxSizing: 'border-box',
  };

  return h(Badge, {
    shape: 'circular',
    style: mergeStyles(presenceStyle, style),
    ...rest,
  });
}

export default Badge;
