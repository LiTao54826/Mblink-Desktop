/**
 * MBlink 工具函数
 */

/**
 * 合并样式对象
 */
export function mergeStyles(...styles) {
  return Object.assign({}, ...styles.filter(Boolean));
}

/**
 * 合并 class 名
 */
export function classNames(...classes) {
  return classes.filter(Boolean).join(' ');
}

/**
 * 创建内联样式字符串
 */
export function styleToString(style) {
  if (!style || typeof style === 'string') return style || '';
  return Object.entries(style)
    .filter(([, v]) => v != null && v !== '')
    .map(([k, v]) => `${toKebabCase(k)}: ${v}`)
    .join('; ');
}

/**
 * camelCase 转 kebab-case
 */
export function toKebabCase(str) {
  return str.replace(/([A-Z])/g, '-$1').toLowerCase();
}

/**
 * 生成唯一 ID
 */
let idCounter = 0;
export function uniqueId(prefix = 'mblink') {
  return `${prefix}-${++idCounter}`;
}

/**
 * 防抖
 */
export function debounce(fn, delay) {
  let timer = null;
  return function (...args) {
    if (timer) clearTimeout(timer);
    timer = setTimeout(() => fn.apply(this, args), delay);
  };
}

/**
 * 节流
 */
export function throttle(fn, delay) {
  let last = 0;
  return function (...args) {
    const now = Date.now();
    if (now - last >= delay) {
      last = now;
      fn.apply(this, args);
    }
  };
}

/**
 * 判断是否为空值
 */
export function isEmpty(value) {
  if (value == null) return true;
  if (Array.isArray(value)) return value.length === 0;
  if (typeof value === 'object') return Object.keys(value).length === 0;
  if (typeof value === 'string') return value.trim() === '';
  return false;
}

/**
 * 浅比较
 */
export function shallowEqual(a, b) {
  if (a === b) return true;
  if (!a || !b) return false;
  const keysA = Object.keys(a);
  const keysB = Object.keys(b);
  if (keysA.length !== keysB.length) return false;
  for (const key of keysA) {
    if (a[key] !== b[key]) return false;
  }
  return true;
}
