/**
 * MBink Fluent Design - Switch 组件
 * 
 * @description 基于 Microsoft Fluent UI 设计规范的开关组件
 * 
 * Props:
 * - checked: boolean
 * - defaultChecked: boolean
 * - disabled: boolean
 * - label: string | VNode
 * - labelPosition: 'before' | 'after' | 'above'
 * - onChange: (e, data) => void
 */

import { h } from 'preact';
import { useState } from 'preact/hooks';
import { 
  colors, borderRadius, transition, fontFamily, fontSize, spacing 
} from './theme.js';
import { mergeStyles } from './utils.js';

// 尺寸配置
const TRACK_WIDTH = 40;
const TRACK_HEIGHT = 20;
const THUMB_SIZE = 14;
const THUMB_MARGIN = 3;

export function Switch(props) {
  const {
    checked,
    defaultChecked = false,
    disabled = false,
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

  // 容器样式
  const containerStyle = {
    display: 'inline-flex',
    alignItems: labelPosition === 'above' ? 'flex-start' : 'center',
    flexDirection: labelPosition === 'above' ? 'column' : 'row',
    gap: spacing.s,
    cursor: disabled ? 'not-allowed' : 'pointer',
    userSelect: 'none',
    opacity: disabled ? 0.5 : 1,
  };

  // 轨道样式
  const getTrackStyle = () => {
    const base = {
      position: 'relative',
      width: `${TRACK_WIDTH}px`,
      height: `${TRACK_HEIGHT}px`,
      borderRadius: borderRadius.circular,
      border: `1px solid ${colors.colorNeutralStrokeAccessible}`,
      backgroundColor: colors.colorNeutralBackground1,
      transition: `all ${transition.fast}`,
      flexShrink: 0,
      boxSizing: 'border-box',
    };

    if (disabled) {
      return {
        ...base,
        borderColor: colors.colorNeutralStrokeDisabled,
        backgroundColor: isChecked 
          ? colors.colorNeutralBackgroundDisabled 
          : colors.colorNeutralBackground1,
      };
    }

    if (isChecked) {
      return {
        ...base,
        backgroundColor: pressed 
          ? colors.colorCompoundBrandBackgroundPressed
          : hovered 
            ? colors.colorCompoundBrandBackgroundHover
            : colors.colorCompoundBrandBackground,
        borderColor: 'transparent',
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

  // 滑块样式
  const getThumbStyle = () => {
    const translateX = isChecked 
      ? TRACK_WIDTH - THUMB_SIZE - THUMB_MARGIN * 2 - 2 
      : 0;

    return {
      position: 'absolute',
      top: `${THUMB_MARGIN}px`,
      left: `${THUMB_MARGIN}px`,
      width: `${THUMB_SIZE}px`,
      height: `${THUMB_SIZE}px`,
      borderRadius: borderRadius.circular,
      backgroundColor: isChecked 
        ? colors.colorNeutralForegroundOnBrand 
        : colors.colorNeutralForeground3,
      transition: `transform ${transition.fast}, background-color ${transition.fast}`,
      transform: `translateX(${translateX}px)`,
      boxShadow: '0 1px 2px rgba(0,0,0,0.14)',
    };
  };

  // 标签样式
  const labelStyle = {
    fontSize: fontSize.base300,
    fontFamily: fontFamily.base,
    color: disabled ? colors.colorNeutralForegroundDisabled : colors.colorNeutralForeground1,
    lineHeight: '1.4',
  };

  const handleClick = (e) => {
    if (disabled) return;
    
    const newValue = !isChecked;
    if (!isControlled) {
      setInternalChecked(newValue);
    }
    onChange?.(e, { checked: newValue });
  };

  // 构建内容
  const content = [];
  
  if (label && (labelPosition === 'before' || labelPosition === 'above')) {
    content.push(h('span', { key: 'label-before', style: labelStyle }, label));
  }
  
  content.push(
    h('span', { key: 'track', style: getTrackStyle() },
      h('span', { style: getThumbStyle() })
    )
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
      role: 'switch',
      'aria-checked': isChecked,
      ...rest,
    },
    ...content
  );
}

export default Switch;
