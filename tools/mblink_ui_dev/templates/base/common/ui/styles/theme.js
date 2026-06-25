export const palette = {
  bg: 'linear-gradient(135deg, #020617 0%, #111827 45%, #172554 100%)',
  text: '#e5e7eb',
  muted: '#94a3b8',
  accent: '#38bdf8',
  panel: 'rgba(15, 23, 42, 0.82)',
  border: 'rgba(148, 163, 184, 0.24)'
};

export const cardStyle = {
  padding: 20,
  borderRadius: 18,
  background: palette.panel,
  border: `1px solid ${palette.border}`,
  boxShadow: '0 24px 80px rgba(2, 6, 23, 0.28)'
};

export const buttonStyle = {
  border: 0,
  borderRadius: 999,
  padding: '10px 16px',
  color: '#020617',
  background: '#67e8f9',
  fontWeight: 700,
  cursor: 'pointer'
};
