/**
 * MBink Fluent Design 主题系统
 * 基于 Microsoft Fluent UI Design Tokens
 * @see https://fluent2.microsoft.design/
 */

// ============================================
// 颜色系统 - Fluent UI Color Tokens
// ============================================

export const colorPalette = {
  // Brand Colors (默认蓝色主题)
  brand10: '#061724',
  brand20: '#082338',
  brand30: '#0a2e4a',
  brand40: '#0c3b5e',
  brand50: '#0e4775',
  brand60: '#0f548c',
  brand70: '#115ea3',
  brand80: '#0f6cbd',  // Primary
  brand90: '#2886de',
  brand100: '#479ef5',
  brand110: '#62abf5',
  brand120: '#77b7f7',
  brand130: '#96c6fa',
  brand140: '#b4d6fa',
  brand150: '#cfe4fa',
  brand160: '#ebf3fc',

  // Neutral Colors
  grey10: '#050505',
  grey20: '#161616',
  grey30: '#212121',
  grey40: '#292929',
  grey50: '#3d3d3d',
  grey60: '#5c5c5c',
  grey70: '#6e6e6e',
  grey80: '#8a8a8a',
  grey90: '#a3a3a3',
  grey100: '#b3b3b3',
  grey110: '#c4c4c4',
  grey120: '#d1d1d1',
  grey130: '#e0e0e0',
  grey140: '#ebebeb',
  grey150: '#f5f5f5',
  grey160: '#fafafa',

  // Status Colors
  red10: '#a80000',
  red20: '#c50f1f',
  green10: '#0b6a0b',
  green20: '#107c10',
  yellow10: '#835b00',
  yellow20: '#c19c00',
};

// ============================================
// 语义化颜色 - Light Theme
// ============================================

export const colors = {
  // Brand
  colorBrandBackground: colorPalette.brand80,
  colorBrandBackgroundHover: colorPalette.brand70,
  colorBrandBackgroundPressed: colorPalette.brand60,
  colorBrandBackgroundSelected: colorPalette.brand70,
  colorBrandForeground1: colorPalette.brand80,
  colorBrandForeground2: colorPalette.brand70,

  // Neutral Background
  colorNeutralBackground1: '#ffffff',
  colorNeutralBackground1Hover: colorPalette.grey140,
  colorNeutralBackground1Pressed: colorPalette.grey130,
  colorNeutralBackground1Selected: colorPalette.grey140,
  colorNeutralBackground2: colorPalette.grey150,
  colorNeutralBackground3: colorPalette.grey140,
  colorNeutralBackground4: colorPalette.grey130,
  colorNeutralBackground5: colorPalette.grey120,
  colorNeutralBackground6: colorPalette.grey110,
  colorNeutralBackgroundDisabled: colorPalette.grey140,

  // Neutral Foreground
  colorNeutralForeground1: colorPalette.grey10,
  colorNeutralForeground2: colorPalette.grey50,
  colorNeutralForeground3: colorPalette.grey60,
  colorNeutralForeground4: colorPalette.grey80,
  colorNeutralForegroundDisabled: colorPalette.grey90,
  colorNeutralForegroundOnBrand: '#ffffff',
  colorNeutralForegroundInverted: '#ffffff',

  // Neutral Stroke
  colorNeutralStroke1: colorPalette.grey110,
  colorNeutralStroke1Hover: colorPalette.grey100,
  colorNeutralStroke1Pressed: colorPalette.grey90,
  colorNeutralStroke2: colorPalette.grey130,
  colorNeutralStrokeAccessible: colorPalette.grey60,
  colorNeutralStrokeDisabled: colorPalette.grey130,

  // Brand Stroke
  colorBrandStroke1: colorPalette.brand80,
  colorBrandStroke2: colorPalette.brand140,

  // Status - Danger
  colorStatusDangerBackground1: '#fdf3f4',
  colorStatusDangerBackground2: colorPalette.red20,
  colorStatusDangerForeground1: colorPalette.red20,
  colorStatusDangerForeground2: colorPalette.red10,
  colorStatusDangerBorder1: colorPalette.red20,

  // Status - Success
  colorStatusSuccessBackground1: '#f1faf1',
  colorStatusSuccessBackground2: colorPalette.green20,
  colorStatusSuccessForeground1: colorPalette.green20,
  colorStatusSuccessForeground2: colorPalette.green10,
  colorStatusSuccessBorder1: colorPalette.green20,

  // Status - Warning
  colorStatusWarningBackground1: '#fff9f5',
  colorStatusWarningBackground2: colorPalette.yellow20,
  colorStatusWarningForeground1: colorPalette.yellow10,
  colorStatusWarningForeground2: colorPalette.yellow10,
  colorStatusWarningBorder1: colorPalette.yellow20,

  // Subtle
  colorSubtleBackground: 'transparent',
  colorSubtleBackgroundHover: colorPalette.grey140,
  colorSubtleBackgroundPressed: colorPalette.grey130,
  colorSubtleBackgroundSelected: colorPalette.grey140,

  // Transparent
  colorTransparentBackground: 'transparent',
  colorTransparentBackgroundHover: 'transparent',
  colorTransparentBackgroundPressed: 'transparent',

  // Compound Brand
  colorCompoundBrandBackground: colorPalette.brand80,
  colorCompoundBrandBackgroundHover: colorPalette.brand70,
  colorCompoundBrandBackgroundPressed: colorPalette.brand60,
  colorCompoundBrandForeground1: colorPalette.brand80,
  colorCompoundBrandForeground1Hover: colorPalette.brand70,
  colorCompoundBrandForeground1Pressed: colorPalette.brand60,
  colorCompoundBrandStroke: colorPalette.brand80,
  colorCompoundBrandStrokeHover: colorPalette.brand70,
  colorCompoundBrandStrokePressed: colorPalette.brand60,

  // Focus
  colorStrokeFocus1: '#ffffff',
  colorStrokeFocus2: colorPalette.grey10,
};

// ============================================
// 圆角 - Border Radius
// ============================================

export const borderRadius = {
  none: '0',
  small: '2px',
  medium: '4px',
  large: '6px',
  xLarge: '8px',
  circular: '9999px',
};

// ============================================
// 阴影 - Shadows
// ============================================

export const shadow = {
  shadow2: '0 0 2px rgba(0,0,0,0.12), 0 1px 2px rgba(0,0,0,0.14)',
  shadow4: '0 0 2px rgba(0,0,0,0.12), 0 2px 4px rgba(0,0,0,0.14)',
  shadow8: '0 0 2px rgba(0,0,0,0.12), 0 4px 8px rgba(0,0,0,0.14)',
  shadow16: '0 0 2px rgba(0,0,0,0.12), 0 8px 16px rgba(0,0,0,0.14)',
  shadow28: '0 0 8px rgba(0,0,0,0.12), 0 14px 28px rgba(0,0,0,0.24)',
  shadow64: '0 0 8px rgba(0,0,0,0.12), 0 32px 64px rgba(0,0,0,0.24)',

  // Brand shadows
  shadow2Brand: '0 0 2px rgba(0,0,0,0.30), 0 1px 2px rgba(0,0,0,0.25)',
  shadow4Brand: '0 0 2px rgba(0,0,0,0.30), 0 2px 4px rgba(0,0,0,0.25)',
  shadow8Brand: '0 0 2px rgba(0,0,0,0.30), 0 4px 8px rgba(0,0,0,0.25)',
  shadow16Brand: '0 0 2px rgba(0,0,0,0.30), 0 8px 16px rgba(0,0,0,0.25)',
  shadow28Brand: '0 0 8px rgba(0,0,0,0.30), 0 14px 28px rgba(0,0,0,0.35)',
};

// ============================================
// 间距 - Spacing
// ============================================

export const spacing = {
  none: '0',
  xxs: '2px',
  xs: '4px',
  sNudge: '6px',
  s: '8px',
  mNudge: '10px',
  m: '12px',
  l: '16px',
  xl: '20px',
  xxl: '24px',
  xxxl: '32px',
};

// ============================================
// 字体 - Typography
// ============================================

export const fontFamily = {
  base: "'Segoe UI', 'Segoe UI Web (West European)', -apple-system, BlinkMacSystemFont, Roboto, 'Helvetica Neue', sans-serif",
  monospace: "Consolas, 'Courier New', Courier, monospace",
  numeric: "'Segoe UI', 'Segoe UI Web (West European)', -apple-system, BlinkMacSystemFont, Roboto, 'Helvetica Neue', sans-serif",
};

export const fontSize = {
  base100: '10px',
  base200: '12px',
  base300: '14px',
  base400: '16px',
  base500: '20px',
  base600: '24px',
  hero700: '28px',
  hero800: '32px',
  hero900: '40px',
  hero1000: '68px',
};

export const fontWeight = {
  regular: '400',
  medium: '500',
  semibold: '600',
  bold: '700',
};

export const lineHeight = {
  base100: '14px',
  base200: '16px',
  base300: '20px',
  base400: '22px',
  base500: '28px',
  base600: '32px',
  hero700: '36px',
  hero800: '40px',
  hero900: '52px',
  hero1000: '92px',
};

// ============================================
// 组件尺寸
// ============================================

export const componentSizes = {
  small: {
    height: '24px',
    minWidth: '64px',
    fontSize: fontSize.base200,
    padding: `${spacing.xs} ${spacing.s}`,
    iconSize: '16px',
    gap: spacing.xs,
  },
  medium: {
    height: '32px',
    minWidth: '96px',
    fontSize: fontSize.base300,
    padding: `${spacing.sNudge} ${spacing.m}`,
    iconSize: '20px',
    gap: spacing.sNudge,
  },
  large: {
    height: '40px',
    minWidth: '96px',
    fontSize: fontSize.base400,
    padding: `${spacing.s} ${spacing.l}`,
    iconSize: '24px',
    gap: spacing.s,
  },
};

// ============================================
// 动画时长
// ============================================

export const duration = {
  ultraFast: '50ms',
  faster: '100ms',
  fast: '150ms',
  normal: '200ms',
  slow: '300ms',
  slower: '400ms',
  ultraSlow: '500ms',
};

// ============================================
// 缓动函数
// ============================================

export const easing = {
  accelerate: 'cubic-bezier(0.9, 0.1, 1, 0.2)',
  decelerate: 'cubic-bezier(0.1, 0.9, 0.2, 1)',
  linear: 'cubic-bezier(0, 0, 1, 1)',
  standard: 'cubic-bezier(0.33, 0, 0.67, 1)',
};

// ============================================
// 过渡
// ============================================

export const transition = {
  fast: `${duration.fast} ${easing.standard}`,
  normal: `${duration.normal} ${easing.standard}`,
  slow: `${duration.slow} ${easing.standard}`,
};

// ============================================
// 默认主题导出
// ============================================

const theme = {
  colorPalette,
  colors,
  borderRadius,
  shadow,
  spacing,
  fontFamily,
  fontSize,
  fontWeight,
  lineHeight,
  componentSizes,
  duration,
  easing,
  transition,
};

export default theme;
