import { h, render } from 'preact';

function App() {
  return h('div', null,
    h('div', null, 'Preact img.src 测试'),
    h('img', {
      src: 'app://res.ico',
      alt: 'app-icon',
      style: {
        width: '48px',
        height: '48px',
        display: 'block',
        marginTop: '12px',
        borderRadius: '8px'
      }
    })
  );
}

render(h(App), document.getElementById('app'));
