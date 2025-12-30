/**
 * MBink Fluent Design - Avatar 组件
 * 
 * @description 基于 Microsoft Fluent UI 设计规范的头像组件
 * 
 * Props:
 * - name: string (用于生成首字母和颜色)
 * - image: string (图片 URL)
 * - initials: string (自定义首字母)
 * - icon: VNode (自定义图标)
 * - size: 16 | 20 | 24 | 28 | 32 | 36 | 40 | 48 | 56 | 64 | 72 | 96 | 120 | 128
 * - shape: 'circular' | 'square'
 * - color: 'neutral' | 'brand' | 'colorful' | 具体颜色名
 * - active: 'active' | 'inactive' | 'unset'
 * - activeAppearance: 'ring' | 'shadow' | 'ring-shadow'
 * - badge: VNode (状态徽章)
 */

import { h } from 'preact';
import { 
  colors, colorPalette, borderRadius, shadow, fontFamily, fontWeight 
} from './theme.js';
import { mergeStyles } from './utils.js';

// 预定义的头像颜色
const avatarColors = [
  { background: '#750B1C', foreground: '#ffffff' }, // darkRed
  { background: '#A4262C', foreground: '#ffffff' }, // cranberry
  { background: '#D13438', foreground: '#ffffff' }, // red
  { background: '#CA5010', foreground: '#ffffff' }, // pumpkin
  { background: '#8A8886', foreground: '#ffffff' }, // peach (using grey)
  { background: '#986F0B', foreground: '#ffffff' }, // marigold
  { background: '#498205', foreground: '#ffffff' }, // gold (using green)
  { background: '#0B6A0B', foreground: '#ffffff' }, // brass (using green)
  { background: '#107C10', foreground: '#ffffff' }, // forest
  { background: '#00AD56', foreground: '#ffffff' }, // seafoam
  { background: '#038387', foreground: '#ffffff' }, // darkGreen
  { background: '#005B70', foreground: '#ffffff' }, // lightTeal
  { background: '#0078D4', foreground: '#ffffff' }, // teal
  { background: '#004E8C', foreground: '#ffffff' }, // steel
  { background: '#5C2D91', foreground: '#ffffff' }, // blue
  { background: '#8764B8', foreground: '#ffffff' }, // royalBlue
  { background: '#881798', foreground: '#ffffff' }, // cornflower
  { background: '#C239B3', foreground: '#ffffff' }, // navy
  { background: '#E3008C', foreground: '#ffffff' }, // lavender
];

// 尺寸配置
const sizeConfig = {
  16: { fontSize: '8px', iconSize: '10px' },
  20: { fontSize: '10px', iconSize: '12px' },
  24: { fontSize: '10px', iconSize: '14px' },
  28: { fontSize: '12px', iconSize: '16px' },
  32: { fontSize: '12px', iconSize: '18px' },
  36: { fontSize: '14px', iconSize: '20px' },
  40: { fontSize: '14px', iconSize: '20px' },
  48: { fontSize: '16px', iconSize: '24px' },
  56: { fontSize: '20px', iconSize: '28px' },
  64: { fontSize: '24px', iconSize: '32px' },
  72: { fontSize: '28px', iconSize: '36px' },
  96: { fontSize: '36px', iconSize: '48px' },
  120: { fontSize: '48px', iconSize: '64px' },
  128: { fontSize: '52px', iconSize: '68px' },
};

// 从名字生成首字母
function getInitials(name) {
  if (!name) return '';
  const parts = name.trim().split(/\s+/);
  if (parts.length === 1) {
    return parts[0].charAt(0).toUpperCase();
  }
  return (parts[0].charAt(0) + parts[parts.length - 1].charAt(0)).toUpperCase();
}

// 从名字生成颜色索引
function getColorIndex(name) {
  if (!name) return 0;
  let hash = 0;
  for (let i = 0; i < name.length; i++) {
    hash = name.charCodeAt(i) + ((hash << 5) - hash);
  }
  return Math.abs(hash) % avatarColors.length;
}

// 默认人物图标
const PersonIcon = ({ size = 20 }) => h('svg', {
  width: size,
  height: size,
  viewBox: '0 0 20 20',
  fill: 'currentColor',
}, h('path', {
  d: 'M10 2a4 4 0 1 0 0 8 4 4 0 0 0 0-8ZM7 6a3 3 0 1 1 6 0 3 3 0 0 1-6 0Zm-1.991 5A2.001 2.001 0 0 0 3 13c0 1.691.833 2.966 2.135 3.797C6.417 17.614 8.145 18 10 18c1.855 0 3.583-.386 4.865-1.203C16.167 15.967 17 14.69 17 13a2 2 0 0 0-2-2H5.009ZM4 13c0-.553.448-1 1.009-1H15a1 1 0 0 1 1 1c0 1.309-.622 2.284-1.673 2.953C13.257 16.636 11.735 17 10 17c-1.735 0-3.257-.364-4.327-1.047C4.623 15.283 4 14.31 4 13Z',
}));

export function Avatar(props) {
  const {
    name,
    image,
    initials,
    icon,
    size = 32,
    shape = 'circular',
    color = 'colorful',
    active = 'unset',
    activeAppearance = 'ring',
    badge,
    style,
    ...rest
  } = props;

  const config = sizeConfig[size] || sizeConfig[32];
  const displayInitials = initials || getInitials(name);

  // 获取颜色
  let backgroundColor, foregroundColor;
  if (color === 'neutral') {
    backgroundColor = colors.colorNeutralBackground6;
    foregroundColor = colors.colorNeutralForeground3;
  } else if (color === 'brand') {
    backgroundColor = colors.colorBrandBackground;
    foregroundColor = colors.colorNeutralForegroundOnBrand;
  } else if (color === 'colorful') {
    const colorIndex = getColorIndex(name);
    backgroundColor = avatarColors[colorIndex].background;
    foregroundColor = avatarColors[colorIndex].foreground;
  } else {
    backgroundColor = colors.colorNeutralBackground6;
    foregroundColor = colors.colorNeutralForeground3;
  }

  // 基础样式
  const avatarStyle = {
    position: 'relative',
    display: 'inline-flex',
    alignItems: 'center',
    justifyContent: 'center',
    width: `${size}px`,
    height: `${size}px`,
    borderRadius: shape === 'circular' ? borderRadius.circular : borderRadius.medium,
    backgroundColor: image ? 'transparent' : backgroundColor,
    color: foregroundColor,
    fontSize: config.fontSize,
    fontFamily: fontFamily.base,
    fontWeight: fontWeight.semibold,
    overflow: 'hidden',
    flexShrink: 0,
    userSelect: 'none',
  };

  // 活动状态样式
  if (active === 'active') {
    if (activeAppearance.includes('ring')) {
      avatarStyle.outline = `2px solid ${colors.colorBrandStroke1}`;
      avatarStyle.outlineOffset = '2px';
    }
    if (activeAppearance.includes('shadow')) {
      avatarStyle.boxShadow = shadow.shadow8Brand;
    }
  } else if (active === 'inactive') {
    avatarStyle.opacity = 0.5;
  }

  // 图片样式
  const imageStyle = {
    width: '100%',
    height: '100%',
    objectFit: 'cover',
  };

  // 徽章位置样式
  const badgeStyle = {
    position: 'absolute',
    bottom: 0,
    right: 0,
    transform: 'translate(25%, 25%)',
  };

  // 渲染内容
  let content;
  if (image) {
    content = h('img', { src: image, alt: name || '', style: imageStyle });
  } else if (icon) {
    content = h('span', { 
      style: { 
        display: 'flex', 
        alignItems: 'center', 
        justifyContent: 'center',
        fontSize: config.iconSize,
      } 
    }, icon);
  } else if (displayInitials) {
    content = displayInitials;
  } else {
    content = h(PersonIcon, { size: parseInt(config.iconSize) });
  }

  return h('span', {
    role: 'img',
    'aria-label': name,
    style: mergeStyles(avatarStyle, style),
    ...rest,
  }, [
    content,
    badge && h('span', { key: 'badge', style: badgeStyle }, badge),
  ]);
}

/**
 * AvatarGroup - 头像组
 */
export function AvatarGroup(props) {
  const {
    layout = 'spread', // spread | stack | pie
    size = 32,
    maxAvatars = 5,
    children,
    style,
    ...rest
  } = props;

  const containerStyle = {
    display: 'inline-flex',
    alignItems: 'center',
  };

  if (layout === 'stack') {
    containerStyle.flexDirection = 'row-reverse';
  }

  // 处理子元素
  const avatars = Array.isArray(children) ? children : [children];
  const visibleAvatars = avatars.slice(0, maxAvatars);
  const overflowCount = avatars.length - maxAvatars;

  const avatarElements = visibleAvatars.map((avatar, index) => {
    const avatarStyle = layout === 'stack' ? {
      marginLeft: index > 0 ? `-${size * 0.25}px` : '0',
      zIndex: visibleAvatars.length - index,
      border: `2px solid ${colors.colorNeutralBackground1}`,
      borderRadius: borderRadius.circular,
    } : {
      marginLeft: index > 0 ? '4px' : '0',
    };

    return h('span', { key: index, style: avatarStyle }, avatar);
  });

  // 溢出指示器
  if (overflowCount > 0) {
    const overflowStyle = {
      display: 'inline-flex',
      alignItems: 'center',
      justifyContent: 'center',
      width: `${size}px`,
      height: `${size}px`,
      borderRadius: borderRadius.circular,
      backgroundColor: colors.colorNeutralBackground5,
      color: colors.colorNeutralForeground2,
      fontSize: sizeConfig[size]?.fontSize || '12px',
      fontFamily: fontFamily.base,
      fontWeight: fontWeight.semibold,
      marginLeft: layout === 'stack' ? `-${size * 0.25}px` : '4px',
      border: layout === 'stack' ? `2px solid ${colors.colorNeutralBackground1}` : 'none',
    };

    avatarElements.push(
      h('span', { key: 'overflow', style: overflowStyle }, `+${overflowCount}`)
    );
  }

  return h('div', {
    role: 'group',
    style: mergeStyles(containerStyle, style),
    ...rest,
  }, avatarElements);
}

export default Avatar;
