/**
 * MBink Fluent Design - Link 组件
 * 
 * @description 基于 Microsoft Fluent UI 设计规范的链接组件
 * 
 * Props:
 * - appearance: 'default' | 'subtle'
 * - inline: boolean
 * - disabled: boolean
 * - href: string
 * - onClick: (e) => void
 */

import { h } from 'preact';
import { useState } from 'preact/hooks';
import { colors, fontFamily, fontSize, transition } from './theme.js';
import { mergeStyles } from './utils.js';

export function Link(props) {
  const {
    appearance = 'default',
    inline = false,
    disabled = false,
    href,
    onClick,
    children,
    style,
    ...rest
  } = props;

  const [hovered, setHovered] = useState(false);
  const [pressed, setPressed] = useState(false);

  // 颜色配置
  const colorConfig = {
    default: {
      base: colors.colorBrandForeground1,
      hover: colors.colorBrandForeground2,
      pressed: colors.colorCompoundBrandForeground1Pressed,
      disabled: colors.colorNeutralForegroundDisabled,
    },
    subtle: {
      base: colors.colorNeutralForeground2,
      hover: colors.colorNeutralForeground1,
      pressed: colors.colorNeutralForeground1,
      disabled: colors.colorNeutralForegroundDisabled,
    },
  };

  const config = colorConfig[appearance] || colorConfig.default;

  // 获取当前颜色
  const getColor = () => {
    if (disabled) return config.disabled;
    if (pressed) return config.pressed;
    if (hovered) return config.hover;
    return config.base;
  };

  const linkStyle = {
    fontFamily: fontFamily.base,
    fontSize: inline ? 'inherit' : fontSize.base300,
    lineHeight: inline ? 'inherit' : '1.4',
    color: getColor(),
    textDecoration: hovered && !disabled ? 'underline' : 'none',
    cursor: disabled ? 'not-allowed' : 'pointer',
    transition: `color ${transition.fast}`,
    background: 'none',
    border: 'none',
    padding: 0,
    margin: 0,
    display: inline ? 'inline' : 'inline-block',
  };

  const handleClick = (e) => {
    if (disabled) {
      e.preventDefault();
      return;
    }
    onClick?.(e);
  };

  const Tag = href ? 'a' : 'button';

  return h(Tag, {
    href: disabled ? undefined : href,
    type: Tag === 'button' ? 'button' : undefined,
    'aria-disabled': disabled,
    style: mergeStyles(linkStyle, style),
    onClick: handleClick,
    onMouseEnter: () => setHovered(true),
    onMouseLeave: () => { setHovered(false); setPressed(false); },
    onMouseDown: () => setPressed(true),
    onMouseUp: () => setPressed(false),
    ...rest,
  }, children);
}

export default Link;
