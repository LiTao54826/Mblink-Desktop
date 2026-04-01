const { h, render } = Preact;

// --- 极致现代、精致小巧的配色 ---
const C = {
  bg: '#f3f4f6',
  panel: '#ffffff',
  panelAlt: '#f9fafb',
  border: 'rgba(0, 0, 0, 0.06)',
  text: '#111827',
  muted: '#6b7280',
  primary: '#6366f1',
  success: '#10b981',
  warning: '#f59e0b',
  danger: '#ef4444',
  shadow: '0 1px 2px rgba(0,0,0,0.02), 0 4px 12px rgba(0,0,0,0.03), 0 12px 24px -4px rgba(0,0,0,0.03)',
  logBg: '#09090b',
  logBorder: '#27272a'
};

function statusTone(status) {
  var colors = { '空闲': C.muted, '监听中': C.primary, '执行中': C.success, '资源告警': C.warning, '异常': C.danger };
  return colors[status] || C.primary;
}

function badge(label, tone) {
  return h('span', {
    style: {
      display: 'inline-flex', alignItems: 'center', gap: '4px',
      padding: '2px 8px', borderRadius: '6px',
      background: tone + '15',
      border: '1px solid ' + tone + '30',
      color: tone === C.muted ? C.text : tone,
      fontSize: '10px', fontWeight: 600, whiteSpace: 'nowrap'
    }
  }, [
    h('span', { style: { width: '4px', height: '4px', borderRadius: '50%', background: tone, boxShadow: '0 0 6px ' + tone } }),
    label || '--'
  ]);
}

function panel(title, body, style) {
  return h('section', {
    style: Object.assign({
      background: C.panel, border: '1px solid ' + C.border,
      borderRadius: '12px', boxShadow: C.shadow,
      padding: '16px', display: 'flex', flexDirection: 'column', minHeight: 0
    }, style || {})
  }, [
    title ? h('div', {
      style: {
        color: C.muted, fontSize: '10px', fontWeight: 600, marginBottom: '12px',
        letterSpacing: '0.04em', display: 'flex', alignItems: 'center', gap: '6px',
      }
    }, [title]) : null,
    body
  ]);
}

function statCard(label, value, hint, tone) {
  return panel(null, h('div', { style: { display: 'flex', flexDirection: 'column', gap: '2px' } }, [
    h('div', { style: { color: C.muted, fontSize: '11px', fontWeight: 500 } }, label),
    h('div', { style: { color: C.text, fontSize: '24px', fontWeight: 700, letterSpacing: '-0.04em', lineHeight: 1.2 } }, value || '--'),
    h('div', { style: { color: tone, fontSize: '10px', fontWeight: 500, marginTop: '2px' } }, hint || '-')
  ]), { padding: '14px 16px', position: 'relative', overflow: 'hidden' });
}

function row(label, value, options) {
  var opts = options || {};
  return h('div', {
    style: {
      display: 'flex', justifyContent: 'space-between', alignItems: 'center',
      padding: '8px 0', borderBottom: '1px solid ' + C.border
    }
  }, [
    h('div', { style: { color: C.muted, fontSize: '11px', flexShrink: 0 } }, label),
    h('div', { style: { color: C.text, fontSize: '11px', fontWeight: 500, textAlign: 'right', wordBreak: opts.breakAll ? 'break-all' : 'break-word' } }, value || '--')
  ]);
}

function btn(label, tone, onClick) {
  var isPrimary = tone === 'primary';
  var isDanger = tone === 'danger';
  var bg = isPrimary ? C.primary : isDanger ? C.danger : '#fff';
  var color = (isPrimary || isDanger) ? '#fff' : C.text;

  return h('button', {
    class: 'no-drag btn-hover',
    onClick: onClick,
    style: {
      background: bg, color: color, border: '1px solid ' + (isPrimary || isDanger ? bg : C.border),
      borderRadius: '6px', padding: '6px 12px', fontSize: '11px',
      fontWeight: 500, cursor: 'pointer', transition: 'all 0.15s ease',
      outline: 'none', display: 'flex', alignItems: 'center', justifyContent: 'center'
    }
  }, label);
}

function TitleBar(s) {
  return h('div', {
    class: 'drag',
    style: {
      height: '52px', flexShrink: 0,
      display: 'flex', alignItems: 'center', justifyContent: 'space-between',
      padding: '0 16px', borderBottom: '1px solid ' + C.border,
      background: 'rgba(255, 255, 255, 0.8)', backdropFilter: 'blur(20px)',
      WebkitBackdropFilter: 'blur(20px)',
      userSelect: 'none', zIndex: 100, position: 'sticky', top: 0,
    }
  }, [
    h('div', { style: { display: 'flex', alignItems: 'center', gap: '10px' } }, [
      h('img', { src: 'app://res.ico', class: 'no-drag', style: { width: '20px', height: '20px', borderRadius: '4px' } }),
      h('div', { style: { color: C.text, fontWeight: 600, fontSize: '13px' } }, s.robot_name || '票据机器人客户端'),
    ]),
    h('div', { class: 'no-drag', style: { display: 'flex', gap: '12px', alignItems: 'center' } }, [
      badge(s.online_status || '在线', C.success),
      badge(s.runtime_status || '空闲', statusTone(s.runtime_status)),
      // 修复后的控制按钮
      h('div', { class: 'window-controls no-drag', style: { display: 'flex', gap: '8px', marginLeft: '8px' } }, [
        h('div', {
          class: 'window-control wc-minimize no-drag', style: {
            width: '12px', height: '12px', borderRadius: '50%', background: '#fbbf24', cursor: 'pointer',
            border: '1px solid #f59e0b', boxShadow: 'inset 0 1px 2px rgba(255,255,255,0.4)'
          }
        }),
        h('div', {
          class: 'window-control wc-maximize no-drag', style: {
            width: '12px', height: '12px', borderRadius: '50%', background: '#34d399', cursor: 'pointer',
            border: '1px solid #10b981', boxShadow: 'inset 0 1px 2px rgba(255,255,255,0.4)'
          }
        }),
        h('div', {
          class: 'window-control wc-close no-drag', style: {
            width: '12px', height: '12px', borderRadius: '50%', background: '#f87171', cursor: 'pointer',
            border: '1px solid #ef4444', boxShadow: 'inset 0 1px 2px rgba(255,255,255,0.4)'
          }
        })
      ])
    ])
  ]);
}

function App() {
  var s = globalThis.sys || {};
  var logs = [
    { level: 'INFO', text: 'Work Client 初始化完成', time: new Date().toLocaleTimeString() },
    { level: 'INFO', text: '环境检查: ' + (s.robot_name || '正常'), time: new Date().toLocaleTimeString() },
    { level: s.last_log_level || 'INFO', text: s.last_log_text || '系统就绪，等待任务...', time: s.timestamp || '--' }
  ];
  
  return h('div', { style: { width: '100vw', height: '100vh', display: 'flex', flexDirection: 'column' } }, [
    TitleBar(s),
    h('div', { style: { flex: 1, padding: '16px', display: 'flex', flexDirection: 'column', gap: '16px', maxWidth: '1200px', margin: '0 auto', width: '100%', boxSizing: 'border-box', overflow: 'hidden' } }, [
      h('div', { style: { display: 'grid', gridTemplateColumns: 'repeat(4, 1fr)', gap: '16px' } }, [
        statCard('核心状态', s.runtime_status || '空闲', '引擎运行中', statusTone(s.runtime_status)),
        statCard('今日成功', s.today_success || '0', '累计处理量', C.success),
        statCard('最近耗时', s.task_elapsed || '0ms', '上次任务用时', C.primary),
        statCard('当前用户', s.current_user || '--', '登录凭证', C.muted),
      ]),
      h('div', { style: { flex: 1, display: 'grid', gridTemplateColumns: '260px 1fr', gap: '16px', minHeight: 0 } }, [
        h('div', { style: { display: 'flex', flexDirection: 'column', gap: '16px' } }, [
          panel('任务详情', h('div', null, [
            row('任务ID', s.current_task_id, { breakAll: true }),
            row('类型', s.current_task_type),
            row('当前阶段', s.current_stage),
            row('账户', s.current_account),
            row('最近心跳', s.heartbeat_at),
          ]), { flex: 1 }),
          panel('指令控制', h('div', { style: { display: 'grid', gridTemplateColumns: '1fr 1fr', gap: '8px' } }, [
            btn('启动', 'primary', function() { return py.start_listen(); }),
            btn('重试', null, function() { return py.retry_task(); }),
            btn('停止', 'danger', function() { return py.stop_task(); }),
            btn('设置', null, function() {  return py.reset_state(); })
          ]))
        ]),
        panel(null, h('div', {
          style: {
            flex: 1, background: C.logBg, borderRadius: '8px', padding: '12px',
            overflowY: 'auto', display: 'flex', flexDirection: 'column', gap: '6px',
            fontFamily: 'Consolas, monospace', fontSize: '11px', border: '1px solid ' + C.logBorder
          }
        }, logs.map((item, idx) => {
          let c = item.level === 'ERROR' ? C.danger : item.level === 'WARN' ? C.warning : '#a1a1aa';
          return h('div', { key: idx, style: { display: 'flex', gap: '10px', color: '#e4e4e7', lineHeight: 1.5 } }, [
            h('span', { style: { color: '#52525b' } }, item.time),
            h('span', { style: { color: c, fontWeight: 700 } }, '[' + item.level + ']'),
            h('span', { style: { flex: 1, wordBreak: 'break-all' } }, item.text)
          ]);
        })), { padding: '8px' })
      ])
    ])
  ]);
}


var root = document.getElementById('root');
function rerender() { render(h(App), root); }

var pending = false;
globalThis.__onSharedUpdate = function() {
  if (!pending) {
    pending = true;
    setTimeout(function() { pending = false; rerender(); }, 0);
  }
};

rerender();

