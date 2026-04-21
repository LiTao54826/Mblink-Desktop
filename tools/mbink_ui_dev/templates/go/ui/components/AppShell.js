import { h } from 'preact';
import { theme } from '../styles/theme.js';

export function AppShell({ count, onIncrement, items }) {
  return h('div', { style: { fontFamily: 'Segoe UI, sans-serif', minHeight: '100vh', boxSizing: 'border-box', padding: '28px', background: theme.page, color: theme.text } },
    h('div', { style: { maxWidth: '960px', margin: '0 auto', display: 'grid', gap: '18px' } },
      h('section', { style: { padding: '24px', borderRadius: '20px', border: `1px solid ${theme.border}`, background: `linear-gradient(135deg, ${theme.accent}33, ${theme.panel} 58%)` } },
        h('div', { style: { display: 'inline-flex', padding: '6px 10px', borderRadius: '999px', background: '#14532d', color: '#dcfce7', fontSize: '12px', fontWeight: '700' } }, 'Go Host'),
        h('h1', { style: { fontSize: '34px', margin: '14px 0 10px', fontWeight: '800' } }, 'Go 宿主工程模板'),
        h('p', { style: { margin: 0, color: '#d1fae5', lineHeight: 1.7 } }, 'UI 与宿主代码分离，适合继续接入 Go service / IPC / plugin 逻辑。'),
        h('button', { onClick: onIncrement, style: { marginTop: '18px', padding: '12px 16px', borderRadius: '12px', border: '1px solid #4ade80', background: theme.accent, color: '#f0fdf4', cursor: 'pointer' } }, `调用计数：${count}`)
      ),
      h('div', { style: { display: 'grid', gridTemplateColumns: 'repeat(3, minmax(0, 1fr))', gap: '16px' } },
        items.map((item) => h('div', { key: item.title, style: { padding: '18px', borderRadius: '16px', border: `1px solid ${theme.border}`, background: theme.panelAlt } }, [
          h('div', { style: { color: theme.muted, fontSize: '13px', marginBottom: '8px' } }, item.title),
          h('div', { style: { fontSize: '18px', fontWeight: '700', marginBottom: '8px' } }, item.value),
          h('div', { style: { color: '#d1fae5', lineHeight: 1.6 } }, item.detail)
        ]))
      )
    )
  );
}
