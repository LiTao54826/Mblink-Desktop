import { h, render } from 'preact';

const styles = {
  wrap: { display: 'flex', flexDirection: 'column', gap: '10px' },
  row: { display: 'grid', gridTemplateColumns: '1fr 1fr', gap: '8px' },
  button: { border: 0, borderRadius: '8px', padding: '10px 12px', fontSize: '14px', cursor: 'pointer', color: '#fff', background: '#2563eb' },
  secondary: { background: '#475569' },
  warn: { background: '#d97706' },
  danger: { background: '#dc2626' },
  card: { padding: '10px', borderRadius: '8px', background: '#0b1220', border: '1px solid #223047', fontFamily: 'Consolas, monospace', fontSize: '12px', whiteSpace: 'pre-wrap' },
  label: { color: '#94a3b8', marginBottom: '4px' }
};

function btn(text, onClick, extra) {
  return h('button', { style: Object.assign({}, styles.button, extra || {}), onClick }, text);
}

async function callAndShow(name) {
  const res = await py[name]();
  if (res && typeof res.preview === 'string') {
    const box = document.getElementById('status');
    if (box) box.textContent = res.preview;
  }
}

function App() {
  return h('div', { style: styles.wrap }, [
    h('div', { style: styles.row }, [
      btn('追加 INFO', () => backend.append_info()),
      btn('追加 ERROR', () => backend.append_error(), styles.warn)
    ]),
    h('div', { style: styles.row }, [
      btn('写入终端', () => backend.write_terminal()),
      btn('执行命令', () => backend.run_command(), styles.secondary)
    ]),
    h('div', { style: styles.row }, [
      btn('导出日志', () => callAndShow('export_logs'), styles.secondary),
      btn('终端快照', () => callAndShow('snapshot_terminal'), styles.secondary)
    ]),
    h('div', { style: styles.row }, [
      btn('清空日志', () => backend.clear_logs(), styles.danger),
      btn('清空终端', () => backend.clear_terminal(), styles.danger)
    ]),
    h('div', { style: styles.card }, [
      h('div', { style: styles.label }, 'shared state'),
      'started_at: ' + demo.started_at + '\n' +
      'last_action: ' + demo.last_action + '\n' +
      'command_count: ' + demo.command_count + '\n' +
      'last_log: ' + demo.last_log
    ])
  ]);
}

var root = document.getElementById('root') || document.body;
render(h(App), root);

