import { h, render } from 'preact';
import { useEffect, useMemo, useState } from 'preact/hooks';
import { hostApi } from './bridge.js';
import { palette, cardStyle, buttonStyle } from './styles/theme.js';

const PURPOSE = 'showcase';

const features = [
  { title: 'Backend binding', body: 'The UI uses a named backend API contract instead of hard-coded language details.' },
  { title: 'Runtime swap', body: 'mbink-ui-dev supplies mock responses; Python, Rust, and Go supply real handlers.' },
  { title: 'Native seams', body: 'The same pattern extends to tray, window lifecycle, resources, and host state.' }
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

function CapabilityList({ info }) {
  const capabilities = info.capabilities ?? {};
  const items = [
    ['Backend', capabilities.backend ? 'host binding' : 'dev mock'],
    ['Borderless', capabilities.borderless ? 'enabled' : 'off'],
    ['Tray', capabilities.tray ? 'available' : 'not wired'],
    ['Runtime', info.runtime ?? 'unknown']
  ];
  return h('div', { style: { display: 'grid', gridTemplateColumns: 'repeat(auto-fit, minmax(160px, 1fr))', gap: 10 } },
    items.map(([label, value]) => h('div', { key: label, style: { ...cardStyle, padding: 14 } },
      h('div', { style: { color: palette.muted, fontSize: 12, textTransform: 'uppercase', letterSpacing: 1 } }, label),
      h('strong', { style: { display: 'block', marginTop: 6 } }, value)
    ))
  );
}

function App() {
  const [info, setInfo] = useState({ host: 'loading', mode: 'loading', runtime: 'loading', capabilities: {} });
  const [count, setCount] = useState(0);
  const [draft, setDraft] = useState('');
  const [submitted, setSubmitted] = useState('nothing submitted yet');
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

  return h('main', { style: { width: '100%', height: '100%', minHeight: 0, boxSizing: 'border-box', overflow: 'hidden', padding: '24px', fontFamily: 'Segoe UI, sans-serif', color: palette.text, background: palette.bg } },
    h('section', { style: { maxWidth: 1080, height: '100%', margin: '0 auto', display: 'grid', gap: '14px', alignContent: 'start', overflow: 'auto', paddingRight: 4 } },
      h('header', { style: { ...cardStyle, display: 'grid', gap: 8 } },
        h('div', { style: { color: palette.accent, fontSize: '13px', letterSpacing: '1.4px', textTransform: 'uppercase' } }, 'MBink Capability Showcase'),
        h('h1', { style: { margin: 0, fontSize: '30px' } }, 'Host API Showcase Template'),
        h('p', { style: { margin: 0, color: palette.muted, lineHeight: 1.45 } }, `Generated at ${generatedAt}. Current provider: ${info.host} / ${info.mode}.`)
      ),
      h(CapabilityList, { info }),
      h('div', { style: { display: 'grid', gridTemplateColumns: 'repeat(auto-fit, minmax(220px, 1fr))', gap: '12px' } },
        features.map((item) => h('article', { key: item.title, style: cardStyle },
          h('strong', { style: { display: 'block', marginBottom: 8 } }, item.title),
          h('span', { style: { color: palette.muted, lineHeight: 1.5 } }, item.body)
        ))
      ),
      h('section', { style: cardStyle },
        h('h2', { style: { marginTop: 0 } }, 'Binding contract'),
        h('p', { style: { color: palette.muted } }, 'These controls exercise the API names every host template implements. Replace the handler bodies with real business logic.'),
        h('div', { style: { display: 'grid', gap: 12, maxWidth: 540 } },
          h('button', { id: 'counter-button', style: buttonStyle, onClick: incrementCounter }, `Host counter: ${count}`),
          h('label', { style: { display: 'grid', gap: 6, color: palette.muted } },
            'Validation input',
            h('input', {
              id: 'validation-input',
              type: 'text',
              value: draft,
              placeholder: 'send data to the host',
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
