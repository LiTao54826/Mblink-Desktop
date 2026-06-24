import { h, render } from 'preact';
import { useMemo, useState } from 'preact/hooks';

const css = `
* { box-sizing: border-box; }
html, body, #combo-root { width: 100%; height: 100%; margin: 0; overflow: hidden; }
body {
  font-family: "Segoe UI", Arial, sans-serif;
  color: #1d2a2e;
  background: #e8eef0;
  -webkit-font-smoothing: antialiased;
}
button, input, select, textarea {
  font-family: inherit;
  font-size: 13px;
}
button {
  height: 34px;
  border: 1px solid #9fb1b4;
  border-radius: 6px;
  background: #f7fbfb;
  color: #213034;
  padding: 0 11px;
  line-height: 20px;
  cursor: pointer;
}
button:hover { background: #eef6f5; border-color: #628e90; }
button:active { background: #dbeceb; }
button:disabled { color: #91a0a3; background: #edf1f1; cursor: default; }
input, select, textarea {
  width: 100%;
  border: 1px solid #9fb1b4;
  border-radius: 6px;
  background: #ffffff;
  color: #203034;
  padding: 7px 9px;
  line-height: 20px;
}
input, select { height: 36px; }
input[type="checkbox"] {
  width: 16px;
  height: 16px;
  padding: 0;
  margin: 0;
}
textarea {
  min-height: 72px;
  resize: none;
}
.app-shell {
  width: 100%;
  height: 100%;
  display: flex;
  background: #eef3f3;
}
.sidebar {
  width: 214px;
  min-width: 214px;
  display: flex;
  flex-direction: column;
  border-right: 1px solid #c3ced0;
  background: #203034;
  color: #e9f2f2;
}
.brand {
  height: 68px;
  display: flex;
  align-items: center;
  gap: 10px;
  padding: 0 16px;
  border-bottom: 1px solid #344b50;
}
.brand-mark {
  width: 30px;
  height: 30px;
  border-radius: 8px;
  display: flex;
  align-items: center;
  justify-content: center;
  background: #42a39b;
  color: #ffffff;
  font-weight: 700;
}
.brand-title { font-size: 15px; font-weight: 700; }
.brand-subtitle { font-size: 11px; color: #aac4c5; margin-top: 2px; }
.nav-list {
  padding: 12px 10px;
  display: grid;
  gap: 5px;
}
.nav-button {
  width: 100%;
  justify-content: flex-start;
  text-align: left;
  display: flex;
  align-items: center;
  gap: 9px;
  border-color: transparent;
  color: #dce8e8;
  background: transparent;
}
.nav-button:hover { background: #2b4448; border-color: #406166; }
.nav-button.active { background: #356d70; border-color: #4aa19b; color: #ffffff; }
.nav-symbol { width: 20px; text-align: center; font-weight: 700; }
.sidebar-footer {
  margin-top: auto;
  padding: 12px;
  border-top: 1px solid #344b50;
  display: grid;
  gap: 8px;
  font-size: 12px;
  color: #b5cacc;
}
.main {
  flex: 1;
  min-width: 0;
  height: 100%;
  display: flex;
  flex-direction: column;
  overflow: hidden;
}
.topbar {
  height: 68px;
  min-height: 68px;
  display: flex;
  align-items: center;
  gap: 12px;
  padding: 0 18px;
  border-bottom: 1px solid #c5d0d1;
  background: #fbfdfd;
}
.title-block { min-width: 210px; }
.page-title { margin: 0; font-size: 20px; line-height: 24px; }
.page-caption { margin: 3px 0 0; color: #65787c; font-size: 12px; }
.search-wrap { flex: 1; min-width: 160px; }
.toolbar {
  display: flex;
  align-items: center;
  gap: 8px;
}
.btn-primary { background: #237b73; border-color: #237b73; color: #ffffff; }
.btn-primary:hover { background: #1d6c65; border-color: #1d6c65; }
.btn-danger { background: #fff4f2; border-color: #dd8c80; color: #9c3e32; }
.btn-icon { width: 34px; padding: 0; text-align: center; font-weight: 700; }
.content {
  flex: 1;
  min-height: 0;
  overflow: hidden;
  display: grid;
  grid-template-columns: minmax(0, 1fr) 318px;
  gap: 14px;
  padding: 14px;
}
.workspace {
  min-width: 0;
  min-height: 0;
  overflow: auto;
  display: grid;
  gap: 14px;
  align-content: start;
  padding-right: 2px;
}
.rail {
  min-height: 0;
  overflow: auto;
  display: grid;
  gap: 14px;
  align-content: start;
}
.panel {
  background: #ffffff;
  border: 1px solid #c3ced0;
  border-radius: 8px;
  overflow: hidden;
}
.panel-head {
  min-height: 48px;
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
  padding: 10px 12px;
  border-bottom: 1px solid #d8e0e1;
}
.panel-title { margin: 0; font-size: 15px; line-height: 20px; }
.panel-body { padding: 12px; }
.metrics {
  display: grid;
  grid-template-columns: repeat(4, minmax(132px, 1fr));
  gap: 10px;
}
.metric {
  min-height: 84px;
  padding: 12px;
  border: 1px solid #c9d5d6;
  border-radius: 8px;
  background: #fbfdfd;
}
.metric-label { color: #5c7074; font-size: 12px; }
.metric-value { margin-top: 8px; font-size: 24px; line-height: 28px; font-weight: 700; }
.metric-note { margin-top: 5px; color: #77888b; font-size: 12px; }
.metric.good { border-top: 3px solid #2c9076; }
.metric.warn { border-top: 3px solid #d59a20; }
.metric.hot { border-top: 3px solid #c85849; }
.metric.cool { border-top: 3px solid #6272a4; }
.state-grid {
  display: grid;
  grid-template-columns: minmax(0, 1fr) 280px;
  gap: 14px;
}
.tabbar, .segmented {
  display: flex;
  gap: 4px;
  align-items: center;
}
.tabbar button, .segmented button {
  height: 30px;
  min-width: 78px;
  background: #ffffff;
}
.tabbar button.active, .segmented button.active {
  color: #ffffff;
  border-color: #237b73;
  background: #237b73;
}
.status-controls {
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
}
.state-box {
  min-height: 128px;
  display: flex;
  align-items: center;
  justify-content: center;
  text-align: center;
  border: 1px dashed #b3c1c3;
  border-radius: 8px;
  background: #f8fbfb;
  color: #586d71;
  padding: 16px;
}
.loader-dot {
  width: 10px;
  height: 10px;
  margin: 0 3px;
  border-radius: 50%;
  display: inline-block;
  background: #237b73;
}
.form-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 10px;
}
.form-field {
  display: grid;
  gap: 5px;
}
.form-field.full { grid-column: 1 / -1; }
.field-label { font-size: 12px; color: #5d7074; }
.check-row {
  display: flex;
  align-items: center;
  gap: 9px;
  min-height: 34px;
  color: #394a4e;
  font-size: 13px;
}
.check-row input {
  width: 16px;
  height: 16px;
  padding: 0;
}
.form-actions {
  display: flex;
  gap: 8px;
  justify-content: flex-end;
  margin-top: 10px;
}
.result-line {
  min-height: 22px;
  margin-top: 9px;
  font-size: 13px;
  color: #237b73;
}
.table-wrap {
  overflow: auto;
  max-height: 308px;
  border: 1px solid #d8e0e1;
  border-radius: 6px;
}
table {
  width: 100%;
  min-width: 720px;
  border-collapse: collapse;
  font-size: 13px;
  background: #ffffff;
}
th, td {
  border-bottom: 1px solid #e1e7e8;
  padding: 8px;
  text-align: left;
  white-space: nowrap;
}
th:first-child, td:first-child {
  width: 42px;
  text-align: center;
}
th {
  color: #5c7074;
  font-weight: 600;
  background: #f5f9f9;
}
tr.selected td { background: #edf8f6; }
.dense th, .dense td { padding-top: 5px; padding-bottom: 5px; }
.badge {
  display: inline-flex;
  align-items: center;
  min-height: 22px;
  padding: 2px 7px;
  border-radius: 6px;
  border: 1px solid #c2ced0;
  background: #f7fafb;
  color: #36494d;
  font-size: 12px;
}
.badge.green { background: #edf8f4; border-color: #9dcdbd; color: #18654f; }
.badge.amber { background: #fff7e7; border-color: #e0bd68; color: #8a5e00; }
.badge.red { background: #fff1ef; border-color: #e2a099; color: #963c31; }
.queue-list {
  display: grid;
  gap: 8px;
}
.queue-item {
  border: 1px solid #d2dcdd;
  border-radius: 8px;
  padding: 10px;
  background: #fbfdfd;
}
.queue-line {
  display: flex;
  justify-content: space-between;
  gap: 8px;
  align-items: center;
}
.queue-name { font-weight: 600; }
.queue-meta { color: #6a7b7f; font-size: 12px; margin-top: 5px; }
.activity-feed {
  max-height: 210px;
  overflow: auto;
  display: grid;
  gap: 8px;
}
.activity-item {
  display: grid;
  grid-template-columns: 64px minmax(0, 1fr);
  gap: 9px;
  padding: 8px;
  border: 1px solid #d7e0e1;
  border-radius: 8px;
  background: #fbfdfd;
}
.activity-time { color: #6d7f83; font-size: 12px; }
.activity-text { min-width: 0; line-height: 18px; }
.toast-stack {
  position: absolute;
  right: 18px;
  bottom: 18px;
  display: grid;
  gap: 8px;
  width: 284px;
  z-index: 20;
}
.toast {
  min-height: 44px;
  border: 1px solid #8ec7bd;
  border-radius: 8px;
  background: #f3fbf9;
  color: #1f4d49;
  padding: 10px 12px;
  box-shadow: 0 5px 14px rgba(21, 36, 40, 0.15);
}
.modal-backdrop {
  position: absolute;
  inset: 0;
  display: flex;
  align-items: center;
  justify-content: center;
  background: rgba(20, 32, 35, 0.28);
  z-index: 15;
}
.modal {
  width: 430px;
  max-width: calc(100% - 32px);
  border: 1px solid #98abad;
  border-radius: 8px;
  background: #ffffff;
  box-shadow: 0 14px 36px rgba(22, 39, 43, 0.25);
}
.modal-head {
  height: 48px;
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 0 12px;
  border-bottom: 1px solid #d8e0e1;
}
.modal-body { padding: 12px; display: grid; gap: 10px; }
.modal-actions {
  display: flex;
  justify-content: flex-end;
  gap: 8px;
  padding: 0 12px 12px;
}
@media (max-width: 960px) {
  .content { grid-template-columns: 1fr; overflow: auto; }
  .workspace, .rail { overflow: visible; }
  .metrics { grid-template-columns: repeat(2, minmax(132px, 1fr)); }
  .state-grid { grid-template-columns: 1fr; }
  .sidebar { width: 184px; min-width: 184px; }
}
`;

const navItems = [
  { id: 'overview', symbol: 'O', label: 'Overview' },
  { id: 'forms', symbol: 'F', label: 'Forms' },
  { id: 'tables', symbol: 'T', label: 'Tables' },
  { id: 'states', symbol: 'S', label: 'States' }
];

const queueRows = [
  { id: 'api', name: 'API Gateway', owner: 'Platform', priority: 'High', status: 'Active', age: '7m' },
  { id: 'billing', name: 'Billing Sync', owner: 'Finance', priority: 'Medium', status: 'Review', age: '22m' },
  { id: 'search', name: 'Search Index', owner: 'Infra', priority: 'Low', status: 'Done', age: '1h' },
  { id: 'reports', name: 'Report Export', owner: 'Data', priority: 'High', status: 'Active', age: '2h' },
  { id: 'desktop', name: 'Desktop Shell', owner: 'Client', priority: 'Medium', status: 'Review', age: '3h' },
  { id: 'auth', name: 'Auth Policy', owner: 'Security', priority: 'High', status: 'Active', age: '4h' }
];

const audits = [
  ['09:10', 'Queue accepted a new rollout candidate for API Gateway.'],
  ['09:17', 'Billing Sync moved to review after dry-run validation.'],
  ['09:25', 'Search Index finished with no blocking warnings.'],
  ['09:38', 'Desktop Shell received a manual priority update.'],
  ['09:46', 'Auth Policy requested a second reviewer.'],
  ['10:03', 'Report Export failed a data freshness check.'],
  ['10:16', 'Queue worker resumed after backoff.'],
  ['10:29', 'API Gateway canary target changed to group B.'],
  ['10:44', 'Desktop Shell snapshot was attached to the review.'],
  ['11:02', 'Auth Policy merged the reviewer note.']
];

let toastId = 0;

function installDocumentShell() {
  let style = document.getElementById('ui-combinations-style');
  if (!style) {
    style = document.createElement('style');
    style.id = 'ui-combinations-style';
    style.textContent = css;
    document.head.appendChild(style);
  }
  Object.assign(document.documentElement.style, { width: '100%', height: '100%', margin: '0', overflow: 'hidden' });
  Object.assign(document.body.style, { width: '100%', height: '100%', margin: '0', overflow: 'hidden' });
}

function mountRoot() {
  let root = document.getElementById('combo-root');
  if (!root) {
    root = document.createElement('div');
    root.id = 'combo-root';
    document.body.appendChild(root);
  }
  return root;
}

function cx() {
  return Array.from(arguments).filter(Boolean).join(' ');
}

function badgeClass(value) {
  if (value === 'Active' || value === 'High') return 'badge green';
  if (value === 'Review' || value === 'Medium') return 'badge amber';
  if (value === 'Blocked') return 'badge red';
  return 'badge';
}

function Sidebar({ activeNav, onNav }) {
  return h('aside', { className: 'sidebar', id: 'sidebar-nav' },
    h('div', { className: 'brand' },
      h('div', { className: 'brand-mark' }, 'MB'),
      h('div', null,
        h('div', { className: 'brand-title' }, 'UI Bench'),
        h('div', { className: 'brand-subtitle' }, 'Common desktop states')
      )
    ),
    h('nav', { className: 'nav-list' },
      navItems.map((item) => h('button', {
        key: item.id,
        id: 'nav-' + item.id,
        className: cx('nav-button', activeNav === item.id && 'active'),
        onClick: () => onNav(item.id)
      },
        h('span', { className: 'nav-symbol' }, item.symbol),
        h('span', null, item.label)
      ))
    ),
    h('div', { className: 'sidebar-footer' },
      h('div', null, 'Runtime: tool'),
      h('div', null, 'Selectors: stable'),
      h('button', { id: 'sidebar-help', className: 'nav-button', onClick: () => onNav('states') }, 'Open states')
    )
  );
}

function MetricGrid() {
  const metrics = [
    ['good', 'Open tasks', '24', '+4 today'],
    ['warn', 'Needs review', '8', '3 owners'],
    ['hot', 'Blocked', '2', 'manual action'],
    ['cool', 'Idle workers', '11', 'healthy']
  ];
  return h('section', { className: 'metrics', id: 'metric-grid' },
    metrics.map((item) => h('article', { key: item[1], className: 'metric ' + item[0] },
      h('div', { className: 'metric-label' }, item[1]),
      h('div', { className: 'metric-value' }, item[2]),
      h('div', { className: 'metric-note' }, item[3])
    ))
  );
}

function StatePreview({ mode }) {
  if (mode === 'loading') {
    return h('div', { className: 'state-box', id: 'state-preview' },
      h('div', null,
        h('span', { className: 'loader-dot' }),
        h('span', { className: 'loader-dot' }),
        h('span', { className: 'loader-dot' }),
        h('div', { style: { marginTop: 10 } }, 'Loading queue snapshot')
      )
    );
  }
  if (mode === 'empty') {
    return h('div', { className: 'state-box', id: 'state-preview' }, 'No matching queue items');
  }
  if (mode === 'error') {
    return h('div', { className: 'state-box', id: 'state-preview' },
      h('div', null,
        h('strong', { style: { color: '#963c31' } }, 'Snapshot failed'),
        h('div', { style: { marginTop: 6 } }, 'Retry or inspect runtime logs')
      )
    );
  }
  return h('div', { className: 'state-box', id: 'state-preview' }, 'Normal content state');
}

function RequestForm({ addToast }) {
  const [requestName, setRequestName] = useState('');
  const [requestType, setRequestType] = useState('feature');
  const [priority, setPriority] = useState('medium');
  const [notes, setNotes] = useState('');
  const [ack, setAck] = useState(true);
  const [result, setResult] = useState('Draft is ready.');

  function resetForm() {
    setRequestName('');
    setRequestType('feature');
    setPriority('medium');
    setNotes('');
    setAck(true);
    setResult('Draft was reset.');
  }

  function createRequest() {
    const input = document.getElementById('request-name');
    const name = (requestName || (input && input.value) || '').trim();
    if (!name) {
      setResult('Name is required.');
      addToast('Request name is required.');
      return;
    }
    const text = name + ' saved as ' + requestType + ' / ' + priority + '.';
    setResult(text);
    addToast(text);
  }

  return h('section', { className: 'panel', id: 'request-form-panel' },
    h('div', { className: 'panel-head' },
      h('h2', { className: 'panel-title' }, 'Request editor'),
      h('span', { className: 'badge' }, ack ? 'Ready' : 'Waiting')
    ),
    h('div', { className: 'panel-body' },
      h('div', { className: 'form-grid' },
        h('label', { className: 'form-field' },
          h('span', { className: 'field-label' }, 'Name'),
          h('input', {
            id: 'request-name',
            value: requestName,
            placeholder: 'Service or workflow',
            onInput: (event) => setRequestName(event.currentTarget.value)
          })
        ),
        h('label', { className: 'form-field' },
          h('span', { className: 'field-label' }, 'Type'),
          h('select', {
            id: 'request-type',
            value: requestType,
            onChange: (event) => setRequestType(event.currentTarget.value)
          },
            h('option', { value: 'feature' }, 'Feature'),
            h('option', { value: 'incident' }, 'Incident'),
            h('option', { value: 'maintenance' }, 'Maintenance')
          )
        ),
        h('label', { className: 'form-field' },
          h('span', { className: 'field-label' }, 'Priority'),
          h('select', {
            id: 'request-priority',
            value: priority,
            onChange: (event) => setPriority(event.currentTarget.value)
          },
            h('option', { value: 'low' }, 'Low'),
            h('option', { value: 'medium' }, 'Medium'),
            h('option', { value: 'high' }, 'High')
          )
        ),
        h('label', { className: 'form-field' },
          h('span', { className: 'field-label' }, 'Owner'),
          h('input', { id: 'request-owner', value: 'Platform', readOnly: true })
        ),
        h('label', { className: 'form-field full' },
          h('span', { className: 'field-label' }, 'Notes'),
          h('textarea', {
            id: 'request-notes',
            value: notes,
            placeholder: 'Add context for the reviewer',
            onInput: (event) => setNotes(event.currentTarget.value)
          })
        )
      ),
      h('label', { className: 'check-row' },
        h('input', {
          id: 'request-ack',
          type: 'checkbox',
          checked: ack,
          onChange: (event) => setAck(event.currentTarget.checked)
        }),
        h('span', null, 'Ready for review')
      ),
      h('div', { className: 'form-actions' },
        h('button', { id: 'request-reset', onClick: resetForm }, 'Reset'),
        h('button', { id: 'request-create', className: 'btn-primary', disabled: !ack, onClick: createRequest }, 'Create')
      ),
      h('div', { className: 'result-line', id: 'form-result' }, result)
    )
  );
}

function QueueTable({ rows, activeTab, density, selected, onToggle }) {
  const filtered = rows.filter((row) => {
    if (activeTab === 'active') return row.status === 'Active';
    if (activeTab === 'review') return row.status === 'Review';
    if (activeTab === 'done') return row.status === 'Done';
    return true;
  });

  return h('div', { className: 'table-wrap' },
    h('table', { id: 'queue-table', className: density === 'compact' ? 'dense' : '' },
      h('thead', null,
        h('tr', null,
          h('th', null, ''),
          h('th', null, 'Name'),
          h('th', null, 'Owner'),
          h('th', null, 'Priority'),
          h('th', null, 'Status'),
          h('th', null, 'Age')
        )
      ),
      h('tbody', null,
        filtered.map((row) => h('tr', {
          key: row.id,
          id: 'queue-row-' + row.id,
          className: selected[row.id] ? 'selected' : ''
        },
          h('td', null, h('input', {
            id: 'select-' + row.id,
            type: 'checkbox',
            checked: !!selected[row.id],
            onChange: () => onToggle(row.id)
          })),
          h('td', null, row.name),
          h('td', null, row.owner),
          h('td', null, h('span', { className: badgeClass(row.priority) }, row.priority)),
          h('td', null, h('span', { className: badgeClass(row.status) }, row.status)),
          h('td', null, row.age)
        ))
      )
    )
  );
}

function QueuePanel({ rows, activeTab, setActiveTab, density, setDensity }) {
  const [selected, setSelected] = useState({});
  const selectedCount = Object.keys(selected).filter((key) => selected[key]).length;

  function toggleRow(id) {
    setSelected((current) => Object.assign({}, current, { [id]: !current[id] }));
  }

  return h('section', { className: 'panel', id: 'queue-panel' },
    h('div', { className: 'panel-head' },
      h('h2', { className: 'panel-title' }, 'Queue table'),
      h('div', { className: 'toolbar' },
        h('div', { className: 'tabbar', id: 'queue-tabs' },
          ['all', 'active', 'review', 'done'].map((tab) => h('button', {
            key: tab,
            id: 'tab-' + tab,
            className: activeTab === tab ? 'active' : '',
            onClick: () => setActiveTab(tab)
          }, tab[0].toUpperCase() + tab.slice(1)))
        )
      )
    ),
    h('div', { className: 'panel-body' },
      h('div', { className: 'toolbar', style: { justifyContent: 'space-between', marginBottom: 10 } },
        h('span', { id: 'selected-count', className: 'badge' }, selectedCount + ' selected'),
        h('div', { className: 'segmented', id: 'density-control' },
          h('button', {
            id: 'density-comfortable',
            className: density === 'comfortable' ? 'active' : '',
            onClick: () => setDensity('comfortable')
          }, 'Comfort'),
          h('button', {
            id: 'density-compact',
            className: density === 'compact' ? 'active' : '',
            onClick: () => setDensity('compact')
          }, 'Compact')
        )
      ),
      h(QueueTable, { rows, activeTab, density, selected, onToggle: toggleRow })
    )
  );
}

function QueueCards({ rows }) {
  return h('section', { className: 'panel', id: 'queue-cards-panel' },
    h('div', { className: 'panel-head' },
      h('h2', { className: 'panel-title' }, 'Queue cards'),
      h('span', { className: 'badge' }, rows.length + ' items')
    ),
    h('div', { className: 'panel-body queue-list' },
      rows.slice(0, 4).map((row) => h('article', { key: row.id, className: 'queue-item', id: 'card-' + row.id },
        h('div', { className: 'queue-line' },
          h('span', { className: 'queue-name' }, row.name),
          h('span', { className: badgeClass(row.status) }, row.status)
        ),
        h('div', { className: 'queue-meta' }, row.owner + ' / ' + row.priority + ' / ' + row.age)
      ))
    )
  );
}

function StatePanel({ mode, setMode }) {
  return h('section', { className: 'panel', id: 'state-panel' },
    h('div', { className: 'panel-head' },
      h('h2', { className: 'panel-title' }, 'State surface'),
      h('span', { className: 'badge' }, mode)
    ),
    h('div', { className: 'panel-body' },
      h('div', { className: 'status-controls', id: 'status-controls' },
        ['normal', 'loading', 'empty', 'error'].map((item) => h('button', {
          key: item,
          id: 'status-' + item,
          className: mode === item ? 'btn-primary' : '',
          onClick: () => setMode(item)
        }, item[0].toUpperCase() + item.slice(1)))
      ),
      h('div', { style: { height: 10 } }),
      h(StatePreview, { mode })
    )
  );
}

function ActivityFeed() {
  return h('section', { className: 'panel', id: 'activity-panel' },
    h('div', { className: 'panel-head' },
      h('h2', { className: 'panel-title' }, 'Activity'),
      h('button', { id: 'clear-activity', disabled: true }, 'Archive')
    ),
    h('div', { className: 'panel-body activity-feed', id: 'activity-feed' },
      audits.map((item, index) => h('div', { key: index, className: 'activity-item', id: 'activity-' + index },
        h('div', { className: 'activity-time' }, item[0]),
        h('div', { className: 'activity-text' }, item[1])
      ))
    )
  );
}

function SettingsModal({ onClose, addToast }) {
  function apply() {
    addToast('Settings applied.');
    onClose();
  }
  return h('div', { className: 'modal-backdrop', id: 'settings-backdrop' },
    h('section', { className: 'modal', id: 'settings-dialog' },
      h('div', { className: 'modal-head' },
        h('strong', null, 'Display settings'),
        h('button', { id: 'modal-close', className: 'btn-icon', onClick: onClose }, 'x')
      ),
      h('div', { className: 'modal-body' },
        h('label', { className: 'check-row' },
          h('input', { id: 'modal-auto-refresh', type: 'checkbox', defaultChecked: true }),
          h('span', null, 'Auto refresh snapshots')
        ),
        h('label', { className: 'form-field' },
          h('span', { className: 'field-label' }, 'Refresh window'),
          h('select', { id: 'modal-refresh-window', defaultValue: 'fast' },
            h('option', { value: 'fast' }, 'Fast'),
            h('option', { value: 'balanced' }, 'Balanced'),
            h('option', { value: 'manual' }, 'Manual')
          )
        )
      ),
      h('div', { className: 'modal-actions' },
        h('button', { id: 'modal-cancel', onClick: onClose }, 'Cancel'),
        h('button', { id: 'modal-apply', className: 'btn-primary', onClick: apply }, 'Apply')
      )
    )
  );
}

function ToastStack({ toasts }) {
  return h('div', { className: 'toast-stack', id: 'toast-stack' },
    toasts.map((toast) => h('div', { key: toast.id, className: 'toast', id: 'toast-' + toast.id }, toast.message))
  );
}

function App() {
  const [activeNav, setActiveNav] = useState('overview');
  const [activeTab, setActiveTab] = useState('all');
  const [density, setDensity] = useState('comfortable');
  const [search, setSearch] = useState('');
  const [mode, setMode] = useState('normal');
  const [modalOpen, setModalOpen] = useState(false);
  const [toasts, setToasts] = useState([]);

  const visibleRows = useMemo(() => {
    const needle = search.trim().toLowerCase();
    if (!needle) return queueRows;
    return queueRows.filter((row) => {
      return row.name.toLowerCase().indexOf(needle) >= 0 ||
        row.owner.toLowerCase().indexOf(needle) >= 0 ||
        row.status.toLowerCase().indexOf(needle) >= 0;
    });
  }, [search]);

  function addToast(message) {
    const id = ++toastId;
    setToasts((current) => current.concat([{ id, message }]).slice(-3));
    setTimeout(() => {
      setToasts((current) => current.filter((toast) => toast.id !== id));
    }, 2500);
  }

  function runAction() {
    addToast('Action queued for ' + visibleRows.length + ' items.');
  }

  return h('div', { className: 'app-shell' },
    h(Sidebar, { activeNav, onNav: setActiveNav }),
    h('main', { className: 'main' },
      h('header', { className: 'topbar' },
        h('div', { className: 'title-block' },
          h('h1', { className: 'page-title', id: 'page-title' }, 'Operations console'),
          h('p', { className: 'page-caption', id: 'page-caption' }, 'Section: ' + activeNav)
        ),
        h('div', { className: 'search-wrap' },
          h('input', {
            id: 'global-search',
            value: search,
            placeholder: 'Search queues, owners, states',
            onInput: (event) => setSearch(event.currentTarget.value)
          })
        ),
        h('div', { className: 'toolbar' },
          h('button', { id: 'open-settings-modal', onClick: () => setModalOpen(true) }, 'Settings'),
          h('button', { id: 'run-action', className: 'btn-primary', onClick: runAction }, 'Run'),
          h('button', { id: 'danger-action', className: 'btn-danger', onClick: () => addToast('Delete was blocked in demo mode.') }, 'Delete')
        )
      ),
      h('div', { className: 'content' },
        h('div', { className: 'workspace', id: 'workspace-scroll' },
          h(MetricGrid),
          h(QueuePanel, { rows: visibleRows, activeTab, setActiveTab, density, setDensity }),
          h('div', { className: 'state-grid' },
            h(StatePanel, { mode, setMode })
          ),
          h('div', { className: 'state-grid' },
            h(RequestForm, { addToast }),
            h(QueueCards, { rows: visibleRows })
          ),
        ),
        h('aside', { className: 'rail' },
          h(ActivityFeed)
        )
      )
    ),
    modalOpen ? h(SettingsModal, { onClose: () => setModalOpen(false), addToast }) : null,
    h(ToastStack, { toasts })
  );
}

installDocumentShell();
render(h(App), mountRoot());
console.log('[ui-combinations-showcase] loaded');
