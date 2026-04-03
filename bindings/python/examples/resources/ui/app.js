import { ToastLayer } from './components.js';
import { LoginView, WorkspaceView } from './views.js';

const { h, render } = Preact;
const { useMemo, useState, useEffect } = PreactHooks;

function createPyBridge() {
  const raw = globalThis.py || {};
  function wrapCall(handler, payload) {
    if (!handler) return Promise.reject(new Error('接口未绑定'));
    return Promise.resolve(handler(payload)).then(function(result) {
      if (result && result.error) {
        throw new Error(result.error);
      }
      return result || { ok: true };
    });
  }
  return {
    login_runtime(payload) {
      return wrapCall(raw.login_runtime, payload);
    },
    logout_runtime() {
      return wrapCall(raw.logout_runtime);
    },
    toggle_settings(payload) {
      return wrapCall(raw.toggle_settings, payload);
    },
    start_runtime() {
      return wrapCall(raw.start_runtime);
    },
    stop_runtime() {
      return wrapCall(raw.stop_runtime);
    },
    retry_task() {
      return wrapCall(raw.retry_task);
    },
    save_config(payload) {
      return wrapCall(raw.save_config, payload);
    }
  };
}

function App() {
  const [, forceUpdate] = useState(0);
  const s = globalThis.sys || {};
  const py = useMemo(createPyBridge, []);
  const [loginDraft, setLoginDraftState] = useState(s.login_form || {});
  const [settingsDraft, setSettingsDraftState] = useState(s.settings_form || {});
  const [toasts, setToasts] = useState([]);

  useEffect(function() {
    globalThis.__onSharedUpdate = function() {
      forceUpdate(function(v) { return v + 1; });
    };
    return function() {
      if (globalThis.__onSharedUpdate) {
        globalThis.__onSharedUpdate = null;
      }
    };
  }, []);

  useEffect(function() {
    setLoginDraftState(s.login_form || {});
    setSettingsDraftState(s.settings_form || {});
  }, [s.is_logged_in, s.settings_visible, s.current_user, s.runtime_status]);

  function setLoginDraft(key, value) {
    setLoginDraftState(function(prev) {
      return Object.assign({}, prev, { [key]: value });
    });
  }

  function setSettingsDraft(key, value) {
    setSettingsDraftState(function(prev) {
      return Object.assign({}, prev, { [key]: value });
    });
  }

  function notify(message, tone, title) {
    var id = Date.now() + Math.random();
    setToasts(function(prev) {
      return prev.concat([{ id: id, message: message, tone: tone || 'info', title: title || '提示' }]).slice(-4);
    });
    setTimeout(function() {
      setToasts(function(prev) {
        return prev.filter(function(item) { return item.id !== id; });
      });
    }, 2600);
  }

  if (!s.is_logged_in) {
    return h('div', null, [
      h(LoginView, { s, draft: loginDraft, setDraft: setLoginDraft, py, notify }),
      h(ToastLayer, toasts)
    ]);
  }

  return h('div', null, [
    h(WorkspaceView, {
      s,
      py,
      notify,
      settingsDraft,
      setSettingsDraft,
    }),
    h(ToastLayer, toasts)
  ]);
}

render(h(App), document.getElementById('root'));

