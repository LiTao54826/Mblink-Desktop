/**
 * MBink Fluent Design - Text 组件
 * 
 * @description 基于 Microsoft Fluent UI 设计规范的文本组件
 * 
 * Props:
 * - size: 100 | 200 | 300 | 400 | 500 | 600 | 700 | 800 | 900 | 1000
 * - weight: 'regular' | 'medium' | 'semibold' | 'bold'
 * - font: 'base' | 'monospace' | 'numeric'
 * - italic: boolean
 * - underline: boolean
 * - strikethrough: boolean
 * - truncate: boolean
 * - block: boolean
 * - align: 'start' | 'center' | 'end' | 'justify'
 * - as: string (HTML 标签)
 */

import { h } from 'preact';
import { colors, fontFamily, fontSize, fontWeight, lineHeight } from './theme.js';
import { mergeStyles } from './utils.js';

// 尺寸映射
const sizeMap = {
  100: { fontSize: fontSize.base100, lineHeight: lineHeight.base100 },
  200: { fontSize: fontSize.base200, lineHeight: lineHeight.base200 },
  300: { fontSize: fontSize.base300, lineHeight: lineHeight.base300 },
  400: { fontSize: fontSize.base400, lineHeight: lineHeight.base400 },
  500: { fontSize: fontSize.base500, lineHeight: lineHeight.base500 },
  600: { fontSize: fontSize.base600, lineHeight: lineHeight.base600 },
  700: { fontSize: fontSize.hero700, lineHeight: lineHeight.hero700 },
  800: { fontSize: fontSize.hero800, lineHeight: lineHeight.hero800 },
  900: { fontSize: fontSize.hero900, lineHeight: lineHeight.hero900 },
  1000: { fontSize: fontSize.hero1000, lineHeight: lineHeight.hero1000 },
};

// 字重映射
const weightMap = {
  regular: fontWeight.regular,
  medium: fontWeight.medium,
  semibold: fontWeight.semibold,
  bold: fontWeight.bold,
};

// 字体映射
const fontMap = {
  base: fontFamily.base,
  monospace: fontFamily.monospace,
  numeric: fontFamily.numeric,
};

export function Text(props) {
  const {
    size = 300,
    weight = 'regular',
    font = 'base',
    italic = false,
    underline = false,
    strikethrough = false,
    truncate = false,
    block = false,
    align,
    as = 'span',
    children,
    style,
    ...rest
  } = props;

  const sizeConfig = sizeMap[size] || sizeMap[300];

  const textStyle = {
    fontFamily: fontMap[font] || fontMap.base,
    fontSize: sizeConfig.fontSize,
    lineHeight: sizeConfig.lineHeight,
    fontWeight: weightMap[weight] || weightMap.regular,
    fontStyle: italic ? 'italic' : 'normal',
    textDecoration: [
      underline && 'underline',
      strikethrough && 'line-through',
    ].filter(Boolean).join(' ') || 'none',
    color: colors.colorNeutralForeground1,
    margin: 0,
    ...(block && { display: 'block' }),
    ...(align && { textAlign: align }),
    ...(truncate && {
      overflow: 'hidden',
      textOverflow: 'ellipsis',
      whiteSpace: 'nowrap',
    }),
  };

  return h(as, {
    style: mergeStyles(textStyle, style),
    ...rest,
  }, children);
}

/**
 * Title - 标题组件
 */
export function Title(props) {
  const { size = 500, as = 'h2', ...rest } = props;
  return h(Text, { size, weight: 'semibold', as, block: true, ...rest });
}

/**
 * Subtitle - 副标题组件
 */
export function Subtitle(props) {
  const { size = 400, as = 'h3', ...rest } = props;
  return h(Text, { size, weight: 'semibold', as, block: true, ...rest });
}

/**
 * Body - 正文组件
 */
export function Body(props) {
  const { size = 300, as = 'p', ...rest } = props;
  return h(Text, { size, as, block: true, ...rest });
}

/**
 * Caption - 说明文字组件
 */
export function Caption(props) {
  const { size = 200, ...rest } = props;
  return h(Text, { 
    size, 
    style: { color: colors.colorNeutralForeground2 },
    ...rest 
  });
}

/**
 * LargeTitle - 大标题
 */
export function LargeTitle(props) {
  const { as = 'h1', ...rest } = props;
  return h(Text, { size: 700, weight: 'semibold', as, block: true, ...rest });
}

/**
 * Display - 展示文字
 */
export function Display(props) {
  const { as = 'h1', ...rest } = props;
  return h(Text, { size: 900, weight: 'semibold', as, block: true, ...rest });
}

export default Text;
