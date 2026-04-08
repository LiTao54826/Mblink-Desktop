const { h, render } = Preact;

const styles = {
    title: { fontSize: '20px', fontWeight: '700', marginBottom: '8px' },
    sub: { fontSize: '12px', color: '#94a3b8', lineHeight: 1.6, marginBottom: '14px' },
    card: { background: '#0b1220', border: '1px solid #223047', borderRadius: '10px', padding: '12px', marginBottom: '12px' },
    label: { fontSize: '12px', color: '#94a3b8', marginBottom: '6px' },
    value: { fontSize: '14px', whiteSpace: 'pre-wrap', wordBreak: 'break-word' },
    button: { border: 0, borderRadius: '8px', padding: '10px 12px', cursor: 'pointer', background: '#2563eb', color: '#fff', fontWeight: '700', width: '100%' },
};

async function pingRust() {
    const result = await backend.ping({ from: 'preact' });
    alert(JSON.stringify(result, null, 2));
}

function Block(label, value) {
    return h('div', { style: styles.card },
        h('div', { style: styles.label }, label),
        h('div', { style: styles.value }, value || '<empty>')
    );
}

function App() {
    return h('div', null,
        h('div', { style: styles.title }, demo.title),
        h('div', { style: styles.sub }, '右侧两个原生控件由 Rust safe wrapper 获取并写入初始内容。左侧是 Preact 面板，只负责展示状态与调用 Rust bind。'),
        Block('状态', demo.status),
        Block('日志导出预览', demo.log_preview),
        Block('终端快照预览', demo.terminal_preview),
        h('button', { style: styles.button, onClick: pingRust }, '调用 Rust bind()')
    );
}

render(h(App), document.getElementById('root'));
