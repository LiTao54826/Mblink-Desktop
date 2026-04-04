import { C, statusTone } from './theme.js';
import { badge, panel, statCard, row, btn } from './components.js';

const { h } = Preact;

export function TitleBar(s) {
  return h('div', {
    class: 'drag',
    style: {
      height: '52px', flexShrink: 0,
      display: 'flex', alignItems: 'center', justifyContent: 'space-between',
      padding: '0 16px', borderBottom: '1px solid ' + C.border,
      background: 'rgba(255, 255, 255, 0.8)', backdropFilter: 'blur(20px)',
      WebkitBackdropFilter: 'blur(20px)', userSelect: 'none', zIndex: 100,
      position: 'sticky', top: 0,
    }
  }, [
    h('div', { style: { display: 'flex', alignItems: 'center', gap: '10px' } }, [
      h('img', { src: 'app://res.ico', class: 'no-drag', style: { width: '20px', height: '20px', borderRadius: '4px' } }),
      h('div', { style: { color: C.text, fontWeight: 600, fontSize: '13px' } }, s.robot_name || '票据机器人客户端'),
    ]),
    h('div', { class: 'no-drag', style: { display: 'flex', gap: '12px', alignItems: 'center' } }, [
      badge(s.online_status || '在线', C.success),
      badge(s.runtime_status || '空闲', statusTone(s.runtime_status)),
      h('div', { class: 'window-controls no-drag', style: { display: 'flex', gap: '8px', marginLeft: '8px' } }, [
        h('div', { class: 'window-control wc-minimize no-drag', style: dot('#fbbf24', '#f59e0b') }),
        h('div', { class: 'window-control wc-maximize no-drag', style: dot('#34d399', '#10b981') }),
        h('div', { class: 'window-control wc-close no-drag', style: dot('#f87171', '#ef4444') })
      ])
    ])
  ]);
}

function dot(bg, border) {
  return {
    width: '12px', height: '12px', borderRadius: '50%', background: bg, cursor: 'pointer',
    border: '1px solid ' + border, boxShadow: 'inset 0 1px 2px rgba(255,255,255,0.4)'
  };
}

export function MainLayout(s, py) {
  return h('div', { style: { width: '100vw', height: '100vh', display: 'flex', flexDirection: 'column' } }, [
    h(TitleBar, { ...s }),
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
            btn('启动', 'primary', function() { return backend.start_listen(); }),
            btn('重试', null, function() { return backend.retry_task(); }),
            btn('停止', 'danger', function() { return backend.stop_task(); }),
            btn('设置', null, function() { return backend.reset_state(); })
          ]))
        ]),
        panel('运行日志', h('logview', {
          id: 'work-log',
          style: {
            flex: 1, width: '100%', height: '100%', display: 'block', minHeight: '0',
            background: C.logBg, borderRadius: '8px', overflow: 'hidden', border: '1px solid ' + C.logBorder
          }
        }), { padding: '8px' })
      ])
    ])
  ]);
}

