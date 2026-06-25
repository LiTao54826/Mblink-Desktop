import { h, render } from 'preact';

const styles = {
    wrap: {
        height: '100%',
        display: 'flex',
        alignItems: 'center',
        justifyContent: 'center',
        background: 'linear-gradient(135deg, #1e293b 0%, #0f172a 100%)',
    },
    card: {
        minWidth: '320px',
        padding: '24px',
        borderRadius: '16px',
        background: 'rgba(15, 23, 42, 0.88)',
        boxShadow: '0 18px 50px rgba(0, 0, 0, 0.35)',
        textAlign: 'center',
    },
    title: { fontSize: '24px', fontWeight: '700', marginBottom: '8px' },
    sub: { color: '#94a3b8', marginBottom: '20px' },
    count: { fontSize: '64px', fontWeight: '800', marginBottom: '20px' },
    row: { display: 'flex', gap: '12px', justifyContent: 'center' },
    btn: {
        border: 'none',
        borderRadius: '10px',
        padding: '10px 18px',
        cursor: 'pointer',
        fontSize: '16px',
        fontWeight: '700',
    },
    secondary: { background: '#334155', color: '#e2e8f0' },
    primary: { background: '#22c55e', color: '#052e16' },
    footer: { marginTop: '18px', fontSize: '12px', color: '#94a3b8' },
};

function App() {
    return h('div', { style: styles.wrap },
        h('div', { style: styles.card },
            h('div', { style: styles.title }, data.title),
            h('div', { style: styles.sub }, 'Shared state + Preact UI loaded from Rust'),
            h('div', { style: styles.count }, String(data.count ?? 0)),
            h('div', { style: styles.row },
                h('button', {
                    style: { ...styles.btn, ...styles.secondary },
                    onClick: () => { data.count = (data.count ?? 0) - 1; rerender(); },
                }, '-1'),
                h('button', {
                    style: { ...styles.btn, ...styles.primary },
                    onClick: () => { data.count = (data.count ?? 0) + 1; rerender(); },
                }, '+1'),
                h('button', {
                    style: { ...styles.btn, ...styles.secondary },
                    onClick: async () => {
                        const result = await backend.greet({ from: 'preact' });
                        alert(JSON.stringify(result, null, 2));
                    },
                }, 'Rust bind'),
            ),
            h('div', { style: styles.footer }, 'Preact runtime is built into MBlink')
        )
    );
}

const root = document.getElementById('root') || document.body;
function rerender() {
    render(h(App), root);
}

rerender();
