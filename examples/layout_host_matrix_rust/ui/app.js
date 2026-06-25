import { h, render } from 'preact';
import { useEffect, useMemo, useState } from 'preact/hooks';
import { hostApi } from './bridge.js';

const PURPOSE = 'layout-host-matrix-rust';

const rowsSeed = [
  ['Flex wrap', 'Long labels, shrinking controls, nested inline actions'],
  ['Grid minmax', 'Auto-fit columns with fixed-height children'],
  ['Overflow', 'Nested scroll areas and clipped badges'],
  ['Absolute overlay', 'Positioned status over dynamic content'],
  ['Input events', 'Controlled input, Enter key, host validation'],
  ['Host mutation', 'Rust state changes row counts and UI geometry'],
  ['Error path', 'Host returns structured failure without crashing UI'],
  ['Mixed density', 'Tiny chips beside larger action targets']
];

const S = {
  shell: {
    width: '100%',
    height: '100%',
    display: 'grid',
    gridTemplateRows: 'auto auto minmax(0, 1fr)',
    gap: 12,
    padding: 16,
    overflow: 'hidden',
    fontFamily: '"Segoe UI", Arial, sans-serif',
    color: '#172033',
    background: '#f4f6f8',
    boxSizing: 'border-box'
  },
  header: {
    display: 'grid',
    gridTemplateColumns: 'minmax(0, 1fr) auto',
    gap: 14,
    alignItems: 'start',
    padding: '14px 16px',
    border: '1px solid #cfdae6',
    borderRadius: 8,
    background: '#ffffff'
  },
  eyebrow: {
    margin: '0 0 4px',
    color: '#28706b',
    fontSize: 12,
    fontWeight: 700,
    textTransform: 'uppercase'
  },
  title: { margin: '0 0 4px', fontSize: 26, lineHeight: 1.18 },
  summary: { margin: 0, color: '#64748b', lineHeight: 1.4 },
  button: {
    minHeight: 34,
    border: '1px solid #9fb3c8',
    borderRadius: 7,
    padding: '7px 11px',
    color: '#102235',
    background: '#ffffff',
    cursor: 'pointer'
  },
  statusStrip: { display: 'flex', flexWrap: 'wrap', gap: 8, minHeight: 34 },
  pill: {
    display: 'inline-flex',
    alignItems: 'center',
    gap: 8,
    minWidth: 128,
    padding: '7px 10px',
    border: '1px solid #c7d2df',
    borderRadius: 999,
    background: '#ffffff'
  },
  workbench: {
    minHeight: 0,
    display: 'grid',
    gridTemplateColumns: 'minmax(0, 1fr) minmax(280px, 340px)',
    gap: 12,
    overflow: 'hidden'
  },
  primaryColumn: {
    minHeight: 0,
    display: 'grid',
    gridTemplateRows: 'minmax(0, 1fr) 210px',
    gap: 12,
    overflow: 'hidden'
  },
  sideRail: {
    minHeight: 0,
    display: 'grid',
    gridTemplateRows: 'auto minmax(120px, 1fr)',
    gap: 12,
    overflow: 'hidden'
  },
  matrix: {
    minHeight: 0,
    display: 'grid',
    gridTemplateColumns: 'repeat(auto-fit, minmax(190px, 1fr))',
    gridAutoRows: 'minmax(134px, auto)',
    gap: 10,
    overflow: 'auto',
    padding: 2
  },
  matrixCompact: {
    gridTemplateColumns: 'repeat(auto-fit, minmax(150px, 1fr))',
    gridAutoRows: 'minmax(104px, auto)',
    gap: 7
  },
  card: {
    minWidth: 0,
    display: 'grid',
    gridTemplateRows: 'auto minmax(0, 1fr) auto auto',
    gap: 8,
    padding: 12,
    overflow: 'hidden',
    border: '1px solid #cbd5e1',
    borderRadius: 8,
    background: '#ffffff'
  },
  cardTopline: { minWidth: 0, display: 'flex', justifyContent: 'space-between', gap: 8 },
  clipText: { minWidth: 0, overflow: 'hidden', textOverflow: 'ellipsis', whiteSpace: 'nowrap' },
  badge: {
    flex: '0 0 auto',
    padding: '2px 7px',
    borderRadius: 999,
    color: '#ffffff',
    background: '#28706b',
    fontSize: 12
  },
  mutedText: { margin: 0, color: '#64748b', lineHeight: 1.35, overflow: 'hidden' },
  chipRow: { display: 'flex', flexWrap: 'wrap', gap: 5 },
  chip: { padding: '2px 7px', borderRadius: 999, background: '#e8eef5', color: '#334155', fontSize: 12 },
  actionRow: { display: 'flex', flexWrap: 'wrap', gap: 7 },
  panel: { minWidth: 0, padding: 12, border: '1px solid #cbd5e1', borderRadius: 8, background: '#ffffff' },
  panelHeading: { display: 'flex', alignItems: 'baseline', justifyContent: 'space-between', gap: 8 },
  scrollBox: { minHeight: 0, overflow: 'auto', border: '1px solid #dbe4ee', borderRadius: 7 },
  logRow: {
    display: 'grid',
    gridTemplateColumns: '34px minmax(90px, 190px) minmax(0, 1fr)',
    gap: 9,
    alignItems: 'center',
    minHeight: 34,
    padding: '6px 8px',
    borderBottom: '1px solid #edf2f7'
  },
  field: { display: 'grid', gap: 5, color: '#475569' },
  input: {
    minWidth: 0,
    width: '100%',
    border: '1px solid #b8c6d6',
    borderRadius: 7,
    padding: '8px 10px',
    color: '#172033',
    background: '#ffffff',
    boxSizing: 'border-box'
  },
  result: {
    minHeight: 40,
    margin: 0,
    padding: 8,
    borderRadius: 7,
    color: '#234e52',
    background: '#e5f5f1'
  }
};

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

function makeRows(count) {
  const rows = [];
  for (let i = 0; i < count; i += 1) {
    const base = rowsSeed[i % rowsSeed.length];
    rows.push({ id: `row-${i + 1}`, title: `${base[0]} ${i + 1}`, body: base[1], weight: (i % 4) + 1 });
  }
  return rows;
}

function StatusPill({ label, value }) {
  return h('span', { style: S.pill, 'data-testid': `pill-${label.toLowerCase()}` },
    h('strong', { style: { color: '#475569', fontSize: 12 } }, label),
    h('span', { style: S.clipText }, value)
  );
}

function MatrixCard({ row, compact }) {
  const chips = ['min', 'auto', 'max', 'clip', 'grow'].slice(0, row.weight + 1);
  const cardStyle = compact ? { ...S.card, gap: 5, padding: 9 } : S.card;
  return h('article', { style: cardStyle, 'data-testid': row.id },
    h('div', { style: S.cardTopline },
      h('strong', { style: S.clipText }, row.title),
      h('span', { style: S.badge }, `w${row.weight}`)
    ),
    h('p', { style: S.mutedText }, row.body),
    h('div', { style: S.chipRow }, chips.map((chip) => h('span', { key: chip, style: S.chip }, chip))),
    h('div', { style: S.actionRow },
      h('button', { type: 'button', style: { ...S.button, width: 34, minHeight: 30, padding: 0 } }, 'A'),
      h('button', { type: 'button', style: { ...S.button, width: 34, minHeight: 30, padding: 0 } }, 'B'),
      h('button', { type: 'button', style: { ...S.button, width: 34, minHeight: 30, padding: 0 } }, 'C')
    )
  );
}

function LayoutMatrix({ rows, compact }) {
  return h('section', {
    id: 'layout-matrix',
    style: compact ? { ...S.matrix, ...S.matrixCompact } : S.matrix
  }, rows.map((row) => h(MatrixCard, { key: row.id, row, compact })));
}

function OverflowProbe({ rows }) {
  return h('section', { id: 'overflow-probe', style: { ...S.panel, minHeight: 0, display: 'grid', gridTemplateRows: 'auto minmax(0, 1fr)' } },
    h('div', { style: S.panelHeading },
      h('h2', { style: { margin: '0 0 8px', fontSize: 16 } }, 'Overflow Probe'),
      h('span', { style: { color: '#64748b', fontSize: 12 } }, `${rows.length} rows`)
    ),
    h('div', { style: S.scrollBox },
      rows.concat(rows.slice(0, 4)).map((row, index) => h('div', { style: S.logRow, key: `${row.id}-${index}` },
        h('span', null, String(index + 1).padStart(2, '0')),
        h('strong', { style: S.clipText }, row.title),
        h('em', { style: { ...S.clipText, color: '#64748b' } }, row.body)
      ))
    )
  );
}

function HostControls({ rows, setRows }) {
  const [draft, setDraft] = useState('');
  const [result, setResult] = useState('No host call yet');
  const [busy, setBusy] = useState(false);

  async function mutate(delta) {
    setBusy(true);
    try {
      const response = await hostApi.mutateMatrix({ delta, currentRows: rows.length, purpose: PURPOSE });
      const nextRows = Number(response.rows ?? rows.length);
      setRows(makeRows(nextRows));
      setResult(`${response.source ?? 'host'} ${response.label ?? 'mutation'} -> ${nextRows} rows`);
    } finally {
      setBusy(false);
    }
  }

  async function validate() {
    setBusy(true);
    try {
      const response = await hostApi.validatePayload({ text: draft, rowCount: rows.length, purpose: PURPOSE });
      setResult(response.message ?? JSON.stringify(response));
    } finally {
      setBusy(false);
    }
  }

  async function fail() {
    setBusy(true);
    try {
      const response = await hostApi.simulateFailure({ caseName: 'layout-host-error', purpose: PURPOSE });
      setResult(`${response.code ?? 'FAIL'}: ${response.message ?? 'failure returned'}`);
    } finally {
      setBusy(false);
    }
  }

  return h('section', { id: 'host-controls', style: { ...S.panel, display: 'grid', gap: 10 } },
    h('div', { style: S.panelHeading },
      h('h2', { style: { margin: '0 0 8px', fontSize: 16 } }, 'Rust Host Contract'),
      h('span', { style: { color: '#64748b', fontSize: 12 } }, busy ? 'calling' : 'idle')
    ),
    h('div', { style: S.actionRow },
      h('button', { id: 'add-row-button', type: 'button', disabled: busy, style: S.button, onClick: () => mutate(3) }, '+ rows'),
      h('button', { id: 'remove-row-button', type: 'button', disabled: busy, style: S.button, onClick: () => mutate(-2) }, '- rows'),
      h('button', { id: 'failure-button', type: 'button', disabled: busy, style: S.button, onClick: fail }, 'failure')
    ),
    h('label', { style: S.field },
      h('span', null, 'Payload'),
      h('input', {
        id: 'payload-input',
        value: draft,
        placeholder: 'send text through Rust validation',
        onInput: (event) => setDraft(event.currentTarget.value),
        onKeyDown: (event) => {
          if (event.key === 'Enter') validate();
        },
        style: S.input
      })
    ),
    h('button', { id: 'validate-button', type: 'button', disabled: busy, style: { ...S.button, justifySelf: 'start' }, onClick: validate }, 'Validate payload'),
    h('p', { id: 'host-result', style: S.result }, result)
  );
}

function App() {
  const [info, setInfo] = useState({ host: 'loading', runtime: 'loading', mode: 'loading', capabilities: {} });
  const [rows, setRows] = useState(makeRows(8));
  const [compact, setCompact] = useState(false);
  const generatedAt = useMemo(() => new Date().toLocaleString(), []);

  useEffect(() => {
    let active = true;
    hostApi.getMatrixInfo({ purpose: PURPOSE }).then((value) => {
      if (active) setInfo(value);
    });
    return () => {
      active = false;
    };
  }, []);

  return h('main', { id: 'app-shell', style: S.shell },
    h('header', { id: 'matrix-header', style: S.header },
      h('div', null,
        h('p', { style: S.eyebrow }, 'MBlink Layout + Host Matrix'),
        h('h1', { style: S.title }, 'Common Layout Combinations for Framework Bug Tests'),
        h('p', { style: S.summary }, `Generated ${generatedAt}. Provider: ${info.host} / ${info.mode}.`)
      ),
      h('button', { id: 'density-toggle', type: 'button', style: S.button, onClick: () => setCompact(!compact) }, compact ? 'Comfortable' : 'Compact')
    ),
    h('section', { id: 'status-strip', style: S.statusStrip },
      h(StatusPill, { label: 'Runtime', value: info.runtime }),
      h(StatusPill, { label: 'Rows', value: String(rows.length) }),
      h(StatusPill, { label: 'Backend', value: info.capabilities?.backend ? 'Rust' : 'Mock' }),
      h(StatusPill, { label: 'Density', value: compact ? 'Compact' : 'Normal' })
    ),
    h('section', { id: 'workbench', style: S.workbench },
      h('div', { style: S.primaryColumn },
        h(LayoutMatrix, { rows, compact }),
        h(OverflowProbe, { rows })
      ),
      h('aside', { id: 'side-rail', style: S.sideRail },
        h(HostControls, { rows, setRows }),
        h('section', { id: 'absolute-probe', style: { ...S.panel, position: 'relative', overflow: 'hidden' } },
          h('span', { style: { position: 'absolute', top: 8, right: 8, padding: '3px 7px', borderRadius: 999, color: '#fff', background: '#805ad5', fontSize: 12 } }, 'overlay'),
          h('h2', { style: { margin: '0 0 8px', fontSize: 16 } }, 'Overlay Probe'),
          h('p', { style: S.mutedText }, 'This panel keeps an absolute tag above normal content while the side rail changes height.')
        )
      )
    )
  );
}

installDocumentShell();
render(h(App), mountRoot());
