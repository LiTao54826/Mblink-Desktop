import { h, render } from 'preact';
import { useEffect, useMemo, useRef, useState } from 'preact/hooks';

globalThis.__MBINK_LEAK_PROBE = true;

const probeState = globalThis.__probeAppState || (globalThis.__probeAppState = {
  appRenderCount: 0,
  panelRenderCount: 0,
  labelRenderCount: 0,
  mountCount: 0,
  unmountCount: 0,
  effectCount: 0,
  summaryTimer: null,
});

function probeLog(tag, payload) {
  try {
    console.log('[LEAK_PROBE_APP][' + tag + ']', JSON.stringify(payload || {}));
  } catch (_) {
    console.log('[LEAK_PROBE_APP][' + tag + ']', payload || {});
  }
}

function HeartbeatLabel(props) {
  probeState.labelRenderCount++;
  const vnodeRef = useRef('label-ref-' + probeState.labelRenderCount);
  useEffect(function() {
    probeState.mountCount++;
    probeLog('HeartbeatLabel.mount', { vnodeRef: vnodeRef.current, mounts: probeState.mountCount });
    return function() {
      probeState.unmountCount++;
      probeLog('HeartbeatLabel.unmount', { vnodeRef: vnodeRef.current, unmounts: probeState.unmountCount });
    };
  }, []);

  return h('div', {
    style: {
      padding: '12px 16px', border: '1px solid #2b3442', borderRadius: '10px',
      background: '#151b23', color: '#7ee787', fontSize: '28px', fontWeight: '700'
    }
  }, 'heartbeat_at = ' + props.value);
}

function HeartbeatPanel(props) {
  probeState.panelRenderCount++;
  const lastValueRef = useRef(null);
  const derived = useMemo(function() {
    return {
      text: String(props.heartbeatAt || '-'),
      tick: Number(props.tick || 0),
    };
  }, [props.heartbeatAt, props.tick]);

  useEffect(function() {
    probeState.effectCount++;
    probeLog('HeartbeatPanel.effect', {
      effectCount: probeState.effectCount,
      tick: derived.tick,
      heartbeatAt: derived.text,
      prevHeartbeatAt: lastValueRef.current,
    });
    lastValueRef.current = derived.text;
  }, [derived.text, derived.tick]);

  return h('div', { style: { display: 'flex', flexDirection: 'column', gap: '12px' } }, [
    h('div', { style: { color: '#8b949e', fontSize: '14px' } }, 'tick = ' + derived.tick),
    h(HeartbeatLabel, { value: derived.text }),
  ]);
}

function App() {
  probeState.appRenderCount++;
  const [, forceUpdate] = useState(0);
  const s = globalThis.sys || {};

  useEffect(function() {
    probeLog('App.mount', { appRenderCount: probeState.appRenderCount });
    globalThis.__onSharedUpdate = function() {
      probeLog('App.onSharedUpdate', { tick: s.tick, heartbeatAt: s.heartbeat_at });
      forceUpdate(function(v) { return v + 1; });
    };
    if (!probeState.summaryTimer) {
      probeState.summaryTimer = setInterval(function() {
        const runtime = globalThis.__mbinkSharedRuntime || {};
        const hookDebug = globalThis.__mbinkHookDebug || {};
        probeLog('Summary', {
          appRenderCount: probeState.appRenderCount,
          panelRenderCount: probeState.panelRenderCount,
          labelRenderCount: probeState.labelRenderCount,
          mountCount: probeState.mountCount,
          unmountCount: probeState.unmountCount,
          effectCount: probeState.effectCount,
          rootCount: runtime.roots ? runtime.roots.length : -1,
          proxyKeyCount: runtime.proxyCache ? Object.keys(runtime.proxyCache).length : -1,
          cleanupCount: hookDebug.cleanupCount || 0,
          tick: s.tick,
          heartbeatAt: s.heartbeat_at,
        });
      }, 3000);
    }
    return function() {
      if (globalThis.__onSharedUpdate) globalThis.__onSharedUpdate = null;
      if (probeState.summaryTimer) {
        clearInterval(probeState.summaryTimer);
        probeState.summaryTimer = null;
      }
    };
  }, []);

  probeLog('App.render', {
    appRenderCount: probeState.appRenderCount,
    tick: s.tick,
    heartbeatAt: s.heartbeat_at,
  });

  return h('div', {
    style: {
      height: '100%', display: 'flex', alignItems: 'center', justifyContent: 'center',
      padding: '24px'
    }
  }, h('div', {
    style: {
      width: '720px', display: 'flex', flexDirection: 'column', gap: '16px',
      padding: '24px', borderRadius: '14px', background: '#0d1117', border: '1px solid #30363d'
    }
  }, [
    h('div', { style: { fontSize: '22px', fontWeight: '700' } }, 'SharedState heartbeat leak probe'),
    h('div', { style: { color: '#8b949e', fontSize: '13px' } }, '只更新 sys.heartbeat_at / sys.tick，观察 vnode/component/hook cleanup 是否跟上'),
    h(HeartbeatPanel, { heartbeatAt: s.heartbeat_at, tick: s.tick }),
  ]));
}

render(h(App), document.getElementById('root') || document.body);
