/**
 * MBink Fluent Design - Divider 组件
 * 
 * @description 基于 Microsoft Fluent UI 设计规范的分割线组件
 * 
 * Props:
 * - appearance: 'default' | 'subtle' | 'brand' | 'strong'
 * - vertical: boolean
 * - inset: boolean
 * - alignContent: 'start' | 'center' | 'end'
 */

import { h } from 'preact';
import { colors, fontFamily, fontSize, spacing } from './theme.js';
import { mergeStyles } from './utils.js';

// 外观配置
const appearanceStyles = {
  default: {
    borderColor: colors.colorNeutralStroke2,
    color: colors.colorNeutralForeground2,
  },
  subtle: {
    borderColor: colors.colorNeutralStroke3 || colors.colorNeutralStroke2,
    color: colors.colorNeutralForeground3,
  },
  brand: {
    borderColor: colors.colorBrandStroke1,
    color: colors.colorBrandForeground1,
  },
  strong: {
    borderColor: colors.colorNeutralStroke1,
    color: colors.colorNeutralForeground1,
  },
};

export function Divider(props) {
  const {
    appearance = 'default',
    vertical = false,
    inset = false,
    alignContent = 'center',
    children,
    style,
    ...rest
  } = props;

  const config = appearanceStyles[appearance] || appearanceStyles.default;
  const hasContent = children !== undefined && children !== null;

  // 基础容器样式
  const containerStyle = {
    display: 'flex',
    alignItems: 'center',
    fontFamily: fontFamily.base,
    fontSize: fontSize.base200,
    color: config.color,
    ...(vertical ? {
      flexDirection: 'column',
      height: '100%',
      minHeight: '20px',
      padding: inset ? `${spacing.m} 0` : '0',
    } : {
      flexDirection: 'row',
      width: '100%',
      padding: inset ? `0 ${spacing.m}` : '0',
    }),
  };

  // 线条样式
  const lineStyle = {
    flex: 1,
    ...(vertical ? {
      width: '1px',
      minHeight: '8px',
      borderLeft: `1px solid ${config.borderColor}`,
    } : {
      height: '1px',
      minWidth: '8px',
      borderTop: `1px solid ${config.borderColor}`,
    }),
  };

  // 内容样式
  const contentStyle = {
    padding: vertical ? `${spacing.s} 0` : `0 ${spacing.m}`,
    whiteSpace: 'nowrap',
    lineHeight: '1',
  };

  // 根据对齐方式调整线条
  const getLines = () => {
    if (!hasContent) {
      return [h('span', { key: 'line', style: lineStyle })];
    }

    const beforeLine = h('span', { 
      key: 'before', 
      style: {
        ...lineStyle,
        flex: alignContent === 'start' ? 0 : alignContent === 'end' ? 1 : 1,
        minWidth: alignContent === 'start' ? '0' : undefined,
        minHeight: alignContent === 'start' ? '0' : undefined,
      }
    });

    const afterLine = h('span', { 
      key: 'after', 
      style: {
        ...lineStyle,
        flex: alignContent === 'end' ? 0 : alignContent === 'start' ? 1 : 1,
        minWidth: alignContent === 'end' ? '0' : undefined,
        minHeight: alignContent === 'end' ? '0' : undefined,
      }
    });

    const content = h('span', { key: 'content', style: contentStyle }, children);

    return [beforeLine, content, afterLine];
  };

  return h('div', {
    role: 'separator',
    'aria-orientation': vertical ? 'vertical' : 'horizontal',
    style: mergeStyles(containerStyle, style),
    ...rest,
  }, getLines());
}

export default Divider;
