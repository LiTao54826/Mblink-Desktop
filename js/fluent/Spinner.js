/**
 * MBink Fluent Design - Spinner 组件
 * 
 * @description 基于 Microsoft Fluent UI 设计规范的加载指示器组件
 * 
 * Props:
 * - appearance: 'primary' | 'inverted'
 * - size: 'tiny' | 'extraSmall' | 'small' | 'medium' | 'large' | 'extraLarge' | 'huge'
 * - label: string
 * - labelPosition: 'before' | 'after' | 'above' | 'below'
 */

import { h } from 'preact';
import { 
  colors, fontFamily, fontSize, spacing 
} from './theme.js';
import { mergeStyles } from './utils.js';

// 尺寸配置
const sizeStyles = {
  tiny: {
    size: '16px',
    strokeWidth: '1.5px',
    fontSize: fontSize.base100,
    gap: spacing.xs,
  },
  extraSmall: {
    size: '20px',
    strokeWidth: '1.5px',
    fontSize: fontSize.base200,
    gap: spacing.xs,
  },
  small: {
    size: '24px',
    strokeWidth: '2px',
    fontSize: fontSize.base200,
    gap: spacing.s,
  },
  medium: {
    size: '32px',
    strokeWidth: '2px',
    fontSize: fontSize.base300,
    gap: spacing.s,
  },
  large: {
    size: '36px',
    strokeWidth: '2.5px',
    fontSize: fontSize.base300,
    gap: spacing.mNudge,
  },
  extraLarge: {
    size: '40px',
    strokeWidth: '3px',
    fontSize: fontSize.base400,
    gap: spacing.m,
  },
  huge: {
    size: '44px',
    strokeWidth: '3px',
    fontSize: fontSize.base400,
    gap: spacing.m,
  },
};

// 外观配置
const appearanceStyles = {
  primary: {
    trackColor: colors.colorNeutralBackground6,
    tailColor: colors.colorBrandBackground,
  },
  inverted: {
    trackColor: 'rgba(255, 255, 255, 0.2)',
    tailColor: colors.colorNeutralForegroundOnBrand,
  },
};

export function Spinner(props) {
  const {
    appearance = 'primary',
    size = 'medium',
    label,
    labelPosition = 'after',
    style,
    ...rest
  } = props;

  const sizeConfig = sizeStyles[size] || sizeStyles.medium;
  const appearanceConfig = appearanceStyles[appearance] || appearanceStyles.primary;

  // 容器样式
  const containerStyle = {
    display: 'inline-flex',
    alignItems: 'center',
    justifyContent: 'center',
    gap: sizeConfig.gap,
    flexDirection: 
      labelPosition === 'above' ? 'column-reverse' :
      labelPosition === 'below' ? 'column' :
      labelPosition === 'before' ? 'row-reverse' : 'row',
  };

  // Spinner 圆环样式
  const spinnerSize = parseInt(sizeConfig.size);
  const strokeWidth = parseFloat(sizeConfig.strokeWidth);
  const radius = (spinnerSize - strokeWidth) / 2;
  const circumference = 2 * Math.PI * radius;

  // SVG 样式
  const svgStyle = {
    width: sizeConfig.size,
    height: sizeConfig.size,
    // 注意：MBink 目前不支持 CSS animation
    // 这里使用静态显示，实际动画需要引擎支持
  };

  // 标签样式
  const labelStyle = {
    fontSize: sizeConfig.fontSize,
    fontFamily: fontFamily.base,
    color: appearance === 'inverted' 
      ? colors.colorNeutralForegroundOnBrand 
      : colors.colorNeutralForeground2,
    lineHeight: '1.4',
  };

  // 创建 SVG 圆环
  const spinnerSvg = h('svg', {
    key: 'spinner',
    style: svgStyle,
    viewBox: `0 0 ${spinnerSize} ${spinnerSize}`,
  }, [
    // 背景轨道
    h('circle', {
      key: 'track',
      cx: spinnerSize / 2,
      cy: spinnerSize / 2,
      r: radius,
      fill: 'none',
      stroke: appearanceConfig.trackColor,
      strokeWidth: strokeWidth,
    }),
    // 旋转的尾巴
    h('circle', {
      key: 'tail',
      cx: spinnerSize / 2,
      cy: spinnerSize / 2,
      r: radius,
      fill: 'none',
      stroke: appearanceConfig.tailColor,
      strokeWidth: strokeWidth,
      strokeLinecap: 'round',
      strokeDasharray: `${circumference * 0.25} ${circumference * 0.75}`,
      // 静态位置，实际应该旋转
      transform: `rotate(-90 ${spinnerSize / 2} ${spinnerSize / 2})`,
    }),
  ]);

  const content = [spinnerSvg];
  
  if (label) {
    content.push(h('span', { key: 'label', style: labelStyle }, label));
  }

  return h('div', {
    role: 'progressbar',
    'aria-label': label || 'Loading',
    style: mergeStyles(containerStyle, style),
    ...rest,
  }, ...content);
}

/**
 * 简化的加载点动画
 * 使用三个点表示加载状态
 */
export function LoadingDots(props) {
  const {
    size = 'medium',
    color = colors.colorBrandBackground,
    style,
    ...rest
  } = props;

  const dotSizes = {
    small: '4px',
    medium: '6px',
    large: '8px',
  };

  const dotSize = dotSizes[size] || dotSizes.medium;

  const containerStyle = {
    display: 'inline-flex',
    alignItems: 'center',
    gap: spacing.xs,
  };

  const dotStyle = {
    width: dotSize,
    height: dotSize,
    borderRadius: '50%',
    backgroundColor: color,
  };

  return h('div', {
    style: mergeStyles(containerStyle, style),
    'aria-label': 'Loading',
    ...rest,
  }, [
    h('span', { key: 'd1', style: { ...dotStyle, opacity: 0.4 } }),
    h('span', { key: 'd2', style: { ...dotStyle, opacity: 0.7 } }),
    h('span', { key: 'd3', style: { ...dotStyle, opacity: 1 } }),
  ]);
}

export default Spinner;
