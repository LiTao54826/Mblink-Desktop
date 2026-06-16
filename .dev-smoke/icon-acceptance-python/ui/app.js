import { h, render } from 'preact';
import { useEffect, useMemo, useState } from 'preact/hooks';
import { hostApi } from './bridge.js';
import { palette, cardStyle, buttonStyle } from './styles/theme.js';

const PURPOSE = 'minimal';

const features = [
  { title: 'One shared UI', body: 'The same ui/app.js runs in mbink-ui-dev mock mode and in Python, Rust, or Go hosts.' },
  { title: 'Thin host API', body: 'UI calls getTemplateInfo, incrementCounter, and submitValidation; hosts provide real data.' },
  { title: 'Fast smoke test', body: 'Stable element ids support snapshot, click, input, Enter, and reload validation.' }
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
  const [info, setInfo] = useState({ host: 'loading', mode: 'loading', runtime: 'loading' });
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
    h('section', { style: { maxWidth: 1040, height: '100%', margin: '0 auto', display: 'grid', gap: '14px', alignContent: 'start', overflow: 'auto', paddingRight: 4 } },
      h('header', { style: { ...cardStyle, display: 'grid', gap: 8 } },
        h('div', { style: { color: palette.accent, fontSize: '13px', letterSpacing: '1.4px', textTransform: 'uppercase' } }, 'MBink UI Dev Template'),
        h('h1', { style: { margin: 0, fontSize: '30px' } }, 'Minimal Host-Ready Starter'),
        h('p', { style: { margin: 0, color: palette.muted, lineHeight: 1.45 } }, `Generated at ${generatedAt}. Running through ${info.host} / ${info.mode}.`)
      ),
      h('div', { style: { display: 'grid', gridTemplateColumns: 'repeat(auto-fit, minmax(220px, 1fr))', gap: '12px' } },
        features.map((item) => h('article', { key: item.title, style: cardStyle },
          h('strong', { style: { display: 'block', marginBottom: 8 } }, item.title),
          h('span', { style: { color: palette.muted, lineHeight: 1.5 } }, item.body)
        ))
      ),
      h('section', { style: cardStyle },
        h('h2', { style: { marginTop: 0 } }, 'Interactive state'),
        h('p', { style: { color: palette.muted } }, 'Click/input events go through the same UI while data comes from either the dev mock or the host binding.'),
        h('div', { style: { display: 'grid', gap: 12, maxWidth: 520 } },
          h('button', { id: 'counter-button', style: buttonStyle, onClick: incrementCounter }, `Clicked ${count} times`),
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
          h('button', { id: 'submit-button', style: buttonStyle, onClick: submitValidation }, 'Submit validation input'),
          h('p', { id: 'result-text', style: { margin: 0, color: palette.accent } }, submitted)
        )
      )
    )
  );
}

installDocumentShell();
render(h(App), mountRoot());
