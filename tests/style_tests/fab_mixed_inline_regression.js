import { h, render } from 'preact';

function FabMixedInlineRegression() {
  return h('div', {
    style: 'padding: 24px; background: #f4f4f4; font-family: Arial, sans-serif;'
  },
    h('h2', { style: 'margin: 0 0 16px 0; color: #333;' }, 'FAB mixed inline regression'),

    h('div', {
      style: 'margin-bottom: 24px; padding: 24px; background: #fff; border-radius: 10px;'
    },
      h('h3', { style: 'margin: 0 0 16px 0; color: #666;' }, 'Case A: span 默认 inline'),
      h('div', { style: 'display: flex; gap: 40px; align-items: center; flex-wrap: wrap;' },
        h('div', { style: 'text-align: center;' },
          h('div', {
            style: 'width: 40px; height: 40px; background: #4caf50; color: white; border-radius: 50%; display: flex; align-items: center; justify-content: center; font-size: 20px; box-shadow: 0 2px 8px rgba(0,0,0,0.3); margin-bottom: 10px;'
          }, '+'),
          h('span', { style: 'font-size: 12px; color: #666;' }, '40x40')
        ),
        h('div', { style: 'text-align: center;' },
          h('div', {
            style: 'width: 56px; height: 56px; background: #2196f3; color: white; border-radius: 50%; display: flex; align-items: center; justify-content: center; font-size: 24px; box-shadow: 0 4px 12px rgba(0,0,0,0.3); margin-bottom: 10px;'
          }, '+'),
          h('span', { style: 'font-size: 12px; color: #666;' }, '56x56')
        ),
        h('div', { style: 'text-align: center;' },
          h('div', {
            style: 'width: 72px; height: 72px; background: #ff5722; color: white; border-radius: 50%; display: flex; align-items: center; justify-content: center; font-size: 32px; box-shadow: 0 6px 16px rgba(0,0,0,0.3); margin-bottom: 10px;'
          }, '+'),
          h('span', { style: 'font-size: 12px; color: #666;' }, '72x72')
        )
      )
    ),

    h('div', {
      style: 'padding: 24px; background: #fff; border-radius: 10px;'
    },
      h('h3', { style: 'margin: 0 0 16px 0; color: #666;' }, 'Case B: span 改为 block（对照）'),
      h('div', { style: 'display: flex; gap: 40px; align-items: center; flex-wrap: wrap;' },
        h('div', { style: 'text-align: center;' },
          h('div', {
            style: 'width: 56px; height: 56px; background: #9c27b0; color: white; border-radius: 50%; display: flex; align-items: center; justify-content: center; font-size: 24px; box-shadow: 0 4px 12px rgba(0,0,0,0.3); margin-bottom: 10px;'
          }, 'A'),
          h('span', { style: 'font-size: 12px; color: #666; display: block;' }, 'block label')
        )
      )
    )
  );
}

const root = document.getElementById('root') || document.body;
render(h(FabMixedInlineRegression), root);
console.log('[fab_mixed_inline_regression] rendered');

