import { h, render } from 'preact';
import { useEffect, useMemo, useState } from 'preact/hooks';
import { hostApi } from './bridge.js';
import { palette, cardStyle, buttonStyle } from './styles/theme.js';

const PURPOSE = 'desktop-app';

const features = [
  { title: 'Frameless shell', body: 'Borderless layout with drag regions and CSS window-control hit areas.' },
  { title: 'Tray lifecycle', body: 'Host templates create a tray icon, wire menu actions, and hide instead of quitting.' },
  { title: 'Shared frontend', body: 'The UI keeps the same API contract in dev mock mode and real host runtime mode.' }
];

function installDocumentShell() {
  Object.assign(document.documentElement.style, { width: '100%', height: '100%', margin: '0', overflow: 'hidden' });
  Object.assign(document.body.style, { width: '100%', height: '100%', margin: '0', overflow: 'hidden' });
}

function mountRoot() {
  let root = document.getElementById('root');
  if (!root) {
    root = document.createElement('div');
    root.id = 'root';
    document.body.appendChild(root);
  }
  Object.assign(root.style, { width: '100%', height: '100%', overflow: 'hidden' });
  return root;
}

function App() {
  const [info, setInfo] = useState({ host: 'loading', mode: 'loading', runtime: 'loading', capabilities: {} });
  const [count, setCount] = useState(0);
  const [draft, setDraft] = useState('');
  const [submitted, setSubmitted] = useState('nothing submitted yet');
  const [trayStatus, setTrayStatus] = useState('tray action not called yet');
  const generatedAt = useMemo(() => new Date().toLocaleString(), []);

  useEffect(() => {
    let active = true;
    hostApi.getTemplateInfo({ purpose: PURPOSE }).then((value) => {
      if (active) setInfo(value);
    });
    return () => {
      active = false;
    };
  }, []);

  async function incrementCounter() {
    const result = await hostApi.incrementCounter({ delta: 1, purpose: PURPOSE });
    setCount(Number(result.count ?? count + 1));
  }

  async function submitValidation() {
    const result = await hostApi.submitValidation({ text: draft, purpose: PURPOSE });
    setSubmitted(result.message ?? result.value ?? 'submitted');
  }

  async function runTrayAction(action) {
    const result = await hostApi.trayAction({ action, purpose: PURPOSE });
    setTrayStatus(result.message ?? `tray action: ${action}`);
  }

  return h('main', { style: { width: '100%', height: '100%', minHeight: 0, boxSizing: 'border-box', overflow: 'hidden', padding: '18px 24px 24px', fontFamily: 'Segoe UI, sans-serif', color: palette.text, background: palette.bg } },
    h('section', { style: { maxWidth: 1100, height: '100%', margin: '0 auto', display: 'grid', gap: '14px', alignContent: 'start', overflow: 'auto', paddingRight: 4 } },
      h('header', { style: { ...cardStyle, display: 'grid', gap: 8, WebkitAppRegion: 'drag' } },
        h('div', { style: { display: 'flex', justifyContent: 'space-between', alignItems: 'center', gap: 12 } },
          h('div', { style: { color: palette.accent, fontSize: '13px', letterSpacing: '1.4px', textTransform: 'uppercase' } }, 'MBlink Desktop Template'),
          h('div', { id: 'window-controls', style: { display: 'flex', gap: 8, WebkitAppRegion: 'no-drag' } },
            h('button', { id: 'window-minimize', title: 'minimize', style: { ...buttonStyle, padding: '6px 10px', WebkitWindowControl: 'minimize' } }, '-'),
            h('button', { id: 'window-maximize', title: 'maximize', style: { ...buttonStyle, padding: '6px 10px', WebkitWindowControl: 'maximize' } }, '□'),
            h('button', { id: 'window-close', title: 'close', style: { ...buttonStyle, padding: '6px 10px', background: '#fb7185', WebkitWindowControl: 'close' } }, '×')
          )
        ),
        h('h1', { style: { margin: 0, fontSize: '30px' } }, 'Frameless Desktop App Skeleton'),
        h('p', { style: { margin: 0, color: palette.muted, lineHeight: 1.45 } }, `Generated at ${generatedAt}. Current provider: ${info.host} / ${info.mode}.`)
      ),
      h('section', { id: 'tray-card', style: { ...cardStyle, display: 'grid', gap: 10 } },
        h('strong', null, 'Tray integration seam'),
        h('p', { style: { margin: 0, color: palette.muted, lineHeight: 1.5 } }, 'Desktop host templates wire create_tray, tray menu callbacks, close-to-tray behavior, and restore from tray click.'),
        h('div', { style: { display: 'flex', flexWrap: 'wrap', gap: 10 } },
          h('button', { id: 'tray-status-button', style: buttonStyle, onClick: () => runTrayAction('status') }, 'Check tray status'),
          h('button', { id: 'tray-show-button', style: buttonStyle, onClick: () => runTrayAction('show') }, 'Show window'),
          h('button', { id: 'tray-hide-button', style: buttonStyle, onClick: () => runTrayAction('hide') }, 'Hide to tray')
        ),
        h('p', { id: 'tray-result', style: { margin: 0, color: palette.accent } }, trayStatus)
      ),
      h('div', { style: { display: 'grid', gridTemplateColumns: 'repeat(auto-fit, minmax(220px, 1fr))', gap: '12px' } },
        features.map((item) => h('article', { key: item.title, style: cardStyle },
          h('strong', { style: { display: 'block', marginBottom: 8 } }, item.title),
          h('span', { style: { color: palette.muted, lineHeight: 1.5 } }, item.body)
        ))
      ),
      h('section', { style: cardStyle },
        h('h2', { style: { marginTop: 0 } }, 'Interactive state'),
        h('p', { style: { color: palette.muted } }, 'Keyboard and click events stay in the UI; data and native actions are supplied by the active host.'),
        h('div', { style: { display: 'grid', gap: 12, maxWidth: 540 } },
          h('button', { id: 'counter-button', style: buttonStyle, onClick: incrementCounter }, `Host counter: ${count}`),
          h('label', { style: { display: 'grid', gap: 6, color: palette.muted } },
            'Validation input',
            h('input', {
              id: 'validation-input',
              type: 'text',
              value: draft,
              placeholder: 'type here',
              onInput: (event) => setDraft(event.currentTarget.value),
              onKeyDown: (event) => {
                if (event.key === 'Enter') submitValidation();
              },
              style: { padding: '10px 12px', borderRadius: 12, border: `1px solid ${palette.border}`, color: palette.text, background: 'rgba(15, 23, 42, 0.9)' }
            })
          ),
          h('button', { id: 'submit-button', style: buttonStyle, onClick: submitValidation }, 'Submit through backend'),
          h('p', { id: 'result-text', style: { margin: 0, color: palette.accent } }, submitted)
        )
      )
    )
  );
}

installDocumentShell();
render(h(App), mountRoot());
