const { h } = Preact;

import { C, statusTone } from './theme.js';
import { badge, panel, statCard, row, btn } from './components.js';

function formRow(label, field) {
  return h('label', { style: { display: 'flex', flexDirection: 'column', gap: '6px' } }, [
    h('span', { style: { color: C.muted, fontSize: '11px', fontWeight: 600 } }, label),
    field
  ]);
}

function textInput(value, placeholder, password, onInput) {
  return h('input', {
    class: 'no-drag',
    type: password ? 'password' : 'text',
    value: value || '',
    placeholder: placeholder,
    onInput: function(e) { return onInput(e.target.value); },
    style: {
      width: '100%', height: '36px', padding: '0 12px', borderRadius: '8px',
      border: '1px solid ' + C.border, background: '#fff', color: C.text, fontSize: '12px', outline: 'none'
    }
  });
}

export function TitleBar(props) {
  const s = props || {};
  const showStatus = !!s.showStatus;
  return h('div', {
    class: 'drag',
    style: {
      height: '52px', flexShrink: 0, display: 'flex', alignItems: 'center', justifyContent: 'space-between',
      padding: '0 16px', borderBottom: '1px solid ' + C.border,
      background: 'rgba(255, 255, 255, 0.8)', backdropFilter: 'blur(20px)', WebkitBackdropFilter: 'blur(20px)',
      userSelect: 'none', zIndex: 100, position: 'sticky', top: 0,
    }
  }, [
    h('div', { style: { display: 'flex', alignItems: 'center', gap: '10px' } }, [
      h('img', { src: 'app://res.ico', class: 'no-drag', style: { width: '20px', height: '20px', borderRadius: '4px' } }),
      h('div', { style: { color: C.text, fontWeight: 600, fontSize: '13px' } }, s.robot_name || '票据机器人客户端'),
    ]),
    h('div', { class: 'no-drag', style: { display: 'flex', gap: '12px', alignItems: 'center' } }, [
      showStatus ? badge(s.online_status || '在线', C.success) : null,
      showStatus ? badge(s.runtime_status || '空闲', statusTone(s.runtime_status)) : null,
      h('div', { class: 'window-controls no-drag', style: { display: 'flex', gap: '8px', marginLeft: showStatus ? '8px' : '0' } }, [
        h('div', { class: 'window-control wc-minimize no-drag', style: dotStyle('#fbbf24', '#f59e0b') }),
        h('div', { class: 'window-control wc-maximize no-drag', style: dotStyle('#34d399', '#10b981') }),
        h('div', { class: 'window-control wc-close no-drag', style: dotStyle('#f87171', '#ef4444') })
      ])
    ])
  ]);
}

function dotStyle(bg, border) {
  return {
    width: '12px', height: '12px', borderRadius: '50%', background: bg, cursor: 'pointer',
    border: '1px solid ' + border, boxShadow: 'inset 0 1px 2px rgba(255,255,255,0.4)'
  };
}

export function LoginView(props) {
  const s = props.s || {};
  const draft = props.draft || {};
  const py = props.py || {};
  async function handleLogin() {
    try {
      await (py.login_runtime && py.login_runtime(draft));
    } catch (err) {
      console.error(err);
    }
  }
  return h('div', { style: { width: '100vw', height: '100vh', display: 'flex', flexDirection: 'column', background: C.bg } }, [
    h(TitleBar, { ...s, showStatus: false }),
    h('div', { style: { flex: 1, display: 'flex', alignItems: 'center', justifyContent: 'center', padding: '24px' } }, [
      panel('登录', h('div', { style: { width: '380px', display: 'flex', flexDirection: 'column', gap: '12px' } }, [
        formRow('接口地址', textInput(draft.base_url, '请输入 BASE_URL', false, function(v) { return props.setDraft('base_url', v); })),
        formRow('用户名', textInput(draft.username, '请输入用户名', false, function(v) { return props.setDraft('username', v); })),
        formRow('密码', textInput(draft.password, '请输入密码', true, function(v) { return props.setDraft('password', v); })),
        h('div', { style: { color: C.muted, fontSize: '11px', lineHeight: 1.5, minHeight: '16px' } }, s.current_action || '请输入账号信息后登录'),
        h('div', { style: { display: 'flex', justifyContent: 'flex-end', marginTop: '4px' } }, [
          btn('登录', 'primary', handleLogin)
        ])
      ]), { width: '420px', padding: '20px' })
    ])
  ]);
}

export function WorkspaceView(props) {
  const s = props.s || {};
  const py = props.py || {};
  return h('div', { style: { width: '100vw', height: '100vh', display: 'flex', flexDirection: 'column' } }, [
    h(TitleBar, { ...s, showStatus: true }),
    h('div', { style: { flex: 1, padding: '16px', display: 'flex', flexDirection: 'column', gap: '16px', maxWidth: '1200px', margin: '0 auto', width: '100%', boxSizing: 'border-box', overflow: 'hidden' } }, [
      h('div', { style: { display: 'grid', gridTemplateColumns: 'repeat(4, 1fr)', gap: '16px' } }, [
        statCard('核心状态', s.runtime_status || '空闲', '引擎运行中', statusTone(s.runtime_status)),
        statCard('今日成功', s.today_success || '0', '累计处理量', C.success),
        statCard('最近耗时', s.task_elapsed || '0s', '上次任务用时', C.primary),
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
            btn('启动', 'primary', function() { return backend.start_runtime && backend.start_runtime(); }),
            btn('重试', null, function() { return backend.retry_task && backend.retry_task(); }),
            btn('停止', 'danger', function() { return backend.stop_runtime && backend.stop_runtime(); }),
            btn('设置', null, function() { return backend.toggle_settings && backend.toggle_settings({ visible: true }); })
          ]))
        ]),
        panel('运行日志', h('logview', { id: 'work-log', style: { flex: 1, width: '100%', height: '100%', display: 'block', minHeight: '0', background: C.logBg, borderRadius: '8px', overflow: 'hidden', border: '1px solid ' + C.logBorder } }), { padding: '8px' })
      ]),
      s.settings_visible ? h(SettingsDialog, props) : null
    ])
  ]);
}

export function SettingsDialog(props) {
  const s = props.s || {};
  const form = props.settingsDraft || {};
  const py = props.py || {};
  return h('div', { style: { position: 'fixed', left: 0, top: 0, right: 0, bottom: 0, background: 'rgba(15, 23, 42, 0.28)', display: 'flex', alignItems: 'center', justifyContent: 'center', zIndex: 99999 } }, [
    h('div', { class: 'no-drag', style: { background: C.panel, border: '1px solid ' + C.border, borderRadius: '12px', boxShadow: C.shadow, padding: '20px', width: '520px', display: 'flex', flexDirection: 'column', gap: '12px', transform: 'translate(0, -10vh)' } }, [
      h('div', { style: { color: C.muted, fontSize: '12px', fontWeight: 600, marginBottom: '8px', letterSpacing: '0.04em', display: 'flex', alignItems: 'center', gap: '6px' } }, ['系统设置']),
      formRow('AES_KEY', textInput(form.aes_key, '请输入 AES_KEY', false, function(v) { return props.setSettingsDraft('aes_key', v); })),
      formRow('AES_IV', textInput(form.aes_iv, '请输入 AES_IV', false, function(v) { return props.setSettingsDraft('aes_iv', v); })),
      formRow('UKEY_PASSWORD', textInput(form.ukey_password, '请输入 UKey 密码', true, function(v) { return props.setSettingsDraft('ukey_password', v); })),
      formRow('轮询间隔', textInput(String(form.poll_interval || ''), '秒', false, function(v) { return props.setSettingsDraft('poll_interval', v); })),
      formRow('Debug Port', textInput(String(form.debug_port || ''), '请输入端口', false, function(v) { return props.setSettingsDraft('debug_port', v); })),
      h('label', { style: { display: 'flex', alignItems: 'center', gap: '8px', color: C.text, fontSize: '12px' } }, [
        h('input', { type: 'checkbox', checked: !!form.auto_start, onChange: function(e) { return props.setSettingsDraft('auto_start', !!e.target.checked); } }),
        '启动后自动监听'
      ]),
      h('div', { style: { display: 'flex', justifyContent: 'flex-end', gap: '8px', marginTop: '4px' } }, [
        btn('关闭', null, function() { return backend.toggle_settings && backend.toggle_settings({ visible: false }); }),
        btn('保存', 'primary', function() { return backend.save_config && backend.save_config(form); })
      ])
    ])
  ]);
}

