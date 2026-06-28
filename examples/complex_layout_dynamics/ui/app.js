import { h, render } from 'preact';
import { useMemo, useState } from 'preact/hooks';

const css = `
* { box-sizing: border-box; }
html, body, #root {
  width: 100%;
  height: 100%;
  margin: 0;
  overflow: hidden;
}
body {
  font-family: "Segoe UI", Arial, sans-serif;
  color: #18242f;
  background: #e8edf1;
}
button, input, select, textarea {
  font: inherit;
}
button {
  min-height: 34px;
  border: 1px solid #a8b6c2;
  border-radius: 6px;
  background: #f7fafc;
  color: #18242f;
  padding: 6px 11px;
}
button.active,
button.primary {
  background: #14796f;
  border-color: #14796f;
  color: #ffffff;
}
button.danger {
  background: #fff4f2;
  border-color: #cf8c81;
  color: #96392d;
}
input, select, textarea {
  width: 100%;
  min-width: 0;
  border: 1px solid #9aaab7;
  border-radius: 6px;
  background: #ffffff;
  color: #18242f;
  padding: 7px 9px;
}
input, select { height: 36px; }
textarea { min-height: 88px; resize: none; }
input[type="checkbox"] {
  width: 16px;
  height: 16px;
  min-width: 16px;
  padding: 0;
  margin: 0;
}
.app-shell {
  width: 100%;
  height: 100%;
  display: grid;
  grid-template-columns: 226px minmax(0, 1fr);
  grid-template-rows: 64px minmax(0, 1fr);
  overflow: hidden;
}
.sidebar {
  grid-row: 1 / 3;
  min-height: 0;
  display: grid;
  grid-template-rows: auto minmax(0, 1fr) auto;
  background: #20313b;
  color: #eef6f8;
  border-right: 1px solid #3b5360;
}
.brand {
  min-height: 68px;
  display: flex;
  align-items: center;
  gap: 10px;
  padding: 0 14px;
  border-bottom: 1px solid #3b5360;
}
.brand-mark {
  width: 34px;
  height: 34px;
  border-radius: 8px;
  display: grid;
  place-items: center;
  background: #39a28f;
  color: #ffffff;
  font-weight: 800;
}
.brand-title { font-weight: 800; }
.brand-note { margin-top: 3px; color: #b8cbd0; font-size: 12px; }
.nav {
  min-height: 0;
  overflow: auto;
  padding: 10px;
  display: grid;
  gap: 6px;
  align-content: start;
}
.nav button {
  width: 100%;
  min-height: 38px;
  display: grid;
  grid-template-columns: 26px minmax(0, 1fr);
  align-items: center;
  gap: 8px;
  text-align: left;
  background: transparent;
  border-color: transparent;
  color: #e5f0f2;
}
.nav button:hover { background: #2a4651; border-color: #426674; }
.nav button.active { background: #2f716d; border-color: #49a99a; }
.sidebar-footer {
  padding: 12px;
  border-top: 1px solid #3b5360;
  color: #b8cbd0;
  font-size: 12px;
  display: grid;
  gap: 7px;
}
.topbar {
  min-width: 0;
  display: grid;
  grid-template-columns: minmax(220px, 1fr) 310px auto;
  align-items: center;
  gap: 12px;
  padding: 12px 16px;
  background: #fbfcfd;
  border-bottom: 1px solid #c7d1d8;
}
.title h1 {
  margin: 0;
  font-size: 18px;
  line-height: 22px;
  letter-spacing: 0;
}
.title p {
  margin: 4px 0 0;
  color: #617380;
  font-size: 12px;
}
.toolbar {
  display: flex;
  align-items: center;
  justify-content: flex-end;
  gap: 8px;
}
.content {
  min-width: 0;
  min-height: 0;
  overflow: hidden;
  display: grid;
  grid-template-columns: minmax(0, 1fr) 306px;
  gap: 14px;
  padding: 14px;
}
.main-stage {
  min-width: 0;
  min-height: 0;
  overflow: auto;
  display: grid;
  gap: 14px;
  align-content: start;
  padding-right: 2px;
}
.right-rail {
  min-height: 0;
  overflow: auto;
  display: grid;
  gap: 14px;
  align-content: start;
}
.panel {
  background: #ffffff;
  border: 1px solid #c6d2da;
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
  border-bottom: 1px solid #d8e1e7;
}
.panel-title {
  margin: 0;
  font-size: 15px;
  line-height: 20px;
}
.panel-body { padding: 12px; }
.metrics {
  display: grid;
  grid-template-columns: repeat(4, minmax(126px, 1fr));
  gap: 10px;
}
.metric {
  min-height: 82px;
  border: 1px solid #c6d2da;
  border-radius: 8px;
  padding: 11px;
  background: #fbfdfe;
}
.metric-label { color: #60737f; font-size: 12px; }
.metric-value { margin-top: 7px; font-size: 25px; line-height: 30px; font-weight: 800; }
.metric-note { margin-top: 4px; color: #74838c; font-size: 12px; }
.metric.good { border-top: 3px solid #188166; }
.metric.warn { border-top: 3px solid #bf7d1d; }
.metric.hot { border-top: 3px solid #bd483a; }
.metric.cool { border-top: 3px solid #3e6e9d; }
.table-wrap {
  min-height: 318px;
  max-height: 342px;
  overflow: auto;
  border: 1px solid #d7e0e6;
  border-radius: 6px;
}
table {
  width: 100%;
  min-width: 920px;
  border-collapse: collapse;
  table-layout: fixed;
  background: #ffffff;
}
th, td {
  border-bottom: 1px solid #e3e9ee;
  border-right: 1px solid #edf2f5;
  padding: 8px;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
  text-align: left;
}
th:last-child, td:last-child { border-right: 0; }
th {
  color: #5c6e7a;
  background: #f5f8fa;
  font-size: 12px;
  font-weight: 800;
}
td { font-size: 13px; }
.select-col { width: 42px; text-align: center; }
.id-col { width: 92px; color: #14796f; font-weight: 800; }
.owner-col { width: 118px; }
.state-col { width: 112px; }
.risk-col { width: 94px; }
.number-col { width: 110px; text-align: right; }
.notes-col { width: 170px; }
.dense th,
.dense td { padding-top: 5px; padding-bottom: 5px; }
tr.selected td { background: #edf8f5; }
.badge {
  display: inline-flex;
  min-height: 22px;
  align-items: center;
  max-width: 100%;
  border: 1px solid #c7d2da;
  border-radius: 6px;
  padding: 2px 7px;
  color: #32444f;
  background: #f6fafc;
  font-size: 12px;
  font-weight: 700;
}
.badge.good { color: #17664e; background: #edf8f2; border-color: #a6d3bf; }
.badge.warn { color: #885b11; background: #fff7e8; border-color: #e1bd6a; }
.badge.hot { color: #8b3b31; background: #fff1ef; border-color: #e5a098; }
.tabs,
.segmented {
  display: flex;
  align-items: center;
  gap: 5px;
  flex-wrap: wrap;
}
.tabs button,
.segmented button { min-width: 78px; }
.split-grid {
  display: grid;
  grid-template-columns: minmax(0, 1fr) 300px;
  gap: 14px;
}
.board {
  min-width: 0;
  display: grid;
  grid-template-columns: repeat(3, minmax(190px, 1fr));
  gap: 10px;
}
.board-column {
  min-height: 330px;
  border: 1px solid #cdd8df;
  border-radius: 8px;
  background: #f8fbfc;
  display: grid;
  grid-template-rows: auto minmax(0, 1fr);
}
.column-head {
  min-height: 42px;
  padding: 10px;
  display: flex;
  justify-content: space-between;
  gap: 10px;
  border-bottom: 1px solid #dce5ea;
  font-weight: 800;
}
.card-list {
  min-height: 0;
  overflow: auto;
  padding: 10px;
  display: grid;
  gap: 9px;
  align-content: start;
}
.task-card {
  border: 1px solid #cbd7de;
  border-radius: 8px;
  background: #ffffff;
  padding: 10px;
}
.task-card strong {
  display: block;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}
.task-meta {
  margin-top: 6px;
  display: flex;
  justify-content: space-between;
  gap: 8px;
  color: #647680;
  font-size: 12px;
}
.form-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 10px;
}
.field { display: grid; gap: 5px; }
.field.full { grid-column: 1 / -1; }
.field span { color: #60737f; font-size: 12px; font-weight: 700; }
.check-row {
  min-height: 36px;
  display: flex;
  align-items: center;
  gap: 9px;
  color: #43555f;
  font-size: 13px;
}
.result-line {
  min-height: 24px;
  margin-top: 10px;
  color: #14796f;
  font-size: 13px;
}
.preview-stack {
  display: grid;
  gap: 10px;
}
.preview-box {
  min-height: 120px;
  border: 1px dashed #adbdc7;
  border-radius: 8px;
  background: #f8fbfc;
  padding: 14px;
  display: grid;
  place-items: center;
  text-align: center;
  color: #60717c;
}
.drawer {
  min-height: 260px;
  border: 1px solid #cdd8df;
  border-radius: 8px;
  background: #fbfdfe;
  padding: 12px;
  display: grid;
  gap: 10px;
  align-content: start;
}
.analytics-grid {
  display: grid;
  grid-template-columns: 1.4fr 1fr;
  gap: 14px;
}
.chart {
  min-height: 292px;
  display: flex;
  align-items: end;
  gap: 9px;
  padding: 14px;
  border: 1px solid #cdd8df;
  border-radius: 8px;
  background: #f8fbfc;
}
.bar {
  flex: 1;
  min-width: 24px;
  border-radius: 5px 5px 0 0;
  background: #2f8f83;
}
.bar.warn { background: #bf7d1d; }
.bar.hot { background: #bd483a; }
.log-list {
  max-height: 290px;
  overflow: auto;
  display: grid;
  gap: 8px;
}
.log-item {
  display: grid;
  grid-template-columns: 62px minmax(0, 1fr);
  gap: 9px;
  padding: 8px;
  border: 1px solid #d6e0e6;
  border-radius: 8px;
  background: #fbfdfe;
  font-size: 13px;
}
.log-time { color: #697b86; font-size: 12px; }
.toast-stack {
  position: absolute;
  right: 18px;
  bottom: 18px;
  z-index: 30;
  width: 300px;
  display: grid;
  gap: 8px;
}
.toast {
  min-height: 46px;
  border: 1px solid #87c5b8;
  border-radius: 8px;
  background: #f3fbf8;
  color: #174d43;
  padding: 10px 12px;
  box-shadow: 0 8px 22px rgba(29, 44, 55, 0.17);
}
.modal-backdrop {
  position: absolute;
  inset: 0;
  z-index: 20;
  display: flex;
  align-items: center;
  justify-content: center;
  background: rgba(22, 35, 43, 0.28);
}
.modal {
  width: 456px;
  max-width: calc(100% - 32px);
  background: #ffffff;
  border: 1px solid #9eafba;
  border-radius: 8px;
  box-shadow: 0 18px 42px rgba(20, 31, 38, 0.28);
}
.modal-head {
  min-height: 50px;
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
  padding: 0 12px;
  border-bottom: 1px solid #d8e1e7;
}
.modal-body { padding: 12px; display: grid; gap: 10px; }
.modal-actions {
  display: flex;
  justify-content: flex-end;
  gap: 8px;
  padding: 0 12px 12px;
}
@media (max-width: 980px) {
  .app-shell { grid-template-columns: 190px minmax(0, 1fr); }
  .topbar { grid-template-columns: 1fr; align-content: center; }
  .content { grid-template-columns: 1fr; overflow: auto; }
  .main-stage, .right-rail { overflow: visible; }
  .metrics { grid-template-columns: repeat(2, minmax(126px, 1fr)); }
  .split-grid,
  .analytics-grid { grid-template-columns: 1fr; }
  .board { grid-template-columns: 1fr; }
}
`;

const views = [
  { id: 'grid', icon: 'G', label: 'Data Grid' },
  { id: 'board', icon: 'B', label: 'Kanban Board' },
  { id: 'form', icon: 'F', label: 'Form Studio' },
  { id: 'analytics', icon: 'A', label: 'Analytics' }
];

const baseRows = [
  { id: 'MB-101', client: 'Auth Policy', owner: 'Security', state: 'Ready', risk: 'High', value: 184, note: 'Reviewer required' },
  { id: 'MB-102', client: 'Billing Sync', owner: 'Finance', state: 'Review', risk: 'Medium', value: 92, note: 'Dry run clean' },
  { id: 'MB-103', client: 'Worker Pool', owner: 'Platform', state: 'Ready', risk: 'Low', value: 44, note: 'Scale window' },
  { id: 'MB-104', client: 'Report Export', owner: 'Data', state: 'Blocked', risk: 'High', value: 128, note: 'Freshness issue' },
  { id: 'MB-105', client: 'Desktop Shell', owner: 'Client', state: 'Review', risk: 'Medium', value: 76, note: 'Visual pass' },
  { id: 'MB-106', client: 'API Gateway', owner: 'Network', state: 'Ready', risk: 'Low', value: 210, note: 'Canary group B' },
  { id: 'MB-107', client: 'Telemetry Digest', owner: 'Ops', state: 'Ready', risk: 'Medium', value: 58, note: 'Batch enabled' },
  { id: 'MB-108', client: 'Native Controls', owner: 'Runtime', state: 'Review', risk: 'High', value: 136, note: 'Input edge cases' }
];

const initialCards = [
  { id: 'task-1', title: 'Snapshot table after filter', lane: 'todo', owner: 'QA', points: 3 },
  { id: 'task-2', title: 'Compare dynamic card geometry', lane: 'todo', owner: 'UI', points: 5 },
  { id: 'task-3', title: 'Review input event parity', lane: 'doing', owner: 'Runtime', points: 8 },
  { id: 'task-4', title: 'Capture idle CPU sample', lane: 'doing', owner: 'Perf', points: 3 },
  { id: 'task-5', title: 'Archive completed scenario', lane: 'done', owner: 'Ops', points: 2 }
];

const logs = [
  ['09:10', 'Window opened and initial snapshot completed.'],
  ['09:17', 'Data grid filtered by owner and state.'],
  ['09:24', 'Kanban lane received a dynamic card.'],
  ['09:31', 'Form draft moved through validation.'],
  ['09:43', 'Chart density changed with stable rail geometry.'],
  ['09:56', 'Runtime returned no JavaScript errors.'],
  ['10:08', 'CPU sample entered idle wait window.'],
  ['10:19', 'Screenshot evidence copied to tmp output.']
];

let toastSeq = 0;

function installStyles() {
  let style = document.getElementById('complex-layout-style');
  if (!style) {
    style = document.createElement('style');
    style.id = 'complex-layout-style';
    document.head.appendChild(style);
  }
  style.textContent = css;
}

function badgeClass(value) {
  if (value === 'Ready' || value === 'Low') return 'badge good';
  if (value === 'Review' || value === 'Medium') return 'badge warn';
  if (value === 'Blocked' || value === 'High') return 'badge hot';
  return 'badge';
}

function MetricGrid({ rows, cards }) {
  const ready = rows.filter((row) => row.state === 'Ready').length;
  const blocked = rows.filter((row) => row.state === 'Blocked').length;
  const total = rows.reduce((sum, row) => sum + row.value, 0);
  const doing = cards.filter((card) => card.lane === 'doing').length;
  const metrics = [
    ['good', 'Ready', ready, 'grid rows'],
    ['warn', 'In review', rows.filter((row) => row.state === 'Review').length, 'needs owner'],
    ['hot', 'Blocked', blocked, 'must inspect'],
    ['cool', 'Value', total + 'k', doing + ' active cards']
  ];
  return h('section', { id: 'metric-grid', className: 'metrics' },
    metrics.map((item) => h('article', { className: 'metric ' + item[0], key: item[1] },
      h('div', { className: 'metric-label' }, item[1]),
      h('div', { className: 'metric-value' }, item[2]),
      h('div', { className: 'metric-note' }, item[3])
    ))
  );
}

function DataGrid({ rows, search, density, setDensity, selected, setSelected, addToast }) {
  const [tab, setTab] = useState('all');
  const filtered = useMemo(() => {
    const needle = search.trim().toLowerCase();
    return rows.filter((row) => {
      const stateOk = tab === 'all' || row.state.toLowerCase() === tab;
      const text = `${row.id} ${row.client} ${row.owner} ${row.state} ${row.risk} ${row.note}`.toLowerCase();
      return stateOk && (!needle || text.indexOf(needle) >= 0);
    });
  }, [rows, search, tab]);

  function toggle(id) {
    setSelected((current) => Object.assign({}, current, { [id]: !current[id] }));
  }

  function selectVisible() {
    const next = Object.assign({}, selected);
    const allSelected = filtered.length > 0 && filtered.every((row) => selected[row.id]);
    filtered.forEach((row) => { next[row.id] = !allSelected; });
    setSelected(next);
    addToast((allSelected ? 'Cleared ' : 'Selected ') + filtered.length + ' visible rows.');
  }

  return h('section', { id: 'grid-panel', className: 'panel' },
    h('div', { className: 'panel-head' },
      h('h2', { className: 'panel-title' }, 'Data grid scenario'),
      h('div', { className: 'tabs', id: 'grid-tabs' },
        ['all', 'ready', 'review', 'blocked'].map((item) => h('button', {
          key: item,
          id: 'grid-tab-' + item,
          className: tab === item ? 'active' : '',
          onClick: () => setTab(item)
        }, item[0].toUpperCase() + item.slice(1)))
      )
    ),
    h('div', { className: 'panel-body' },
      h('div', { className: 'toolbar', style: { justifyContent: 'space-between', marginBottom: 10 } },
        h('span', { id: 'grid-count', className: 'badge' }, filtered.length + ' visible'),
        h('div', { className: 'segmented', id: 'density-tabs' },
          h('button', {
            id: 'density-comfortable',
            className: density === 'comfortable' ? 'active' : '',
            onClick: () => setDensity('comfortable')
          }, 'Comfort'),
          h('button', {
            id: 'density-compact',
            className: density === 'compact' ? 'active' : '',
            onClick: () => setDensity('compact')
          }, 'Compact'),
          h('button', { id: 'select-visible', onClick: selectVisible }, 'Select visible')
        )
      ),
      h('div', { id: 'orders-table-wrap', className: 'table-wrap' },
        h('table', { id: 'orders-table', className: density === 'compact' ? 'dense' : '' },
          h('thead', null,
            h('tr', null,
              h('th', { className: 'select-col' }, ''),
              h('th', { className: 'id-col' }, 'ID'),
              h('th', null, 'Client'),
              h('th', { className: 'owner-col' }, 'Owner'),
              h('th', { className: 'state-col' }, 'State'),
              h('th', { className: 'risk-col' }, 'Risk'),
              h('th', { className: 'number-col' }, 'Value'),
              h('th', { className: 'notes-col' }, 'Notes')
            )
          ),
          h('tbody', null,
            filtered.map((row) => h('tr', {
              key: row.id,
              id: 'row-' + row.id.toLowerCase(),
              className: selected[row.id] ? 'selected' : ''
            },
              h('td', { className: 'select-col' }, h('input', {
                id: 'select-' + row.id.toLowerCase(),
                type: 'checkbox',
                checked: !!selected[row.id],
                onChange: () => toggle(row.id)
              })),
              h('td', { className: 'id-col' }, row.id),
              h('td', null, row.client),
              h('td', { className: 'owner-col' }, row.owner),
              h('td', { className: 'state-col' }, h('span', { className: badgeClass(row.state) }, row.state)),
              h('td', { className: 'risk-col' }, h('span', { className: badgeClass(row.risk) }, row.risk)),
              h('td', { className: 'number-col' }, '$' + row.value + 'k'),
              h('td', { className: 'notes-col' }, row.note)
            ))
          )
        )
      )
    )
  );
}

function BoardView({ cards, setCards, addToast }) {
  const lanes = [
    ['todo', 'Todo'],
    ['doing', 'Doing'],
    ['done', 'Done']
  ];

  function addCard() {
    const id = 'task-' + (cards.length + 1);
    setCards((current) => current.concat([{
      id,
      title: 'Dynamic layout card ' + cards.length,
      lane: 'todo',
      owner: 'QA',
      points: 2 + (cards.length % 5)
    }]));
    addToast('Added board card ' + id + '.');
  }

  function advance(card) {
    const nextLane = card.lane === 'todo' ? 'doing' : card.lane === 'doing' ? 'done' : 'todo';
    setCards((current) => current.map((item) => item.id === card.id ? Object.assign({}, item, { lane: nextLane }) : item));
    addToast(card.title + ' moved to ' + nextLane + '.');
  }

  return h('section', { id: 'board-panel', className: 'panel' },
    h('div', { className: 'panel-head' },
      h('h2', { className: 'panel-title' }, 'Kanban dynamic scenario'),
      h('button', { id: 'add-board-card', className: 'primary', onClick: addCard }, 'Add card')
    ),
    h('div', { className: 'panel-body' },
      h('div', { id: 'kanban-board', className: 'board' },
        lanes.map(([lane, label]) => {
          const laneCards = cards.filter((card) => card.lane === lane);
          return h('section', { key: lane, id: 'lane-' + lane, className: 'board-column' },
            h('div', { className: 'column-head' },
              h('span', null, label),
              h('span', { className: 'badge' }, laneCards.length)
            ),
            h('div', { className: 'card-list' },
              laneCards.map((card) => h('article', { key: card.id, id: card.id, className: 'task-card' },
                h('strong', null, card.title),
                h('div', { className: 'task-meta' },
                  h('span', null, card.owner + ' / ' + card.points + 'pt'),
                  h('button', { id: 'advance-' + card.id, onClick: () => advance(card) }, 'Move')
                )
              ))
            )
          );
        })
      )
    )
  );
}

function FormView({ addToast }) {
  const [name, setName] = useState('');
  const [type, setType] = useState('feature');
  const [priority, setPriority] = useState('medium');
  const [notes, setNotes] = useState('');
  const [ack, setAck] = useState(true);
  const [result, setResult] = useState('Draft waiting.');
  const [drawerOpen, setDrawerOpen] = useState(false);

  function submit() {
    if (!name.trim()) {
      setResult('Name is required.');
      addToast('Validation failed.');
      return;
    }
    const text = `${name} saved as ${type} / ${priority}.`;
    setResult(text);
    addToast(text);
  }

  return h('section', { id: 'form-panel', className: 'panel' },
    h('div', { className: 'panel-head' },
      h('h2', { className: 'panel-title' }, 'Form and drawer scenario'),
      h('button', { id: 'toggle-drawer', onClick: () => setDrawerOpen((value) => !value) }, drawerOpen ? 'Close drawer' : 'Open drawer')
    ),
    h('div', { className: 'panel-body split-grid' },
      h('div', null,
        h('div', { className: 'form-grid' },
          h('label', { className: 'field' },
            h('span', null, 'Name'),
            h('input', { id: 'request-name', value: name, placeholder: 'Workflow name', onInput: (event) => setName(event.currentTarget.value) })
          ),
          h('label', { className: 'field' },
            h('span', null, 'Type'),
            h('select', { id: 'request-type', value: type, onChange: (event) => setType(event.currentTarget.value) },
              h('option', { value: 'feature' }, 'Feature'),
              h('option', { value: 'incident' }, 'Incident'),
              h('option', { value: 'maintenance' }, 'Maintenance')
            )
          ),
          h('label', { className: 'field' },
            h('span', null, 'Priority'),
            h('select', { id: 'request-priority', value: priority, onChange: (event) => setPriority(event.currentTarget.value) },
              h('option', { value: 'low' }, 'Low'),
              h('option', { value: 'medium' }, 'Medium'),
              h('option', { value: 'high' }, 'High')
            )
          ),
          h('label', { className: 'field' },
            h('span', null, 'Owner'),
            h('input', { id: 'request-owner', value: 'Runtime', readOnly: true })
          ),
          h('label', { className: 'field full' },
            h('span', null, 'Notes'),
            h('textarea', { id: 'request-notes', value: notes, placeholder: 'Reviewer notes', onInput: (event) => setNotes(event.currentTarget.value) })
          )
        ),
        h('label', { className: 'check-row' },
          h('input', { id: 'request-ack', type: 'checkbox', checked: ack, onChange: (event) => setAck(event.currentTarget.checked) }),
          h('span', null, 'Ready for runtime review')
        ),
        h('div', { className: 'toolbar', style: { justifyContent: 'flex-end' } },
          h('button', { id: 'request-clear', onClick: () => { setName(''); setNotes(''); setResult('Draft cleared.'); } }, 'Clear'),
          h('button', { id: 'request-submit', className: 'primary', disabled: !ack, onClick: submit }, 'Submit')
        ),
        h('div', { id: 'form-result', className: 'result-line' }, result)
      ),
      drawerOpen
        ? h('aside', { id: 'detail-drawer', className: 'drawer' },
          h('strong', null, 'Dynamic detail drawer'),
          h('p', null, 'This side panel appears and disappears without changing the outer shell geometry.'),
          h('span', { className: 'badge warn' }, priority),
          h('button', { id: 'drawer-action', onClick: () => addToast('Drawer action ran.') }, 'Run drawer action')
        )
        : h('aside', { id: 'drawer-placeholder', className: 'preview-box' }, 'Drawer closed')
    )
  );
}

function AnalyticsView({ addToast }) {
  const [mode, setMode] = useState('normal');
  const values = mode === 'stress' ? [86, 72, 95, 64, 79, 88, 69, 91] : [42, 58, 35, 66, 51, 73, 47, 62];
  return h('section', { id: 'analytics-panel', className: 'panel' },
    h('div', { className: 'panel-head' },
      h('h2', { className: 'panel-title' }, 'Analytics and state scenario'),
      h('div', { className: 'tabs', id: 'analytics-tabs' },
        h('button', { id: 'analytics-normal', className: mode === 'normal' ? 'active' : '', onClick: () => setMode('normal') }, 'Normal'),
        h('button', { id: 'analytics-empty', className: mode === 'empty' ? 'active' : '', onClick: () => setMode('empty') }, 'Empty'),
        h('button', { id: 'analytics-stress', className: mode === 'stress' ? 'active' : '', onClick: () => setMode('stress') }, 'Stress')
      )
    ),
    h('div', { className: 'panel-body analytics-grid' },
      mode === 'empty'
        ? h('div', { id: 'analytics-empty-state', className: 'preview-box' }, 'No samples in the selected window.')
        : h('div', { id: 'chart', className: 'chart' },
          values.map((value, index) => h('div', {
            id: 'bar-' + index,
            key: index,
            className: 'bar ' + (value > 84 ? 'hot' : value > 70 ? 'warn' : ''),
            style: { height: value + '%' },
            title: String(value)
          }))
        ),
      h('div', { className: 'preview-stack' },
        h('div', { id: 'analytics-state-preview', className: 'preview-box' }, mode === 'stress' ? 'High churn data window' : 'Stable data window'),
        h('button', { id: 'analytics-toast', className: 'primary', onClick: () => addToast('Analytics state captured.') }, 'Capture state')
      )
    )
  );
}

function ActivityRail() {
  return h('aside', { className: 'right-rail', id: 'right-rail' },
    h('section', { className: 'panel', id: 'activity-panel' },
      h('div', { className: 'panel-head' },
        h('h2', { className: 'panel-title' }, 'Activity log'),
        h('span', { className: 'badge' }, logs.length)
      ),
      h('div', { id: 'activity-feed', className: 'panel-body log-list' },
        logs.map((item, index) => h('div', { key: index, id: 'activity-' + index, className: 'log-item' },
          h('div', { className: 'log-time' }, item[0]),
          h('div', null, item[1])
        ))
      )
    )
  );
}

function SettingsModal({ onClose, addToast }) {
  function apply() {
    addToast('Settings applied.');
    onClose();
  }
  return h('div', { id: 'settings-backdrop', className: 'modal-backdrop' },
    h('section', { id: 'settings-dialog', className: 'modal' },
      h('div', { className: 'modal-head' },
        h('strong', null, 'Runtime settings'),
        h('button', { id: 'settings-close', onClick: onClose }, 'Close')
      ),
      h('div', { className: 'modal-body' },
        h('label', { className: 'check-row' },
          h('input', { id: 'settings-raf', type: 'checkbox', defaultChecked: true }),
          h('span', null, 'Prefer animation frame batching')
        ),
        h('label', { className: 'field' },
          h('span', null, 'Snapshot depth'),
          h('select', { id: 'settings-depth', defaultValue: 'deep' },
            h('option', { value: 'shallow' }, 'Shallow'),
            h('option', { value: 'deep' }, 'Deep'),
            h('option', { value: 'full' }, 'Full')
          )
        )
      ),
      h('div', { className: 'modal-actions' },
        h('button', { id: 'settings-cancel', onClick: onClose }, 'Cancel'),
        h('button', { id: 'settings-apply', className: 'primary', onClick: apply }, 'Apply')
      )
    )
  );
}

function ToastStack({ toasts }) {
  return h('div', { id: 'toast-stack', className: 'toast-stack' },
    toasts.map((toast) => h('div', { key: toast.id, id: 'toast-' + toast.id, className: 'toast' }, toast.message))
  );
}

function App() {
  const [view, setView] = useState('grid');
  const [search, setSearch] = useState('');
  const [rows, setRows] = useState(baseRows);
  const [cards, setCards] = useState(initialCards);
  const [density, setDensity] = useState('comfortable');
  const [selected, setSelected] = useState({});
  const [modalOpen, setModalOpen] = useState(false);
  const [toasts, setToasts] = useState([]);

  function addToast(message) {
    const id = ++toastSeq;
    setToasts((current) => current.concat([{ id, message }]).slice(-3));
    setTimeout(() => setToasts((current) => current.filter((toast) => toast.id !== id)), 2200);
  }

  function addRow() {
    const next = rows.length + 101;
    const row = {
      id: 'MB-' + next,
      client: 'Dynamic Layout ' + (rows.length + 1),
      owner: rows.length % 2 ? 'Runtime' : 'QA',
      state: rows.length % 3 === 0 ? 'Review' : 'Ready',
      risk: rows.length % 2 ? 'Medium' : 'Low',
      value: 50 + rows.length * 9,
      note: 'Added during verification'
    };
    setRows((current) => current.concat([row]));
    addToast('Added row ' + row.id + '.');
  }

  const title = views.find((item) => item.id === view);

  return h('main', { id: 'app-shell', className: 'app-shell' },
    h('aside', { id: 'sidebar', className: 'sidebar' },
      h('div', { className: 'brand' },
        h('div', { className: 'brand-mark' }, 'MB'),
        h('div', null,
          h('div', { className: 'brand-title' }, 'Layout Lab'),
          h('div', { className: 'brand-note' }, 'Dynamic stress scenes')
        )
      ),
      h('nav', { className: 'nav' },
        views.map((item) => h('button', {
          key: item.id,
          id: 'nav-' + item.id,
          className: view === item.id ? 'active' : '',
          onClick: () => setView(item.id)
        },
          h('span', null, item.icon),
          h('span', null, item.label)
        ))
      ),
      h('div', { className: 'sidebar-footer' },
        h('div', null, 'Runtime: tool'),
        h('div', { id: 'view-indicator' }, 'View: ' + view),
        h('div', null, 'Rows: ' + rows.length + ' / Cards: ' + cards.length)
      )
    ),
    h('header', { id: 'topbar', className: 'topbar' },
      h('div', { className: 'title' },
        h('h1', { id: 'page-title' }, title.label),
        h('p', { id: 'page-caption' }, 'Complex layout scenario with live DOM mutations')
      ),
      h('input', {
        id: 'global-search',
        value: search,
        placeholder: 'Search grid rows',
        onInput: (event) => setSearch(event.currentTarget.value)
      }),
      h('div', { className: 'toolbar' },
        h('button', { id: 'open-settings', onClick: () => setModalOpen(true) }, 'Settings'),
        h('button', { id: 'add-row', className: 'primary', onClick: addRow }, 'Add row'),
        h('button', { id: 'clear-search', onClick: () => setSearch('') }, 'Clear')
      )
    ),
    h('section', { id: 'content', className: 'content' },
      h('div', { id: 'main-stage', className: 'main-stage' },
        h(MetricGrid, { rows, cards }),
        view === 'grid' && h(DataGrid, { rows, search, density, setDensity, selected, setSelected, addToast }),
        view === 'board' && h(BoardView, { cards, setCards, addToast }),
        view === 'form' && h(FormView, { addToast }),
        view === 'analytics' && h(AnalyticsView, { addToast })
      ),
      h(ActivityRail)
    ),
    modalOpen && h(SettingsModal, { onClose: () => setModalOpen(false), addToast }),
    h(ToastStack, { toasts })
  );
}

installStyles();
render(h(App), document.getElementById('root') || document.body);
console.log('[complex-layout-dynamics] loaded');
