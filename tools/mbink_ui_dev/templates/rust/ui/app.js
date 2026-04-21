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
      { title: '入口', value: 'ui/app.js', detail: '运行时直接渲染当前前端入口。' },
      { title: '宿主', value: 'src/main.rs', detail: '预留 Rust 主程序与桥接入口。' },
      { title: '模块', value: 'src/api / service', detail: '适合继续拆分命令处理和状态层。' }
    ]
  });
}

syncBodyViewport();
render(App(), document.body);
