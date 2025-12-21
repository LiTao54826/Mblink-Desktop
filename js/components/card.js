/**
 * LightUI Card 组件
 *
 * Props:
 * - title: string | VNode
 * - extra: VNode (右上角额外内容)
 * - padding: number | string
 * - shadow: 'none' | 'sm' | 'md' | 'lg'
 * - bordered: boolean
 * - hoverable: boolean
 * - onClick: () => void
 */

import { h } from 'preact';
import { colors, radius, shadow as shadowMap, transition } from './theme.js';
import { mergeStyles } from './utils.js';

export function Card(props) {
  const {
    title,
    extra,
    padding = 16,
    shadow = 'sm',
    bordered = true,
    hoverable = false,
    onClick,
    children,
    style,
    ...rest
  } = props;

  const cardStyle = {
    backgroundColor: colors.bg,
    borderRadius: radius.lg,
    border: bordered ? `1px solid ${colors.border}` : 'none',
    boxShadow: shadowMap[shadow] || shadow,
    transition: hoverable ? `box-shadow ${transition.normal}` : undefined,
    cursor: onClick || hoverable ? 'pointer' : 'default',
    overflow: 'hidden',
  };

  const headerStyle = {
    display: 'flex',
    alignItems: 'center',
    justifyContent: 'space-between',
    padding: typeof padding === 'number' ? `${padding}px` : padding,
    borderBottom: `1px solid ${colors.border}`,
  };

  const titleStyle = {
    fontSize: '16px',
    fontWeight: '600',
    color: colors.text,
    margin: 0,
  };

  const bodyStyle = {
    padding: typeof padding === 'number' ? `${padding}px` : padding,
  };

  const handleMouseEnter = (e) => {
    if (hoverable) {
      e.currentTarget.style.boxShadow = shadowMap.md;
    }
  };

  const handleMouseLeave = (e) => {
    if (hoverable) {
      e.currentTarget.style.boxShadow = shadowMap[shadow] || shadow;
    }
  };

  const hasHeader = title || extra;

  return h(
    'div',
    {
      style: mergeStyles(cardStyle, style),
      onClick,
      onMouseEnter: handleMouseEnter,
      onMouseLeave: handleMouseLeave,
      ...rest,
    },
    hasHeader &&
      h(
        'div',
        { style: headerStyle },
        title && (typeof title === 'string' ? h('div', { style: titleStyle }, title) : title),
        extra
      ),
    h('div', { style: bodyStyle }, children)
  );
}

export default Card;
