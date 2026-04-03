export const C = {
  bg: '#f3f4f6',
  panel: '#ffffff',
  panelAlt: '#f9fafb',
  border: 'rgba(0, 0, 0, 0.06)',
  text: '#111827',
  muted: '#6b7280',
  primary: '#6366f1',
  success: '#10b981',
  warning: '#f59e0b',
  danger: '#ef4444',
  shadow: '0 1px 2px rgba(0,0,0,0.02), 0 4px 12px rgba(0,0,0,0.03), 0 12px 24px -4px rgba(0,0,0,0.03)',
  logBg: '#09090b',
  logBorder: '#27272a'
};

export function statusTone(status) {
  var colors = {
    '空闲': C.muted,
    '监听中': C.primary,
    '执行中': C.success,
    '资源告警': C.warning,
    '异常': C.danger
  };
  return colors[status] || C.primary;
}

