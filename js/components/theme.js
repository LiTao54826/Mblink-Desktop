/**
 * MBlink 主题系统
 */

export const colors = {
  // 主色
  primary: '#3b82f6',
  primaryHover: '#2563eb',
  primaryActive: '#1d4ed8',

  // 次要色
  secondary: '#64748b',
  secondaryHover: '#475569',

  // 状态色
  success: '#22c55e',
  successHover: '#16a34a',
  warning: '#f59e0b',
  warningHover: '#d97706',
  error: '#ef4444',
  errorHover: '#dc2626',
  info: '#3b82f6',

  // 背景
  bg: '#ffffff',
  bgSecondary: '#f8fafc',
  bgHover: '#f1f5f9',
  bgActive: '#e2e8f0',

  // 文字
  text: '#1e293b',
  textSecondary: '#64748b',
  textTertiary: '#94a3b8',
  textDisabled: '#cbd5e1',
  textInverse: '#ffffff',

  // 边框
  border: '#e2e8f0',
  borderHover: '#cbd5e1',
  borderFocus: '#3b82f6',

  // 遮罩
  mask: 'rgba(0, 0, 0, 0.45)',
};

export const radius = {
  none: '0',
  sm: '4px',
  md: '6px',
  lg: '8px',
  xl: '12px',
  full: '9999px',
};

export const shadow = {
  none: 'none',
  sm: '0 1px 2px rgba(0, 0, 0, 0.05)',
  md: '0 4px 6px -1px rgba(0, 0, 0, 0.1)',
  lg: '0 10px 15px -3px rgba(0, 0, 0, 0.1)',
  xl: '0 20px 25px -5px rgba(0, 0, 0, 0.1)',
};

export const space = {
  0: '0',
  1: '4px',
  2: '8px',
  3: '12px',
  4: '16px',
  5: '20px',
  6: '24px',
  8: '32px',
  10: '40px',
  12: '48px',
};

export const fontSize = {
  xs: '12px',
  sm: '14px',
  md: '16px',
  lg: '18px',
  xl: '20px',
  '2xl': '24px',
};

export const fontWeight = {
  normal: '400',
  medium: '500',
  semibold: '600',
  bold: '700',
};

export const lineHeight = {
  tight: '1.25',
  normal: '1.5',
  relaxed: '1.75',
};

// 组件尺寸
export const sizes = {
  sm: {
    height: '28px',
    fontSize: '12px',
    padding: '4px 10px',
    iconSize: '14px',
  },
  md: {
    height: '36px',
    fontSize: '14px',
    padding: '8px 16px',
    iconSize: '16px',
  },
  lg: {
    height: '44px',
    fontSize: '16px',
    padding: '12px 20px',
    iconSize: '18px',
  },
};

// 过渡动画
export const transition = {
  fast: '0.1s ease',
  normal: '0.2s ease',
  slow: '0.3s ease',
};

// 默认主题
const theme = {
  colors,
  radius,
  shadow,
  space,
  fontSize,
  fontWeight,
  lineHeight,
  sizes,
  transition,
};

export default theme;
