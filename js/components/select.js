/**
 * MBink Select 组件
 *
 * Props:
 * - value: any
 * - options: Array<{ value, label, disabled? }>
 * - placeholder: string
 * - size: 'sm' | 'md' | 'lg'
 * - disabled: boolean
 * - clearable: boolean
 * - onChange: (value) => void
 */

import { h } from 'preact';
import { useState, useRef, useEffect, useLayoutEffect } from 'preact/hooks';
import { colors, radius, sizes, shadow, transition } from './theme.js';
import { mergeStyles } from './utils.js';

// 全局状态：当前打开的 Select
let activeSelectId = null;
let activeSelectCallback = null;

export function Select(props) {
  const {
    value,
    defaultValue,
    options = [],
    placeholder = 'Select...',
    size = 'md',
    disabled = false,
    clearable = false,
    onChange,
    style,
    ...rest
  } = props;

  const [internalValue, setInternalValue] = useState(defaultValue);
  const [open, setOpen] = useState(false);
  const [hoveredIndex, setHoveredIndex] = useState(-1);
  const [selectId] = useState(() => Math.random().toString(36).substr(2, 9));
  const triggerRef = useRef(null);

  const isControlled = value !== undefined;
  const currentValue = isControlled ? value : internalValue;

  const selectedOption = options.find((opt) => opt.value === currentValue);
  const sizeConfig = sizes[size] || sizes.md;

  // 当其他 Select 打开时，关闭当前的
  useEffect(() => {
    // 只有当 activeSelectId 存在且不是当前 Select 时才关闭
    if (open && activeSelectId && activeSelectId !== selectId) {
      setOpen(false);
    }
  }, [open, selectId]);

  // 打开时注册为活动 Select
  useEffect(() => {
    if (open) {
      activeSelectId = selectId;
      activeSelectCallback = () => setOpen(false);
    } else if (activeSelectId === selectId) {
      activeSelectId = null;
      activeSelectCallback = null;
    }
  }, [open, selectId]);

  const wrapperStyle = {
    position: 'relative',
    width: '100%',
  };

  const triggerStyle = {
    display: 'flex',
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'space-between',
    width: '100%',
    height: sizeConfig.height,
    padding: '0 12px',
    fontSize: sizeConfig.fontSize,
    color: selectedOption ? colors.text : colors.textTertiary,
    backgroundColor: disabled ? colors.bgSecondary : colors.bg,
    border: `1px solid ${open ? colors.borderFocus : colors.border}`,
    borderRadius: radius.md,
    cursor: disabled ? 'not-allowed' : 'pointer',
    boxSizing: 'border-box',
    opacity: disabled ? 0.5 : 1,
  };

  const dropdownStyle = {
    position: 'absolute',
    top: sizeConfig.height,
    left: '0',
    width: '100%',
    marginTop: '4px',
    padding: '4px 0',
    backgroundColor: colors.bg,
    border: `1px solid ${colors.border}`,
    borderRadius: radius.md,
    boxShadow: shadow.md,
    zIndex: 1000,
    maxHeight: '200px',
    overflowY: 'auto',
    boxSizing: 'border-box',
  };

  const getOptionStyle = (opt, index) => ({
    padding: '8px 12px',
    fontSize: sizeConfig.fontSize,
    color: opt.value === currentValue 
      ? colors.textInverse 
      : opt.disabled 
        ? colors.textDisabled 
        : colors.text,
    backgroundColor: opt.value === currentValue 
      ? colors.primary
      : hoveredIndex === index 
        ? colors.bgHover 
        : 'transparent',
    cursor: opt.disabled ? 'not-allowed' : 'pointer',
  });

  const handleTriggerClick = (e) => {
    if (disabled) return;
    
    // 阻止事件冒泡，避免触发其他处理器
    e.stopPropagation();
    
    // 如果有其他 Select 打开，先关闭它
    if (activeSelectCallback && activeSelectId !== selectId) {
      activeSelectCallback();
    }
    
    setOpen(!open);
    setHoveredIndex(-1);
  };

  const handleSelect = (opt, e) => {
    if (e) e.stopPropagation();
    if (opt.disabled) return;
    if (!isControlled) {
      setInternalValue(opt.value);
    }
    onChange?.(opt.value);
    setOpen(false);
  };

  const handleClear = (e) => {
    e.stopPropagation();
    if (!isControlled) {
      setInternalValue(undefined);
    }
    onChange?.(undefined);
  };

  // 构建子元素
  const children = [
    // 触发器
    h('div', { 
      key: 'trigger',
      ref: triggerRef,
      style: triggerStyle, 
      onClick: handleTriggerClick 
    }, [
      h('span', { key: 'text' }, selectedOption ? selectedOption.label : placeholder),
      h('span', { key: 'icons', style: { display: 'flex', alignItems: 'center' } }, [
        clearable && currentValue !== undefined && h('span', { 
          key: 'clear',
          style: { marginRight: '4px', color: colors.textTertiary, cursor: 'pointer' }, 
          onClick: handleClear 
        }, '×'),
        h('span', { 
          key: 'arrow',
          style: { 
            fontSize: '12px', 
            color: colors.textTertiary,
            transform: open ? 'rotate(180deg)' : 'rotate(0)',
          } 
        }, '▼'),
      ]),
    ]),
  ];

  // 下拉框 - 直接作为子元素，依赖高 z-index
  if (open) {
    children.push(
      h('div', { 
        key: 'dropdown', 
        style: dropdownStyle,
        onWheel: (e) => {
          // 阻止滚动事件穿透到父级
          e.stopPropagation();
          e.preventDefault();
        }
      },
        options.map((opt, index) =>
          h('div', {
            key: opt.value,
            style: getOptionStyle(opt, index),
            onClick: (e) => handleSelect(opt, e),
            onMouseEnter: () => !opt.disabled && setHoveredIndex(index),
            onMouseLeave: () => setHoveredIndex(-1),
          }, opt.label)
        )
      )
    );
  }

  return h('div', { style: mergeStyles(wrapperStyle, style), ...rest }, children);
}

export default Select;
