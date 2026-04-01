import { h, render } from 'preact';
import { useMemo } from 'preact/hooks';
import { message, checks } from './message.js';

function App() {
  const passed = useMemo(() => checks.filter(Boolean).length, []);

  return h('div', null,
    h('div', { class: 'card' },
      h('div', { class: 'ok' }, 'ESM / HTML 加载验证通过'),
      h('p', null, message),
      h('p', null, '已验证：外链 CSS、HTML base path、script type="module"、相对 import、preact 模块导入')
    ),
    h('div', { class: 'card' },
      h('div', null, `检查项：${passed}/${checks.length}`),
      h('ul', null,
        checks.map((ok, i) => h('li', { key: i }, `check_${i + 1}: ${ok ? 'ok' : 'fail'}`))
      )
    ),
    h('div', { class: 'card' },
      h('div', null, '如果这个页面正常渲染，说明 Python 绑定已能走接近 esm_loader 的 HTML + ESM 链路。'),
      h('div', null, h('code', null, './app.js -> ./message.js'))
    )
  );
}

render(h(App), document.getElementById('app'));

