import { h, render } from 'preact';
import { useMemo, useState } from 'preact/hooks';
import { FeatureGrid } from './components/FeatureGrid.jsx';
import { theme } from './styles/theme.js';

const syncBodyViewport = () => {
  document.documentElement.style.height = '100vh';
  document.body.style.height = '100vh';
  document.body.style.minHeight = '100vh';
  document.body.style.margin = '0';
};

function App() {
  const [count, setCount] = useState(0);
  const features = useMemo(
    () => [
      { title: '工程化结构', description: '按 src/components 与 src/styles 分层，便于继续扩展。' },
      { title: '开箱可预览', description: 'mbink-ui-dev open 会先构建 JSX，再直接载入 .dist/App.js。' },
      { title: '渲染状态清晰', description: '按钮点击会立刻修改页面数值，方便确认事件链路正常。' },
      { title: '适合继续改造', description: '保留轻量依赖，只依赖官方 preact / preact/hooks。' }
    ],
    []
  );

  return (
    <div style={theme.page}>
      <div style={theme.shell}>
        <section style={theme.hero}>
          <div style={theme.chip}>MBink UI Dev</div>
          <h1 style={{ fontSize: 34, margin: '14px 0 10px', fontWeight: 800 }}>Preact JSX 工程模板</h1>
          <p style={{ margin: 0, color: '#cbd5e1', lineHeight: 1.7 }}>
            这是给 mbink-ui-dev 的 JSX 模板，适合继续拆分组件、增加状态管理与宿主桥接。
          </p>
          <button
            onClick={() => setCount((value) => value + 1)}
            style={{ marginTop: 18, padding: '12px 16px', borderRadius: 12, border: '1px solid #60a5fa', background: '#1d4ed8', color: '#eff6ff', cursor: 'pointer' }}
          >
            渲染计数：{count}
          </button>
        </section>
        <FeatureGrid items={features} />
      </div>
    </div>
  );
}

syncBodyViewport();
render(<App />, document.body);
