/**
 * MBink Fluent Design - Checkbox 组件
 * 
 * @description 基于 Microsoft Fluent UI 设计规范的复选框组件
 * 
 * Props:
 * - checked: boolean | 'mixed'
 * - defaultChecked: boolean
 * - disabled: boolean
 * - size: 'medium' | 'large'
 * - shape: 'square' | 'circular'
 * - label: string | VNode
 * - labelPosition: 'before' | 'after'
 * - onChange: (e, data) => void
 */

import { h } from 'preact';
import { useState } from 'preact/hooks';
import { 
  colors, borderRadius, transition, fontFamily, fontSize, spacing 
} from './theme.js';
import { mergeStyles } from './utils.js';

// 尺寸配置
const sizeConfig = {
  medium: {
    boxSize: '16px',
    fontSize: fontSize.base300,
    iconSize: '12px',
    gap: spacing.s,
  },
  large: {
    boxSize: '20px',
    fontSize: fontSize.base400,
    iconSize: '14px',
    gap: spacing.mNudge,
  },
};

// 勾选图标 SVG
const CheckIcon = ({ size = 12 }) => h('svg', {
  width: size,
  height: size,
  viewBox: '0 0 12 12',
  fill: 'none',
  style: { display: 'block' },
}, h('path', {
  d: 'M9.76 3.2a.75.75 0 0 1 .04 1.06l-4.25 4.5a.75.75 0 0 1-1.08.02L2.22 6.53a.75.75 0 0 1 1.06-1.06l1.7 1.7 3.72-3.93a.75.75 0 0 1 1.06-.04z',
  fill: 'currentColor',
}));

// 混合状态图标
const MixedIcon = ({ size = 12 }) => h('svg', {
  width: size,
  height: size,
  viewBox: '0 0 12 12',
  fill: 'none',
  style: { display: 'block' },
}, h('path', {
  d: 'M2 6a.75.75 0 0 1 .75-.75h6.5a.75.75 0 0 1 0 1.5h-6.5A.75.75 0 0 1 2 6z',
  fill: 'currentColor',
}));

export function Checkbox(props) {
  const {
    checked,
    defaultChecked = false,
    disabled = false,
    size = 'medium',
    shape = 'square',
    label,
    labelPosition = 'after',
    onChange,
    style,
    ...rest
  } = props;

  const [internalChecked, setInternalChecked] = useState(defaultChecked);
  const [hovered, setHovered] = useState(false);
  const [pressed, setPressed] = useState(false);

  const isControlled = checked !== undefined;
  const isChecked = isControlled ? checked : internalChecked;
  const isMixed = isChecked === 'mixed';
  const isSelected = isChecked === true || isMixed;

  const config = sizeConfig[size] || sizeConfig.medium;

  // 容器样式
  const containerStyle = {
    display: 'inline-flex',
    alignItems: 'center',
    gap: config.gap,
    cursor: disabled ? 'not-allowed' : 'pointer',
    userSelect: 'none',
    opacity: disabled ? 0.5 : 1,
  };

  // 复选框样式
  const getBoxStyle = () => {
    const base = {
      width: config.boxSize,
      height: config.boxSize,
      borderRadius: shape === 'circular' ? borderRadius.circular : borderRadius.small,
      border: `1px solid ${colors.colorNeutralStrokeAccessible}`,
      backgroundColor: colors.colorNeutralBackground1,
      display: 'flex',
      alignItems: 'center',
      justifyContent: 'center',
      transition: `all ${transition.fast}`,
      flexShrink: 0,
      boxSizing: 'border-box',
    };

    if (disabled) {
      return {
        ...base,
        borderColor: colors.colorNeutralStrokeDisabled,
        backgroundColor: isSelected 
          ? colors.colorNeutralBackgroundDisabled 
          : colors.colorNeutralBackground1,
      };
    }

    if (isSelected) {
      return {
        ...base,
        backgroundColor: pressed 
          ? colors.colorCompoundBrandBackgroundPressed
          : hovered 
            ? colors.colorCompoundBrandBackgroundHover
            : colors.colorCompoundBrandBackground,
        borderColor: pressed 
          ? colors.colorCompoundBrandStrokePressed
          : hovered 
            ? colors.colorCompoundBrandStrokeHover
            : colors.colorCompoundBrandStroke,
        color: colors.colorNeutralForegroundOnBrand,
      };
    }

    return {
      ...base,
      borderColor: hovered 
        ? colors.colorNeutralStroke1Hover 
        : colors.colorNeutralStrokeAccessible,
      backgroundColor: hovered 
        ? colors.colorNeutralBackground1Hover 
        : colors.colorNeutralBackground1,
    };
  };

  // 标签样式
  const labelStyle = {
    fontSize: config.fontSize,
    fontFamily: fontFamily.base,
    color: disabled ? colors.colorNeutralForegroundDisabled : colors.colorNeutralForeground1,
    lineHeight: '1.4',
  };

  const handleClick = (e) => {
    if (disabled) return;
    
    const newValue = isMixed ? true : !isChecked;
    if (!isControlled) {
      setInternalChecked(newValue);
    }
    onChange?.(e, { checked: newValue });
  };

  // 渲染图标
  const renderIcon = () => {
    if (!isSelected) return null;
    if (isMixed) {
      return h(MixedIcon, { size: parseInt(config.iconSize) });
    }
    return h(CheckIcon, { size: parseInt(config.iconSize) });
  };

  // 构建内容
  const content = [];
  
  if (label && labelPosition === 'before') {
    content.push(h('span', { key: 'label-before', style: labelStyle }, label));
  }
  
  content.push(
    h('span', { key: 'box', style: getBoxStyle() }, renderIcon())
  );
  
  if (label && labelPosition === 'after') {
    content.push(h('span', { key: 'label-after', style: labelStyle }, label));
  }

  return h(
    'label',
    {
      style: mergeStyles(containerStyle, style),
      onClick: handleClick,
      onMouseEnter: () => setHovered(true),
      onMouseLeave: () => { setHovered(false); setPressed(false); },
      onMouseDown: () => setPressed(true),
      onMouseUp: () => setPressed(false),
      ...rest,
    },
    ...content
  );
}

export default Checkbox;
