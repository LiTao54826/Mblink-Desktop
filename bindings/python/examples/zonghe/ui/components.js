import { C } from './theme.js';

const { h } = Preact;

export function badge(label, tone) {
  return h('span', {
    style: {
      display: 'inline-flex', alignItems: 'center', gap: '4px',
      padding: '2px 8px', borderRadius: '6px',
      background: tone + '15', border: '1px solid ' + tone + '30',
      color: tone === C.muted ? C.text : tone,
      fontSize: '10px', fontWeight: 600, whiteSpace: 'nowrap'
    }
  }, [
    h('span', { style: { width: '4px', height: '4px', borderRadius: '50%', background: tone, boxShadow: '0 0 6px ' + tone } }),
    label || '--'
  ]);
}

export function panel(title, body, style) {
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
        letterSpacing: '0.04em', display: 'flex', alignItems: 'center', gap: '6px'
      }
    }, [title]) : null,
    body
  ]);
}

export function statCard(label, value, hint, tone) {
  return panel(null, h('div', { style: { display: 'flex', flexDirection: 'column', gap: '2px' } }, [
    h('div', { style: { color: C.muted, fontSize: '11px', fontWeight: 500 } }, label),
    h('div', { style: { color: C.text, fontSize: '24px', fontWeight: 700, letterSpacing: '-0.04em', lineHeight: 1.2 } }, value || '--'),
    h('div', { style: { color: tone, fontSize: '10px', fontWeight: 500, marginTop: '2px' } }, hint || '-')
  ]), { padding: '14px 16px', position: 'relative', overflow: 'hidden' });
}

export function row(label, value, options) {
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

export function btn(label, tone, onClick) {
  var isPrimary = tone === 'primary';
  var isDanger = tone === 'danger';
  var bg = isPrimary ? C.primary : isDanger ? C.danger : '#fff';
  var color = (isPrimary || isDanger) ? '#fff' : C.text;
  return h('button', {
    class: 'no-drag btn-hover', onClick: onClick,
    style: {
      background: bg, color: color, border: '1px solid ' + (isPrimary || isDanger ? bg : C.border),
      borderRadius: '6px', padding: '6px 12px', fontSize: '11px', fontWeight: 500,
      cursor: 'pointer', transition: 'all 0.15s ease', outline: 'none',
      display: 'flex', alignItems: 'center', justifyContent: 'center'
    }
  }, label);
}

export function ToastLayer(items) {
  var list = items || [];
  if (!list.length) return null;
  return h('div', {
    style: {
      position: 'fixed', top: '16px', left: '50%', transform: 'translateX(-50%)', zIndex: 100000,
      display: 'flex', flexDirection: 'column', gap: '10px', alignItems: 'center', pointerEvents: 'none'
    }
  }, list.map(function(item) {
    var tone = item.tone || 'info';
    var color = tone === 'success' ? C.success : tone === 'error' ? C.danger : tone === 'warning' ? C.warning : C.primary;
    return h('div', {
      key: item.id,
      style: {
        minWidth: '240px', maxWidth: '360px', padding: '12px 14px', borderRadius: '10px',
        background: '#ffffff', color: C.text, border: '1px solid ' + color + '33',
        boxShadow: C.shadow, display: 'flex', flexDirection: 'column', gap: '4px',
        whiteSpace: 'normal', overflowWrap: 'anywhere', wordBreak: 'break-word'
      }
    }, [
      h('div', { style: { color: color, fontSize: '11px', fontWeight: 700, letterSpacing: '0.03em', textAlign: 'center', whiteSpace: 'normal', overflowWrap: 'anywhere', wordBreak: 'break-word' } }, item.title || '提示'),
      h('div', { style: { fontSize: '12px', lineHeight: 1.5, wordBreak: 'break-word', overflowWrap: 'anywhere', whiteSpace: 'normal', textAlign: 'center' } }, item.message || '--')
    ]);
  }));
}

