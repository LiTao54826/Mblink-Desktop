import { h, render } from 'preact';
import { useEffect, useMemo, useState } from 'preact/hooks';
import { hostApi } from './bridge.js';
import { palette, shadows } from './styles/theme.js';

const PURPOSE = 'desktop-app';
const statusTabs = ['All', 'Ready', 'Review', 'Blocked'];
const regions = ['All regions', 'NA', 'EU', 'APAC', 'LATAM'];

function installDocumentShell() {
  Object.assign(document.documentElement.style, {
    width: '100%',
    height: '100%',
    margin: '0',
    overflow: 'hidden'
  });
  Object.assign(document.body.style, {
    width: '100%',
    height: '100%',
    margin: '0',
    overflow: 'hidden',
    background: palette.bg
  });
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

function installStyles() {
  let style = document.getElementById('modern-table-demo-styles');
  if (!style) {
    style = document.createElement('style');
    style.id = 'modern-table-demo-styles';
    document.head.appendChild(style);
  }
  style.textContent = `
    * { box-sizing: border-box; }
    body { font-family: Segoe UI, Arial, sans-serif; color: ${palette.text}; }
    button, input, select { font: inherit; }
    .app-shell {
      width: 100%;
      height: 100%;
      display: grid;
      grid-template-rows: 40px 1fr;
      overflow: hidden;
      background: ${palette.bg};
    }
    .titlebar {
      display: flex;
      justify-content: space-between;
      align-items: center;
      padding: 0 10px 0 16px;
      background: #202939;
      color: #eef2f6;
      -webkit-app-region: drag;
      user-select: none;
    }
    .titlebar-title {
      display: flex;
      align-items: center;
      gap: 10px;
      min-width: 0;
      font-size: 13px;
      font-weight: 700;
    }
    .titlebar-dot {
      width: 8px;
      height: 8px;
      border-radius: 999px;
      background: #38bdf8;
    }
    .window-controls {
      display: flex;
      flex: 0 0 auto;
      gap: 6px;
      -webkit-app-region: no-drag;
    }
    .window-control {
      width: 30px;
      height: 26px;
      border-radius: 5px;
      border: 0;
      color: #e5e7eb;
      background: rgba(255, 255, 255, 0.08);
      cursor: pointer;
    }
    .window-control.close { background: #d94b5f; }
    .workspace {
      min-height: 0;
      display: grid;
      grid-template-columns: 248px 1fr;
      gap: 14px;
      padding: 14px;
      overflow: hidden;
    }
    .sidebar, .content {
      min-height: 0;
      background: ${palette.surface};
      border: 1px solid ${palette.line};
      border-radius: 8px;
      box-shadow: ${shadows.panel};
      overflow: hidden;
    }
    .sidebar {
      display: grid;
      grid-template-rows: auto auto 1fr;
    }
    .sidebar-header {
      padding: 16px;
      border-bottom: 1px solid ${palette.line};
    }
    .eyebrow {
      margin: 0 0 6px;
      font-size: 11px;
      font-weight: 800;
      letter-spacing: 0;
      text-transform: uppercase;
      color: ${palette.blue};
    }
    h1 {
      margin: 0;
      font-size: 18px;
      line-height: 1.25;
      letter-spacing: 0;
    }
    .summary-stack {
      display: grid;
      gap: 8px;
      padding: 12px;
      border-bottom: 1px solid ${palette.line};
      background: ${palette.surfaceSoft};
    }
    .metric {
      display: grid;
      grid-template-columns: 1fr auto;
      align-items: center;
      gap: 10px;
      padding: 10px;
      border: 1px solid ${palette.line};
      border-radius: 8px;
      background: #ffffff;
    }
    .metric span { color: ${palette.muted}; font-size: 12px; }
    .metric strong { font-size: 18px; }
    .filters {
      padding: 12px;
      overflow: auto;
      display: grid;
      gap: 12px;
      align-content: start;
    }
    .field {
      display: grid;
      gap: 6px;
    }
    .field label {
      font-size: 12px;
      font-weight: 700;
      color: ${palette.muted};
    }
    .input, .select {
      width: 100%;
      height: 34px;
      min-height: 34px;
      max-height: 34px;
      border: 1px solid ${palette.lineStrong};
      border-radius: 6px;
      background: #ffffff;
      color: ${palette.text};
      padding: 0 9px;
    }
    .segmented {
      display: grid;
      gap: 6px;
    }
    .segmented button {
      min-height: 34px;
      text-align: left;
      border: 1px solid ${palette.line};
      border-radius: 6px;
      background: #ffffff;
      color: ${palette.text};
      padding: 7px 10px;
      cursor: pointer;
    }
    .segmented button.active {
      color: #ffffff;
      background: ${palette.blue};
      border-color: ${palette.blue};
      box-shadow: ${shadows.raised};
    }
    .content {
      display: grid;
      grid-template-rows: auto 1fr auto;
      height: 100%;
    }
    .toolbar {
      min-height: 70px;
      display: flex;
      justify-content: space-between;
      align-items: center;
      gap: 14px;
      padding: 14px 16px;
      border-bottom: 1px solid ${palette.line};
      background: #ffffff;
    }
    .toolbar-title {
      display: grid;
      gap: 4px;
      flex: 1 1 auto;
      min-width: 0;
    }
    .toolbar-title h2 {
      margin: 0;
      font-size: 18px;
      line-height: 1.25;
      letter-spacing: 0;
    }
    .toolbar-title span {
      color: ${palette.muted};
      font-size: 12px;
    }
    .actions {
      display: flex;
      align-items: center;
      justify-content: flex-end;
      flex: 0 0 auto;
      flex-wrap: wrap;
      gap: 8px;
    }
    .primary-btn, .ghost-btn {
      min-height: 34px;
      border-radius: 6px;
      padding: 7px 12px;
      border: 1px solid ${palette.lineStrong};
      cursor: pointer;
      font-weight: 700;
    }
    .primary-btn {
      color: #ffffff;
      background: ${palette.blue};
      border-color: ${palette.blue};
    }
    .ghost-btn {
      color: ${palette.text};
      background: #ffffff;
    }
    .table-region {
      height: 605px;
      max-height: 605px;
      min-height: 0;
      overflow: auto;
      background: #ffffff;
    }
    .table-region::after {
      content: "";
      display: block;
      height: 1px;
    }
    table.modern-table {
      width: 100%;
      min-width: 1120px;
      border-collapse: separate;
      border-spacing: 0;
      table-layout: fixed;
      user-select: text;
    }
    .modern-table thead th {
      position: sticky;
      top: 0;
      z-index: 4;
      padding: 8px 10px;
      color: #475467;
      background: #f3f6fa;
      border-bottom: 1px solid ${palette.lineStrong};
      font-size: 12px;
      font-weight: 800;
      text-align: left;
      white-space: nowrap;
    }
    .modern-table th, .modern-table td {
      border-right: 1px solid ${palette.line};
      border-bottom: 1px solid ${palette.line};
    }
    .modern-table th:last-child, .modern-table td:last-child { border-right: 0; }
    .modern-table tbody td {
      height: 48px;
      padding: 8px 10px;
      background: #ffffff;
      color: ${palette.text};
      font-size: 13px;
      vertical-align: middle;
      white-space: nowrap;
      overflow: hidden;
      text-overflow: ellipsis;
    }
    .modern-table tbody tr.selected td {
      background: #eef5ff;
    }
    .modern-table tbody tr:hover td {
      background: #f8fbff;
    }
    .modern-table .select-col {
      width: 42px;
      text-align: center;
    }
    .modern-table .id-col {
      width: 88px;
      position: sticky;
      left: 0;
      z-index: 3;
      background: #ffffff;
      font-weight: 800;
      color: ${palette.blue};
    }
    .modern-table thead .id-col {
      z-index: 5;
      background: #f3f6fa;
      color: #475467;
    }
    .modern-table tbody tr.selected .id-col { background: #eef5ff; }
    .modern-table .client-col { width: 180px; }
    .modern-table .owner-col { width: 130px; }
    .modern-table .status-col { width: 110px; }
    .modern-table .risk-col { width: 96px; }
    .modern-table .number-col { width: 112px; text-align: right; }
    .modern-table .small-col { width: 82px; }
    .modern-table .notes-col { width: 170px; }
    .sort-btn {
      width: 100%;
      padding: 0;
      border: 0;
      background: transparent;
      color: inherit;
      display: flex;
      align-items: center;
      justify-content: space-between;
      gap: 8px;
      font-weight: 800;
      line-height: 1.2;
      cursor: pointer;
    }
    .sort-mark { color: ${palette.faint}; font-size: 11px; }
    .badge {
      display: inline-flex;
      align-items: center;
      min-height: 24px;
      max-width: 100%;
      padding: 3px 8px;
      border-radius: 999px;
      font-weight: 800;
      font-size: 11px;
      border: 1px solid transparent;
    }
    .badge.ready { color: ${palette.green}; background: #eaf7f1; border-color: #bde6d4; }
    .badge.review { color: ${palette.violet}; background: #f1efff; border-color: #d8d2ff; }
    .badge.blocked { color: ${palette.red}; background: #fff1f2; border-color: #fecdd3; }
    .badge.low { color: ${palette.green}; background: #ecfdf5; border-color: #bbf7d0; }
    .badge.medium { color: ${palette.amber}; background: #fff7ed; border-color: #fed7aa; }
    .badge.high { color: ${palette.red}; background: #fef2f2; border-color: #fecaca; }
    .footer {
      display: grid;
      grid-template-columns: 1fr auto;
      align-items: center;
      gap: 12px;
      padding: 10px 16px;
      border-top: 1px solid ${palette.line};
      color: ${palette.muted};
      background: ${palette.surfaceSoft};
      font-size: 12px;
    }
    .reviewed {
      color: ${palette.cyan};
      font-weight: 800;
    }
  `;
}

function formatMoney(value) {
  const number = Number(value || 0);
  if (number >= 1000) return `$${Math.round(number / 1000)}k`;
  return `$${number}`;
}

function normalize(value) {
  return String(value ?? '').toLowerCase();
}

function Badge({ value, type }) {
  return h('span', { class: `badge ${normalize(type || value)}` }, value);
}

function SortHeader({ label, field, sort, onSort, className }) {
  const active = sort.field === field;
  const mark = active ? (sort.direction === 'asc' ? '^' : 'v') : '-';
  return h('th', { class: className },
    h('button', { class: 'sort-btn', onClick: () => onSort(field), type: 'button' },
      h('span', null, label),
      h('span', { class: 'sort-mark' }, mark)
    )
  );
}

function App() {
  const [info, setInfo] = useState({ host: 'loading', mode: 'loading' });
  const [rows, setRows] = useState([]);
  const [query, setQuery] = useState('');
  const [status, setStatus] = useState('All');
  const [region, setRegion] = useState('All regions');
  const [sort, setSort] = useState({ field: 'updated', direction: 'desc' });
  const [selected, setSelected] = useState({});
  const [lastAction, setLastAction] = useState('No batch action yet');

  useEffect(() => {
    let active = true;
    hostApi.getTemplateInfo({ purpose: PURPOSE }).then((value) => {
      if (active) setInfo(value);
    });
    hostApi.getOrders({}).then((value) => {
      if (active) setRows(value.rows ?? []);
    });
    return () => {
      active = false;
    };
  }, []);

  const filteredRows = useMemo(() => {
    const text = normalize(query);
    const output = rows.filter((row) => {
      const matchesStatus = status === 'All' || row.status === status;
      const matchesRegion = region === 'All regions' || row.region === region;
      const haystack = `${row.id} ${row.client} ${row.owner} ${row.stage} ${row.notes}`.toLowerCase();
      return matchesStatus && matchesRegion && (!text || haystack.includes(text));
    });
    const direction = sort.direction === 'asc' ? 1 : -1;
    return output.slice().sort((a, b) => {
      const left = a[sort.field];
      const right = b[sort.field];
      if (typeof left === 'number' && typeof right === 'number') return (left - right) * direction;
      return String(left).localeCompare(String(right)) * direction;
    });
  }, [rows, query, status, region, sort]);

  const selectedIds = Object.keys(selected).filter((id) => selected[id]);
  const totalValue = filteredRows.reduce((sum, row) => sum + row.value, 0);
  const blockedCount = filteredRows.filter((row) => row.status === 'Blocked').length;
  const highRiskCount = filteredRows.filter((row) => row.risk === 'High').length;

  function changeSort(field) {
    setSort((current) => ({
      field,
      direction: current.field === field && current.direction === 'asc' ? 'desc' : 'asc'
    }));
  }

  function toggleRow(id) {
    setSelected((current) => ({ ...current, [id]: !current[id] }));
  }

  function toggleVisibleRows() {
    const allSelected = filteredRows.length > 0 && filteredRows.every((row) => selected[row.id]);
    const next = { ...selected };
    filteredRows.forEach((row) => {
      next[row.id] = !allSelected;
    });
    setSelected(next);
  }

  async function markReviewed() {
    const result = await hostApi.markReviewed({ ids: selectedIds });
    if (Array.isArray(result.rows)) setRows(result.rows);
    setLastAction(`${selectedIds.length} rows marked reviewed`);
  }

  function resetFilters() {
    setQuery('');
    setStatus('All');
    setRegion('All regions');
    setSelected({});
    setLastAction('Filters reset');
  }

  return h('main', { class: 'app-shell' },
      h('div', { class: 'titlebar' },
        h('div', { class: 'titlebar-title' },
        h('span', { class: 'titlebar-dot' }),
        h('span', null, 'Modern Table Demo')
      ),
        h('div', { class: 'window-controls' },
        h('button', { class: 'window-control', title: 'Minimize', type: 'button', style: { WebkitWindowControl: 'minimize' } }, '-'),
        h('button', { class: 'window-control', title: 'Maximize', type: 'button', style: { WebkitWindowControl: 'maximize' } }, '[ ]'),
        h('button', { class: 'window-control close', title: 'Close', type: 'button', style: { WebkitWindowControl: 'close' } }, 'x')
      )
    ),
    h('section', { class: 'workspace' },
      h('aside', { class: 'sidebar' },
        h('div', { class: 'sidebar-header' },
          h('p', { class: 'eyebrow' }, 'Operations'),
          h('h1', null, 'Trade Review Queue')
        ),
        h('div', { class: 'summary-stack' },
          h('div', { class: 'metric' }, h('span', null, 'Rows'), h('strong', null, filteredRows.length)),
          h('div', { class: 'metric' }, h('span', null, 'Value'), h('strong', null, formatMoney(totalValue))),
          h('div', { class: 'metric' }, h('span', null, 'Blocked'), h('strong', null, blockedCount)),
          h('div', { class: 'metric' }, h('span', null, 'High risk'), h('strong', null, highRiskCount))
        ),
        h('div', { class: 'filters' },
          h('div', { class: 'field' },
            h('label', { for: 'table-search' }, 'Search'),
            h('input', {
              id: 'table-search',
              class: 'input',
              style: { height: '34px', minHeight: '34px', maxHeight: '34px', padding: '0 9px' },
              value: query,
              placeholder: 'Client, owner, stage',
              onInput: (event) => setQuery(event.currentTarget.value)
            })
          ),
          h('div', { class: 'field' },
            h('label', null, 'Status'),
            h('div', { class: 'segmented', id: 'status-tabs' },
              statusTabs.map((item) => h('button', {
                key: item,
                class: item === status ? 'active' : '',
                onClick: () => setStatus(item)
              }, item))
            )
          ),
          h('div', { class: 'field' },
            h('label', { for: 'region-filter' }, 'Region'),
            h('select', {
              id: 'region-filter',
              class: 'select',
              style: { height: '34px', minHeight: '34px', maxHeight: '34px', padding: '0 9px' },
              value: region,
              onInput: (event) => setRegion(event.currentTarget.value)
            }, regions.map((item) => h('option', { key: item, value: item }, item)))
          ),
          h('button', { class: 'ghost-btn', onClick: resetFilters }, 'Reset')
        )
      ),
      h('section', { class: 'content', style: { height: '712px', maxHeight: '712px' } },
        h('div', { class: 'toolbar' },
          h('div', { class: 'toolbar-title' },
            h('h2', null, 'Order Pipeline'),
            h('span', null, `${info.mode} / ${lastAction}`)
          ),
          h('div', { class: 'actions' },
            h('button', {
              class: 'ghost-btn',
              disabled: selectedIds.length === 0,
              onClick: () => setLastAction(`${selectedIds.length} rows queued`)
            }, `Queue ${selectedIds.length}`),
            h('button', {
              class: 'primary-btn',
              disabled: selectedIds.length === 0,
              onClick: markReviewed
            }, 'Mark reviewed')
          )
        ),
        h('div', {
          class: 'table-region',
          id: 'orders-table-region',
          style: { height: '605px', maxHeight: '605px', overflow: 'auto' }
        },
          h('table', { class: 'modern-table', id: 'orders-table' },
            h('thead', null,
              h('tr', null,
                h('th', { class: 'select-col' },
                  h('input', {
                    id: 'select-visible',
                    type: 'checkbox',
                    checked: filteredRows.length > 0 && filteredRows.every((row) => selected[row.id]),
                    onClick: toggleVisibleRows
                  })
                ),
                h(SortHeader, { label: 'ID', field: 'id', sort, onSort: changeSort, className: 'id-col' }),
                h(SortHeader, { label: 'Client', field: 'client', sort, onSort: changeSort, className: 'client-col' }),
                h(SortHeader, { label: 'Owner', field: 'owner', sort, onSort: changeSort, className: 'owner-col' }),
                h(SortHeader, { label: 'Status', field: 'status', sort, onSort: changeSort, className: 'status-col' }),
                h(SortHeader, { label: 'Risk', field: 'risk', sort, onSort: changeSort, className: 'risk-col' }),
                h(SortHeader, { label: 'Value', field: 'value', sort, onSort: changeSort, className: 'number-col' }),
                h(SortHeader, { label: 'Margin', field: 'margin', sort, onSort: changeSort, className: 'small-col number-col' }),
                h(SortHeader, { label: 'Updated', field: 'updated', sort, onSort: changeSort, className: 'small-col' }),
                h(SortHeader, { label: 'Stage', field: 'stage', sort, onSort: changeSort, className: 'owner-col' }),
                h('th', { class: 'small-col' }, 'ETA'),
                h('th', { class: 'notes-col' }, 'Notes')
              )
            ),
            h('tbody', null,
              filteredRows.map((row) => h('tr', { key: row.id, class: selected[row.id] ? 'selected' : '' },
                h('td', { class: 'select-col' },
                  h('input', {
                    type: 'checkbox',
                    checked: !!selected[row.id],
                    onClick: () => toggleRow(row.id)
                  })
                ),
                h('td', { class: 'id-col' }, row.id),
                h('td', { class: 'client-col', title: row.client }, row.client),
                h('td', { class: 'owner-col' }, row.owner),
                h('td', { class: 'status-col' }, h(Badge, { value: row.status, type: row.status })),
                h('td', { class: 'risk-col' }, h(Badge, { value: row.risk, type: row.risk })),
                h('td', { class: 'number-col' }, formatMoney(row.value)),
                h('td', { class: 'small-col number-col' }, `${row.margin}%`),
                h('td', { class: 'small-col' }, row.updated.slice(5)),
                h('td', { class: 'owner-col' }, row.stage),
                h('td', { class: 'small-col' }, row.eta),
                h('td', { class: 'notes-col' },
                  row.reviewed ? h('span', { class: 'reviewed' }, 'Reviewed') : row.notes
                )
              ))
            )
          )
        ),
        h('footer', { class: 'footer' },
          h('span', null, `${selectedIds.length} selected`),
          h('span', null, 'Sticky header, sticky ID column, sorting, filtering, and row selection')
        )
      )
    )
  );
}

installDocumentShell();
installStyles();
render(h(App), mountRoot());
