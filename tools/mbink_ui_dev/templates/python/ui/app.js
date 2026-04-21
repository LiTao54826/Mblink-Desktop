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
    title: 'Python 宿主工程模板',
    subtitle: 'UI 位于 ui/，宿主代码位于 src/，便于继续接入 Python bindings。',
    count,
    onIncrement: () => {
      count += 1;
      render(App(), document.body);
    },
    items: [
      { title: '入口', value: 'ui/app.js', detail: 'mbink-ui-dev 直接打开并渲染当前 UI 入口。' },
      { title: '宿主', value: 'src/main.py', detail: '用于承载 Python 侧桥接逻辑。' },
      { title: '结构', value: 'src + ui', detail: '前后端职责分离，更接近真实工程布局。' }
    ]
  });
}

syncBodyViewport();
render(App(), document.body);
