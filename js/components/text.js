/**
 * LightUI Text 组件
 *
 * Props:
 * - size: 'xs' | 'sm' | 'md' | 'lg' | 'xl' | '2xl'
 * - weight: 'normal' | 'medium' | 'semibold' | 'bold'
 * - color: string
 * - align: 'left' | 'center' | 'right'
 * - truncate: boolean | number (行数)
 * - as: string (HTML 标签，默认 'span')
 */

import { h } from 'preact';
import { colors, fontSize, fontWeight } from './theme.js';
import { mergeStyles } from './utils.js';

const sizeMap = {
  xs: fontSize.xs,
  sm: fontSize.sm,
  md: fontSize.md,
  lg: fontSize.lg,
  xl: fontSize.xl,
  '2xl': fontSize['2xl'],
};

const weightMap = {
  normal: fontWeight.normal,
  medium: fontWeight.medium,
  semibold: fontWeight.semibold,
  bold: fontWeight.bold,
};

export function Text(props) {
  const {
    size = 'md',
    weight = 'normal',
    color,
    align,
    truncate = false,
    as = 'span',
    children,
    style,
    ...rest
  } = props;

  const textStyle = {
    fontSize: sizeMap[size] || size,
    fontWeight: weightMap[weight] || weight,
    color: color || colors.text,
    textAlign: align,
    lineHeight: '1.5',
  };

  // 单行截断
  if (truncate === true) {
    Object.assign(textStyle, {
      overflow: 'hidden',
      textOverflow: 'ellipsis',
      whiteSpace: 'nowrap',
    });
  }

  // 多行截断
  if (typeof truncate === 'number' && truncate > 1) {
    Object.assign(textStyle, {
      display: '-webkit-box',
      WebkitLineClamp: truncate,
      WebkitBoxOrient: 'vertical',
      overflow: 'hidden',
    });
  }

  return h(as, { style: mergeStyles(textStyle, style), ...rest }, children);
}

// 预设变体
Text.Title = function Title(props) {
  return h(Text, { as: 'h1', size: '2xl', weight: 'bold', ...props });
};

Text.Subtitle = function Subtitle(props) {
  return h(Text, { as: 'h2', size: 'xl', weight: 'semibold', ...props });
};

Text.Body = function Body(props) {
  return h(Text, { as: 'p', size: 'md', ...props });
};

Text.Caption = function Caption(props) {
  return h(Text, { as: 'span', size: 'sm', color: colors.textSecondary, ...props });
};

Text.Code = function Code(props) {
  const codeStyle = {
    fontFamily: 'monospace',
    backgroundColor: colors.bgSecondary,
    padding: '2px 6px',
    borderRadius: '4px',
  };
  return h(Text, { as: 'code', size: 'sm', style: codeStyle, ...props });
};

export default Text;
