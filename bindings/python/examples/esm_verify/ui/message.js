export const message = 'preact / preact-hooks / 相对模块导入工作正常';

export const checks = [
  typeof document !== 'undefined',
  typeof window !== 'undefined',
  typeof message === 'string',
  Array.isArray([1, 2, 3]),
];

