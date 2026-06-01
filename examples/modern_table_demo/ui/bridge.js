const mockState = {
  reviewedIds: new Set(['TR-1007', 'TR-1011'])
};

function backendFunction(name) {
  const backend = globalThis.backend;
  if (!backend || typeof backend[name] !== 'function') return null;
  return backend[name].bind(backend);
}

async function callBackend(name, payload, fallback) {
  const fn = backendFunction(name);
  if (fn) return (await fn(payload ?? {})) ?? {};
  return fallback(payload ?? {});
}

const orders = [
  {
    id: 'TR-1001',
    client: 'Northstar Labs',
    owner: 'Mina Chen',
    region: 'APAC',
    status: 'Ready',
    risk: 'Low',
    value: 128400,
    margin: 31,
    updated: '2026-05-31',
    stage: 'Fulfillment',
    eta: '2d',
    notes: 'Clean handoff'
  },
  {
    id: 'TR-1002',
    client: 'Atlas Grid',
    owner: 'Owen Park',
    region: 'NA',
    status: 'Review',
    risk: 'Medium',
    value: 88400,
    margin: 22,
    updated: '2026-05-29',
    stage: 'Pricing',
    eta: '4d',
    notes: 'Discount approval'
  },
  {
    id: 'TR-1003',
    client: 'HelioWorks',
    owner: 'Rae Stone',
    region: 'EU',
    status: 'Blocked',
    risk: 'High',
    value: 231900,
    margin: 18,
    updated: '2026-05-28',
    stage: 'Legal',
    eta: '8d',
    notes: 'MSA redlines'
  },
  {
    id: 'TR-1004',
    client: 'Riverline Ops',
    owner: 'Theo Grant',
    region: 'NA',
    status: 'Ready',
    risk: 'Low',
    value: 64200,
    margin: 27,
    updated: '2026-05-31',
    stage: 'Shipping',
    eta: '1d',
    notes: 'Rush batch'
  },
  {
    id: 'TR-1005',
    client: 'Kairo Systems',
    owner: 'June Watts',
    region: 'LATAM',
    status: 'Review',
    risk: 'Medium',
    value: 157800,
    margin: 25,
    updated: '2026-05-30',
    stage: 'QA',
    eta: '3d',
    notes: 'Sample variance'
  },
  {
    id: 'TR-1006',
    client: 'Vector Forge',
    owner: 'Nora Ali',
    region: 'EU',
    status: 'Ready',
    risk: 'Low',
    value: 97800,
    margin: 29,
    updated: '2026-05-27',
    stage: 'Fulfillment',
    eta: '5d',
    notes: 'Standard route'
  },
  {
    id: 'TR-1007',
    client: 'Pioneer Bio',
    owner: 'Iris Nolan',
    region: 'APAC',
    status: 'Review',
    risk: 'High',
    value: 312600,
    margin: 16,
    updated: '2026-05-26',
    stage: 'Finance',
    eta: '6d',
    notes: 'Credit hold'
  },
  {
    id: 'TR-1008',
    client: 'Cobalt Rail',
    owner: 'Leo Burns',
    region: 'NA',
    status: 'Blocked',
    risk: 'High',
    value: 423500,
    margin: 12,
    updated: '2026-05-25',
    stage: 'Supply',
    eta: '12d',
    notes: 'Part shortage'
  },
  {
    id: 'TR-1009',
    client: 'Orbit House',
    owner: 'Sana Reed',
    region: 'EU',
    status: 'Ready',
    risk: 'Low',
    value: 74600,
    margin: 34,
    updated: '2026-05-31',
    stage: 'Fulfillment',
    eta: '2d',
    notes: 'Repeat buyer'
  },
  {
    id: 'TR-1010',
    client: 'SummitWorks',
    owner: 'Vera Liu',
    region: 'NA',
    status: 'Review',
    risk: 'Medium',
    value: 196300,
    margin: 24,
    updated: '2026-05-24',
    stage: 'Contract',
    eta: '7d',
    notes: 'Term update'
  },
  {
    id: 'TR-1011',
    client: 'Redwood Cloud',
    owner: 'Ari Vega',
    region: 'APAC',
    status: 'Ready',
    risk: 'Medium',
    value: 118700,
    margin: 28,
    updated: '2026-05-30',
    stage: 'QA',
    eta: '3d',
    notes: 'Inspection queued'
  },
  {
    id: 'TR-1012',
    client: 'Quartz Media',
    owner: 'Mika Holt',
    region: 'LATAM',
    status: 'Blocked',
    risk: 'High',
    value: 275400,
    margin: 14,
    updated: '2026-05-23',
    stage: 'Legal',
    eta: '10d',
    notes: 'DPA missing'
  }
];

const generatedOwners = ['Eli Moore', 'Kai Brooks', 'Lena Ford', 'Noah Singh', 'Tess Blake', 'Mara Day'];
const generatedStages = ['Packaging', 'Compliance', 'Allocation', 'Customs', 'Replenish', 'Dispatch'];
const generatedNotes = ['Awaiting dock slot', 'Priority lane', 'Manual check', 'Partner callback', 'Split shipment', 'Inventory sweep'];

function twoDigit(value) {
  return value < 10 ? `0${value}` : String(value);
}

function expandedOrders() {
  const generated = orders.map((row, index) => ({
    ...row,
    id: `TR-20${twoDigit(index + 1)}`,
    client: `${row.client} ${index % 2 === 0 ? 'East' : 'West'}`,
    owner: generatedOwners[index % generatedOwners.length],
    status: index % 5 === 0 ? 'Blocked' : (index % 2 === 0 ? 'Review' : 'Ready'),
    risk: index % 4 === 0 ? 'High' : (index % 3 === 0 ? 'Medium' : 'Low'),
    value: row.value + (index + 1) * 18400,
    margin: Math.max(9, row.margin - (index % 5)),
    updated: `2026-05-${twoDigit(22 - (index % 10))}`,
    stage: generatedStages[index % generatedStages.length],
    eta: `${3 + (index % 8)}d`,
    notes: generatedNotes[index % generatedNotes.length]
  }));
  return orders.concat(generated);
}

function rowsWithReviewState() {
  return expandedOrders().map((row) => ({
    ...row,
    reviewed: mockState.reviewedIds.has(row.id)
  }));
}

export const hostApi = {
  getTemplateInfo(defaults = {}) {
    return callBackend('getTemplateInfo', defaults, async () => ({
      purpose: defaults.purpose ?? 'desktop-app',
      runtime: 'tool',
      host: 'mbink-ui-dev mock backend',
      mode: 'dev-mock',
      capabilities: {
        backend: false,
        snapshot: true,
        table: true
      }
    }));
  },

  getOrders(payload = {}) {
    return callBackend('getOrders', payload, async () => ({
      ok: true,
      generatedAt: '2026-05-31T17:50:00Z',
      rows: rowsWithReviewState()
    }));
  },

  markReviewed(payload = {}) {
    return callBackend('markReviewed', payload, async (args) => {
      const ids = Array.isArray(args.ids) ? args.ids : [];
      ids.forEach((id) => mockState.reviewedIds.add(id));
      return {
        ok: true,
        reviewed: ids.length,
        rows: rowsWithReviewState()
      };
    });
  }
};
