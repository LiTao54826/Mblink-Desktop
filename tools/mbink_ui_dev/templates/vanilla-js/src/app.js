import { h, render } from 'preact';
import { palette } from './styles/palette.js';

const syncBodyViewport = () => {
  document.documentElement.style.height = '100vh';
  document.body.style.height = '100vh';
  document.body.style.minHeight = '100vh';
  document.body.style.margin = '0';
};

let count = 0;

function stat(title, value) {
  return h(
    'div',
    {
      style: {
        padding: '18px',
        borderRadius: '16px',
        border: `1px solid ${palette.border}`,
        background: palette.card,
        boxShadow: '0 10px 30px rgba(15, 23, 42, 0.06)'
      }
    },
    h('div', { style: { color: palette.muted, fontSize: '13px', marginBottom: '8px' } }, title),
    h('div', { style: { fontSize: '28px', fontWeight: '800', color: palette.text } }, value)
  );
}

function App() {
  const root = h(
    'div',
    { style: { fontFamily: 'Segoe UI, sans-serif', minHeight: '100vh', boxSizing: 'border-box', padding: '28px', background: palette.page, color: palette.text } },
    h(
      'div',
      { style: { maxWidth: '960px', margin: '0 auto', display: 'grid', gap: '18px' } },
      h(
        'section',
        { style: { padding: '24px', borderRadius: '20px', border: `1px solid ${palette.border}`, background: '#ffffff' } },
        h('div', { style: { display: 'inline-flex', padding: '6px 10px', borderRadius: '999px', background: '#dbeafe', color: '#1d4ed8', fontSize: '12px', fontWeight: '700' } }, 'Preact without JSX Template'),
        h('h1', { style: { fontSize: '34px', margin: '14px 0 10px', fontWeight: '800' } }, 'MBink 工程化基础模板'),
        h('p', { style: { color: palette.muted, lineHeight: 1.7, margin: 0 } }, '不依赖 JSX，适合最小成本快速起步。'),
        h(
          'button',
          {
            style: { marginTop: '18px', padding: '12px 16px', borderRadius: '12px', border: 'none', background: palette.primary, color: palette.primaryText, cursor: 'pointer' },
            onClick: () => {
              count += 1;
              render(App(), document.body);
            }
          },
          `点击计数：${count}`
        )
      ),
      h('div', { style: { display: 'grid', gridTemplateColumns: 'repeat(3, minmax(0, 1fr))', gap: '16px' } }, [
        stat('渲染方式', '直接加载'),
        stat('入口文件', 'src/app.js'),
        stat('适用场景', '纯 JS / 原型工具')
      ])
    )
  );

  return root;
}

syncBodyViewport();
render(App(), document.body);
