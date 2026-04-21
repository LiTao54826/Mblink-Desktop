import { h, render } from 'preact';
import { AppShell } from './components/AppShell.js';

const syncBodyViewport = () => {
  document.documentElement.style.height = '100vh';
  document.body.style.height = '100vh';
  document.body.style.minHeight = '100vh';
  document.body.style.margin = '0';
};

let count = 0;

function App() {
  return h(AppShell, {
    count,
    onIncrement: () => {
      count += 1;
      render(App(), document.body);
    },
    items: [
      { title: '入口', value: 'ui/app.js', detail: '直接由 mbink-ui-dev 加载与渲染。' },
      { title: 'CLI', value: 'cmd/mbink/main.go', detail: '可继续扩展 Go 侧启动与桥接逻辑。' },
      { title: '服务层', value: 'internal/service', detail: '预留业务状态与 API 封装。' }
    ]
  });
}

syncBodyViewport();
render(App(), document.body);
