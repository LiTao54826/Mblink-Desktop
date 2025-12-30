/**
 * MBink Fluent Design 工具函数
 */

/**
 * 合并样式对象
 */
export function mergeStyles(...styles) {
  return styles.reduce((acc, style) => {
    if (style && typeof style === 'object') {
      return { ...acc, ...style };
    }
    return acc;
  }, {});
}

/**
 * 创建类名字符串
 */
export function classNames(...classes) {
  return classes.filter(Boolean).join(' ');
}

/**
 * 生成唯一 ID
 */
let idCounter = 0;
export function useId(prefix = 'fluent') {
  return `${prefix}-${++idCounter}`;
}

/**
 * 防抖函数
 */
export function debounce(fn, delay) {
  let timer = null;
  return function (...args) {
    if (timer) clearTimeout(timer);
    timer = setTimeout(() => fn.apply(this, args), delay);
  };
}

/**
 * 节流函数
 */
export function throttle(fn, limit) {
  let inThrottle = false;
  return function (...args) {
    if (!inThrottle) {
      fn.apply(this, args);
      inThrottle = true;
      setTimeout(() => (inThrottle = false), limit);
    }
  };
}

/**
 * 检查是否为空值
 */
export function isNullish(value) {
  return value === null || value === undefined;
}

/**
 * 获取嵌套对象属性
 */
export function get(obj, path, defaultValue) {
  const keys = path.split('.');
  let result = obj;
  for (const key of keys) {
    if (result === null || result === undefined) {
      return defaultValue;
    }
    result = result[key];
  }
  return result === undefined ? defaultValue : result;
}
