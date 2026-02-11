import { h, render } from 'preact';

function App() {
  return h('div', { style: 'padding: 40px; background:#f5f5f5; min-height: 100vh;' },
    h('h3', { style: 'margin:0 0 16px 0; color:#333;' }, 'FAB mixed inline regression'),

    h('div', { style: 'display:flex; gap:40px; align-items:center; flex-wrap:wrap; background:#fff; padding:40px; border-radius:12px;' },
      h('div', { id: 'item-40', style: 'text-align:center; width:72px;' },
        h('div', {
          id: 'fab-40',
          style: 'width:40px;height:40px;background:#ff5722;color:#fff;border-radius:50%;display:flex;align-items:center;justify-content:center;font-size:20px;margin:0 auto 8px auto;'
        }, '+'),
        h('span', { id: 'label-40', style: 'font-size:12px;color:#666;' }, '40x40')
      ),

      h('div', { id: 'item-56', style: 'text-align:center; width:72px;' },
        h('div', {
          id: 'fab-56',
          style: 'width:56px;height:56px;background:#2196f3;color:#fff;border-radius:50%;display:flex;align-items:center;justify-content:center;font-size:26px;margin:0 auto 8px auto;'
        }, '✉'),
        h('span', { id: 'label-56', style: 'font-size:12px;color:#666;' }, '56x56')
      ),

      h('div', { id: 'item-72', style: 'text-align:center; width:72px;' },
        h('div', {
          id: 'fab-72',
          style: 'width:72px;height:72px;background:#4caf50;color:#fff;border-radius:50%;display:flex;align-items:center;justify-content:center;font-size:32px;margin:0 auto 8px auto;'
        }, '✓'),
        h('span', { id: 'label-72', style: 'font-size:12px;color:#666;' }, '72x72')
      )
    ),

    h('p', { style: 'margin-top:16px;color:#777;font-size:12px;' },
      'Check: label should be centered under each circle and DevTools picker should hit label text accurately.'
    )
  );
}

render(h(App), document.body);

