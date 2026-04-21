import { h } from 'preact';

export function FeatureGrid({ items }) {
  return h(
    'div',
    { style: { display: 'grid', gridTemplateColumns: 'repeat(2, minmax(0, 1fr))', gap: '16px' } },
    items.map((item) =>
      h(
        'div',
        {
          key: item.title,
          style: {
            padding: '18px',
            borderRadius: '16px',
            border: '1px solid #334155',
            background: '#0f172a'
          }
        },
        h('div', { style: { fontSize: '18px', fontWeight: '700', marginBottom: '8px' } }, item.title),
        h('div', { style: { color: '#94a3b8', lineHeight: 1.6 } }, item.description)
      )
    )
  );
}
