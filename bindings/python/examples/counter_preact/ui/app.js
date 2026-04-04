/**
 * Preact Counter 示例 — UI 入口文件
 *
 * 全局可用变量：
 *   - Preact          → { h, render, Component, Fragment, ... }
 *   - PreactHooks     → { useState, useEffect, ... }
 *   - data            → 共享 C 对象（Python 侧 app.shared("data") 创建）
 *   - backend.increment()  → 调用 Python @app.bind("increment")
 *   - backend.decrement()  → 调用 Python @app.bind("decrement")
 *   - backend.reset()      → 调用 Python @app.bind("reset")
 */

const { h, render } = Preact;

// ====== 样式 ======
const styles = {
    container: {
        display: 'flex',
        flexDirection: 'column',
        alignItems: 'center',
        justifyContent: 'center',
        height: '100vh',
        fontFamily: '-apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif',
        background: 'linear-gradient(135deg, #667eea 0%, #764ba2 100%)',
        color: '#fff',
        margin: 0,
    },
    title: {
        fontSize: '18px',
        fontWeight: 300,
        marginBottom: '8px',
        opacity: 0.8,
    },
    count: {
        fontSize: '72px',
        fontWeight: 700,
        margin: '16px 0',
        textShadow: '0 2px 10px rgba(0,0,0,0.2)',
    },
    buttonGroup: {
        display: 'flex',
        gap: '12px',
        marginTop: '24px',
    },
    button: {
        padding: '12px 28px',
        fontSize: '18px',
        fontWeight: 600,
        border: 'none',
        borderRadius: '8px',
        cursor: 'pointer',
        transition: 'transform 0.1s, box-shadow 0.1s',
        boxShadow: '0 4px 15px rgba(0,0,0,0.2)',
    },
    btnPlus: {
        background: '#4CAF50',
        color: '#fff',
    },
    btnMinus: {
        background: '#f44336',
        color: '#fff',
    },
    btnReset: {
        background: 'rgba(255,255,255,0.2)',
        color: '#fff',
        fontSize: '14px',
        padding: '8px 20px',
    },
    footer: {
        marginTop: '32px',
        fontSize: '12px',
        opacity: 0.6,
    },
};

// ====== 组件 ======
function App() {
    return h('div', { style: styles.container },
        h('div', { style: styles.title }, '🐾 Preact + SharedObject Demo'),
        h('div', { style: styles.count }, data.count),
        h('div', { style: styles.buttonGroup },
            h('button', {
                style: { ...styles.button, ...styles.btnMinus },
                onClick: () => backend.decrement(),
            }, '− 1'),
            h('button', {
                style: { ...styles.button, ...styles.btnPlus },
                onClick: () => backend.increment(),
            }, '+ 1'),
        ),
        h('button', {
            style: { ...styles.button, ...styles.btnReset },
            onClick: () => backend.reset(),
        }, 'Reset'),
        h('div', { style: styles.footer },
            'Python ↔ C SharedObject ↔ Preact VDOM'
        ),
    );
}

// ====== 渲染 + 更新钩子 ======
var _root = document.getElementById('root');
function rerender() {
    render(h(App), _root);
}

// 首次渲染
rerender();

