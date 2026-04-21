import { h, render } from 'preact';
import { useMemo, useState } from 'preact/hooks';

const syncBodyViewport = () => {
  document.documentElement.style.height = '100vh';
  document.body.style.height = '100vh';
  document.body.style.minHeight = '100vh';
  document.body.style.margin = '0';
};

type MetricCardProps = {
  title: string;
  value: string;
  hint: string;
};

function MetricCard(props: MetricCardProps) {
  return (
    <div style={{ padding: 18, borderRadius: 16, border: '1px solid #334155', background: '#0f172a' }}>
      <div style={{ color: '#94a3b8', fontSize: 13, marginBottom: 8 }}>{props.title}</div>
      <div style={{ fontSize: 28, fontWeight: 800, marginBottom: 8 }}>{props.value}</div>
      <div style={{ color: '#cbd5e1', lineHeight: 1.6 }}>{props.hint}</div>
    </div>
  );
}

function App() {
  const [count, setCount] = useState<number>(0);
  const metrics = useMemo(
    () => [
      { title: '语言', value: 'TypeScript', hint: '适合继续加类型、拆模块与接入宿主 bridge。' },
      { title: '构建', value: 'esbuild', hint: 'open 时自动进行 JSX/TS 编译并载入 .dist/App.js。' }
    ],
    []
  );

  return (
    <div style={{ fontFamily: 'Segoe UI, sans-serif', minHeight: '100vh', boxSizing: 'border-box', padding: 28, background: 'linear-gradient(180deg, #020617, #111827)', color: '#e2e8f0' }}>
      <div style={{ maxWidth: 960, margin: '0 auto', display: 'grid', gap: 18 }}>
        <section style={{ padding: 24, borderRadius: 20, border: '1px solid #334155', background: 'linear-gradient(135deg, rgba(14,165,233,0.24), rgba(15,23,42,0.94) 58%)' }}>
          <div style={{ display: 'inline-flex', padding: '6px 10px', borderRadius: 999, background: '#0f766e', color: '#ccfbf1', fontSize: 12, fontWeight: 700 }}>MBink + TS</div>
          <h1 style={{ fontSize: 34, margin: '14px 0 10px', fontWeight: 800 }}>Preact TS 工程模板</h1>
          <p style={{ color: '#cbd5e1', lineHeight: 1.7 }}>当前按钮可用于确认真实渲染与事件更新已跑通。</p>
          <button onClick={() => setCount((value) => value + 1)} style={{ marginTop: 18, padding: '12px 16px', borderRadius: 12, border: '1px solid #2dd4bf', background: '#0f766e', color: '#f0fdfa', cursor: 'pointer' }}>
            点击计数：{count}
          </button>
        </section>
        <div style={{ display: 'grid', gridTemplateColumns: 'repeat(2, minmax(0, 1fr))', gap: 16 }}>
          {metrics.map((item) => (
            <MetricCard key={item.title} title={item.title} value={item.value} hint={item.hint} />
          ))}
        </div>
      </div>
    </div>
  );
}

syncBodyViewport();
render(<App />, document.body);
