/**
 * MBink Fluent Design - Select/Dropdown 组件
 * 
 * @description 基于 Microsoft Fluent UI 设计规范的下拉选择组件
 * 
 * Props:
 * - size: 'small' | 'medium' | 'large'
 * - appearance: 'outline' | 'underline' | 'filledDarker' | 'filledLighter'
 * - value: string
 * - defaultValue: string
 * - placeholder: string
 * - disabled: boolean
 * - options: Array<{ value: string, label: string, disabled?: boolean }>
 * - onChange: (e, data) => void
 */

import { h } from 'preact';
import { useState, useRef, useEffect } from 'preact/hooks';
import { 
  colors, borderRadius, shadow, transition, fontFamily, fontSize, spacing 
} from './theme.js';
import { mergeStyles } from './utils.js';

// 全局状态：当前打开的 Select
let activeSelectId = null;
let activeSelectCallback = null;

// 尺寸配置
const sizeStyles = {
  small: {
    height: '24px',
    fontSize: fontSize.base200,
    padding: `0 ${spacing.s}`,
    iconSize: '12px',
  },
  medium: {
    height: '32px',
    fontSize: fontSize.base300,
    padding: `0 ${spacing.mNudge}`,
    iconSize: '16px',
  },
  large: {
    height: '40px',
    fontSize: fontSize.base400,
    padding: `0 ${spacing.m}`,
    iconSize: '20px',
  },
};

// 下拉箭头图标
const ChevronIcon = ({ size = 16, expanded = false }) => h('svg', {
  width: size,
  height: size,
  viewBox: '0 0 20 20',
  fill: 'currentColor',
  style: {
    transition: `transform ${transition.fast}`,
    transform: expanded ? 'rotate(180deg)' : 'rotate(0deg)',
  },
}, h('path', {
  d: 'M15.85 7.65c.2.2.2.5 0 .7l-5.46 5.49a.55.55 0 01-.78 0L4.15 8.35a.5.5 0 11.7-.7L10 12.8l5.15-5.16c.2-.2.5-.2.7 0z',
}));

export function Select(props) {
  const {
    size = 'medium',
    appearance = 'outline',
    value,
    defaultValue = '',
    placeholder = 'Select an option',
    disabled = false,
    options = [],
    onChange,
    style,
    ...rest
  } = props;

  const [isOpen, setIsOpen] = useState(false);
  const [focused, setFocused] = useState(false);
  const [hovered, setHovered] = useState(false);
  const [internalValue, setInternalValue] = useState(defaultValue);
  const [highlightedIndex, setHighlightedIndex] = useState(-1);
  const [selectId] = useState(() => Math.random().toString(36).substr(2, 9));

  const isControlled = value !== undefined;
  const currentValue = isControlled ? value : internalValue;
  const selectedOption = options.find(opt => opt.value === currentValue);

  const sizeConfig = sizeStyles[size] || sizeStyles.medium;

  // 触发器样式
  const getTriggerStyle = () => {
    const base = {
      display: 'flex',
      alignItems: 'center',
      justifyContent: 'space-between',
      width: '100%',
      height: sizeConfig.height,
      padding: sizeConfig.padding,
      fontSize: sizeConfig.fontSize,
      fontFamily: fontFamily.base,
      backgroundColor: colors.colorNeutralBackground1,
      border: `1px solid ${colors.colorNeutralStroke1}`,
      borderRadius: borderRadius.medium,
      cursor: disabled ? 'not-allowed' : 'pointer',
      transition: `all ${transition.fast}`,
      boxSizing: 'border-box',
      outline: 'none',
      color: selectedOption ? colors.colorNeutralForeground1 : colors.colorNeutralForeground4,
    };

    if (disabled) {
      return {
        ...base,
        backgroundColor: colors.colorNeutralBackgroundDisabled,
        borderColor: colors.colorNeutralStrokeDisabled,
        color: colors.colorNeutralForegroundDisabled,
      };
    }

    if (isOpen || focused) {
      return {
        ...base,
        borderColor: colors.colorCompoundBrandStroke,
      };
    }

    if (hovered) {
      return {
        ...base,
        borderColor: colors.colorNeutralStroke1Hover,
      };
    }

    return base;
  };

  // 下拉列表样式
  const listStyle = {
    position: 'absolute',
    top: '100%',
    left: 0,
    right: 0,
    marginTop: '4px',
    backgroundColor: colors.colorNeutralBackground1,
    border: `1px solid ${colors.colorNeutralStroke1}`,
    borderRadius: borderRadius.medium,
    boxShadow: shadow.shadow16,
    maxHeight: '300px',
    overflowY: 'auto',
    zIndex: 1000,
    padding: `${spacing.xs} 0`,
  };

  // 选项样式
  const getOptionStyle = (option, index) => {
    const isSelected = option.value === currentValue;
    const isHighlighted = index === highlightedIndex;
    const isDisabled = option.disabled;

    return {
      display: 'flex',
      alignItems: 'center',
      padding: `${spacing.sNudge} ${spacing.s}`,
      fontSize: sizeConfig.fontSize,
      fontFamily: fontFamily.base,
      cursor: isDisabled ? 'not-allowed' : 'pointer',
      backgroundColor: isHighlighted 
        ? colors.colorNeutralBackground1Hover 
        : isSelected 
          ? colors.colorNeutralBackground1Selected 
          : 'transparent',
      color: isDisabled 
        ? colors.colorNeutralForegroundDisabled 
        : colors.colorNeutralForeground1,
      transition: `background-color ${transition.fast}`,
    };
  };

  const handleTriggerClick = (e) => {
    if (disabled) return;
    e.stopPropagation();
    setIsOpen(!isOpen);
    setHighlightedIndex(-1);
  };

  const handleOptionClick = (option, e) => {
    if (e) e.stopPropagation();
    if (option.disabled) return;
    
    if (!isControlled) {
      setInternalValue(option.value);
    }
    onChange?.({ target: { value: option.value } }, { value: option.value, option });
    setIsOpen(false);
  };

  const handleKeyDown = (e) => {
    if (disabled) return;

    switch (e.key) {
      case 'Enter':
      case ' ':
        e.preventDefault();
        if (isOpen && highlightedIndex >= 0) {
          handleOptionClick(options[highlightedIndex], e);
        } else {
          setIsOpen(!isOpen);
        }
        break;
      case 'ArrowDown':
        e.preventDefault();
        if (!isOpen) {
          setIsOpen(true);
        } else {
          setHighlightedIndex(prev => 
            Math.min(prev + 1, options.length - 1)
          );
        }
        break;
      case 'ArrowUp':
        e.preventDefault();
        setHighlightedIndex(prev => Math.max(prev - 1, 0));
        break;
      case 'Escape':
        setIsOpen(false);
        break;
    }
  };

  // 构建子元素
  const children = [
    // 触发器
    h('div', {
      key: 'trigger',
      style: getTriggerStyle(),
      onClick: handleTriggerClick,
      onFocus: () => setFocused(true),
      onBlur: () => setFocused(false),
      onMouseEnter: () => setHovered(true),
      onMouseLeave: () => setHovered(false),
      onKeyDown: handleKeyDown,
      tabIndex: disabled ? -1 : 0,
      role: 'combobox',
      'aria-haspopup': 'listbox',
      'aria-expanded': isOpen,
    }, [
      h('span', { key: 'text', style: { flex: 1, textAlign: 'left' } }, 
        selectedOption?.label || placeholder
      ),
      h('span', { 
        key: 'icon', 
        style: { 
          display: 'flex', 
          alignItems: 'center',
          color: colors.colorNeutralForeground3,
          marginLeft: spacing.s,
        } 
      }, h(ChevronIcon, { expanded: isOpen })),
    ]),
  ];

  // 下拉列表
  if (isOpen) {
    children.push(
      h('div', {
        key: 'list',
        role: 'listbox',
        style: listStyle,
      }, options.map((option, index) => 
        h('div', {
          key: option.value,
          role: 'option',
          'aria-selected': option.value === currentValue,
          'aria-disabled': option.disabled,
          style: getOptionStyle(option, index),
          onClick: (e) => handleOptionClick(option, e),
          onMouseEnter: () => setHighlightedIndex(index),
          onMouseLeave: () => setHighlightedIndex(-1),
        }, option.label)
      ))
    );
  }

  return h('div', {
    style: mergeStyles({ position: 'relative', display: 'inline-block', minWidth: '150px' }, style),
    ...rest,
  }, children);
}

export default Select;
