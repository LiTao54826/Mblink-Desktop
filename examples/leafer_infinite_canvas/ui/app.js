import {
  Leafer,
  Rect,
  Text,
  Group,
  Path,
  Line,
  Ellipse,
  PointerEvent,
  DragEvent,
  RenderEvent
} from '../../leafer_ui_showcase/js/leafer-ui/web.module.min.js';

const STAGE = { width: 960, height: 600 };
const GRID = { extent: 1600, minor: 40, major: 160 };

const nodeSeed = [
  {
    id: 'inbox',
    title: 'Inbox',
    label: 'events',
    x: -360,
    y: -138,
    width: 176,
    height: 92,
    color: '#5474a8',
    fill: '#ffffff'
  },
  {
    id: 'model',
    title: 'Model',
    label: 'reasoning',
    x: -74,
    y: -168,
    width: 192,
    height: 104,
    color: '#2f8f7a',
    fill: '#ffffff'
  },
  {
    id: 'canvas',
    title: 'Canvas',
    label: 'live surface',
    x: 222,
    y: -86,
    width: 184,
    height: 96,
    color: '#b8687a',
    fill: '#ffffff'
  },
  {
    id: 'review',
    title: 'Review',
    label: 'checks',
    x: -124,
    y: 88,
    width: 186,
    height: 96,
    color: '#a57936',
    fill: '#ffffff'
  }
];

const edgeSeed = [
  { id: 'edge-inbox-model', from: 'inbox', to: 'model', color: '#2f80ed' },
  { id: 'edge-model-canvas', from: 'model', to: 'canvas', color: '#20b486' },
  { id: 'edge-model-review', from: 'model', to: 'review', color: '#d99a24' },
  { id: 'edge-review-canvas', from: 'review', to: 'canvas', color: '#e85d75' }
];

let leafer = null;
let world = null;
let gridGroup = null;
let edgeGroup = null;
let nodeGroup = null;
let refs = {};

const state = {
  selectedId: 'model',
  panX: 480,
  panY: 300,
  zoom: 1,
  renderCount: 0,
  moveStep: 0,
  traceStep: 0,
  nodeSerial: 0,
  lastAction: 'ready',
  nodes: new Map(),
  edges: []
};

function resetState() {
  state.selectedId = 'model';
  state.panX = 480;
  state.panY = 300;
  state.zoom = 1;
  state.renderCount = 0;
  state.moveStep = 0;
  state.traceStep = 0;
  state.nodeSerial = 0;
  state.lastAction = 'ready';
  state.nodes = new Map();
  state.edges = [];
}

function setShellStyles() {
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
    overflow: 'hidden'
  });
}

function installStyles() {
  let style = document.getElementById('leafer-infinite-canvas-styles');
  if (!style) {
    style = document.createElement('style');
    style.id = 'leafer-infinite-canvas-styles';
    document.head.appendChild(style);
  }

  style.textContent = `
    * { box-sizing: border-box; }
    body {
      color: #182234;
      background: #d9e3ee;
      font-family: Segoe UI, Arial, sans-serif;
    }
    button { font: inherit; }
    #infinite-canvas-root {
      width: 100%;
      height: 100%;
      display: grid;
      grid-template-columns: 278px minmax(0, 1fr);
      gap: 12px;
      padding: 12px;
      overflow: hidden;
    }
    #inspector-pane,
    #canvas-workspace {
      min-height: 0;
      border: 1px solid #c4d1df;
      border-radius: 8px;
      background: #ffffff;
      overflow: hidden;
    }
    #inspector-pane {
      display: grid;
      grid-template-rows: auto auto minmax(0, 1fr) auto;
    }
    .pane-head {
      padding: 14px;
      border-bottom: 1px solid #e2e8f0;
    }
    .eyebrow {
      margin: 0 0 6px;
      color: #47627f;
      font-size: 11px;
      font-weight: 800;
      letter-spacing: 0;
      text-transform: uppercase;
    }
    h1 {
      margin: 0;
      font-size: 19px;
      line-height: 24px;
      letter-spacing: 0;
    }
    .status-line {
      margin-top: 10px;
      min-height: 34px;
      display: flex;
      align-items: center;
      padding: 8px 10px;
      border: 1px solid #d7e0ea;
      border-radius: 6px;
      background: #f8fafc;
      color: #31465c;
      font-size: 13px;
      line-height: 18px;
    }
    .control-section {
      padding: 12px 14px;
      border-bottom: 1px solid #e2e8f0;
    }
    .section-title {
      margin: 0 0 8px;
      color: #65758a;
      font-size: 12px;
      font-weight: 800;
      letter-spacing: 0;
      text-transform: uppercase;
    }
    .button-grid {
      display: grid;
      grid-template-columns: repeat(2, minmax(0, 1fr));
      gap: 7px;
    }
    .wide-button {
      grid-column: 1 / -1;
    }
    .tool-button {
      min-height: 34px;
      padding: 6px 8px;
      border: 1px solid #c6d0dc;
      border-radius: 6px;
      background: #fbfcfe;
      color: #25364a;
      cursor: pointer;
      font-size: 12px;
      font-weight: 700;
      line-height: 18px;
    }
    .tool-button.primary {
      border-color: #355f88;
      background: #355f88;
      color: #ffffff;
    }
    .tool-button.warn {
      border-color: #b9c4d1;
      background: #ffffff;
      color: #3f5268;
    }
    #readout-list {
      min-height: 0;
      overflow: auto;
      padding: 10px 14px;
      display: grid;
      align-content: start;
      gap: 7px;
      background: #fbfcfe;
    }
    .readout-row {
      min-height: 34px;
      display: grid;
      grid-template-columns: 74px minmax(0, 1fr);
      gap: 8px;
      align-items: center;
      padding: 7px 8px;
      border: 1px solid #e0e7ef;
      border-radius: 6px;
      background: #ffffff;
      font-size: 12px;
      line-height: 17px;
    }
    .readout-key {
      color: #65758a;
      font-weight: 800;
    }
    .readout-value {
      color: #25364a;
      overflow-wrap: anywhere;
    }
    .vendor-note {
      min-height: 32px;
      padding: 8px 14px;
      border-top: 1px solid #e2e8f0;
      color: #7b8795;
      font-size: 11px;
      line-height: 16px;
      background: #eef3f8;
    }
    #canvas-workspace {
      display: grid;
      grid-template-rows: 48px minmax(0, 1fr);
      background: #e4ebf3;
    }
    #canvas-toolbar {
      display: flex;
      align-items: center;
      gap: 10px;
      min-width: 0;
      padding: 10px 12px;
      border-bottom: 1px solid #e0e7ef;
      background: #ffffff;
    }
    .toolbar-title {
      min-width: 156px;
      color: #25364a;
      font-size: 14px;
      font-weight: 800;
      line-height: 20px;
    }
    .toolbar-stat {
      min-width: 96px;
      min-height: 28px;
      display: inline-flex;
      align-items: center;
      justify-content: center;
      border: 1px solid #dce4ee;
      border-radius: 6px;
      background: #fbfcfe;
      color: #52657a;
      font-size: 12px;
      font-weight: 700;
    }
    #stage-shell {
      min-width: 0;
      min-height: 0;
      display: flex;
      align-items: center;
      justify-content: center;
      padding: 12px;
      overflow: hidden;
    }
    #infinite-stage-frame {
      width: ${STAGE.width}px;
      height: ${STAGE.height}px;
      border: 1px solid #8fa2b8;
      border-radius: 8px;
      background: #d9e4ef;
      overflow: hidden;
    }
    #infinite-stage-frame canvas {
      display: block;
    }
  `;
}

function el(tag, props = {}, children = []) {
  const node = document.createElement(tag);
  for (const [key, value] of Object.entries(props)) {
    if (key === 'text') {
      node.textContent = value;
    } else if (key === 'className') {
      node.className = value;
    } else if (key === 'dataset') {
      for (const [dataKey, dataValue] of Object.entries(value)) {
        node.dataset[dataKey] = dataValue;
      }
    } else if (key.startsWith('on') && typeof value === 'function') {
      node.addEventListener(key.slice(2), value);
    } else if (value !== false && value != null) {
      node.setAttribute(key, value === true ? '' : String(value));
    }
  }
  for (const child of children) node.appendChild(child);
  return node;
}

function button(id, text, onClick, className = '') {
  return el('button', {
    id,
    className: `tool-button ${className}`.trim(),
    type: 'button',
    text,
    onclick: onClick
  });
}

function readout(id, key, value) {
  const valueNode = el('span', {
    id,
    className: 'readout-value',
    text: value
  });
  return el('div', { className: 'readout-row' }, [
    el('span', { className: 'readout-key', text: key }),
    valueNode
  ]);
}

function buildDom() {
  const root = el('main', { id: 'infinite-canvas-root' }, [
    el('aside', { id: 'inspector-pane' }, [
      el('header', { className: 'pane-head' }, [
        el('p', { className: 'eyebrow', text: 'Leafer graph' }),
        el('h1', { text: 'Infinite Canvas' }),
        el('div', { id: 'canvas-status', className: 'status-line', text: 'ready' })
      ]),
      el('section', { className: 'control-section' }, [
        el('p', { className: 'section-title', text: 'View' }),
        el('div', { className: 'button-grid' }, [
          button('zoom-in-button', 'Zoom +', zoomIn),
          button('zoom-out-button', 'Zoom -', zoomOut),
          button('pan-left-button', 'Pan left', () => panBy(-92, 0)),
          button('pan-right-button', 'Pan right', () => panBy(92, 0)),
          button('center-view-button', 'Center', centerView, 'wide-button')
        ])
      ]),
      el('section', { id: 'readout-list' }, [
        readout('selection-readout', 'Select', 'Model'),
        readout('view-readout', 'View', '100%, 480,300'),
        readout('node-count-readout', 'Nodes', '4'),
        readout('edge-count-readout', 'Links', '4'),
        readout('render-readout', 'Render', '0'),
        readout('last-action-readout', 'Action', 'ready')
      ]),
      el('div', { id: 'vendor-readout', className: 'vendor-note', text: 'vendor: leafer_ui_showcase/js/leafer-ui/web.module.min.js' })
    ]),
    el('section', { id: 'canvas-workspace' }, [
      el('div', { id: 'canvas-toolbar' }, [
        el('div', { className: 'toolbar-title', text: 'Node canvas' }),
        el('div', { id: 'toolbar-node-stat', className: 'toolbar-stat', text: '4 nodes' }),
        el('div', { id: 'toolbar-link-stat', className: 'toolbar-stat', text: '4 links' }),
        button('move-node-button', 'Move node', moveSelectedNode, 'primary'),
        button('add-node-button', 'Add node', addIdeaNode),
        button('trace-links-button', 'Trace links', traceLinks, 'warn')
      ]),
      el('div', { id: 'stage-shell' }, [
        el('div', { id: 'infinite-stage-frame' })
      ])
    ])
  ]);

  document.body.textContent = '';
  document.body.appendChild(root);

  refs = {
    status: document.getElementById('canvas-status'),
    stageFrame: document.getElementById('infinite-stage-frame'),
    selection: document.getElementById('selection-readout'),
    view: document.getElementById('view-readout'),
    nodeCount: document.getElementById('node-count-readout'),
    edgeCount: document.getElementById('edge-count-readout'),
    render: document.getElementById('render-readout'),
    lastAction: document.getElementById('last-action-readout'),
    toolbarNodeStat: document.getElementById('toolbar-node-stat'),
    toolbarLinkStat: document.getElementById('toolbar-link-stat')
  };
}

function assertLeaferSurface() {
  const constructors = { Leafer, Rect, Text, Group, Path, Line, Ellipse };
  for (const [name, value] of Object.entries(constructors)) {
    if (typeof value !== 'function') throw new Error(`Leafer export missing: ${name}`);
  }
  if (typeof PointerEvent !== 'function' || typeof DragEvent !== 'function') {
    throw new Error('Leafer pointer/drag events missing');
  }
  if (typeof PointerEvent.CLICK !== 'string' || typeof DragEvent.DRAG !== 'string') {
    throw new Error('Leafer event constants missing');
  }
  if (typeof requestAnimationFrame !== 'function') {
    throw new Error('requestAnimationFrame missing');
  }
}

function addGrid() {
  gridGroup = new Group({ x: 0, y: 0, zIndex: 0 });
  const extent = GRID.extent;

  for (let value = -extent; value <= extent; value += GRID.minor) {
    const major = value % GRID.major === 0;
    const stroke = value === 0 ? '#95a7bb' : (major ? '#adbdce' : '#c5d1de');
    const width = value === 0 ? 1.4 : 1;

    gridGroup.add(new Path({
      path: `M${-extent} ${value} L${extent} ${value}`,
      stroke,
      strokeWidth: width,
      opacity: value === 0 ? 0.9 : (major ? 0.78 : 0.62),
      hittable: false
    }));
    gridGroup.add(new Path({
      path: `M${value} ${-extent} L${value} ${extent}`,
      stroke,
      strokeWidth: width,
      opacity: value === 0 ? 0.9 : (major ? 0.78 : 0.62),
      hittable: false
    }));
  }

  world.add(gridGroup);
}

function endpoint(node, side) {
  const x = node.x + node.width / 2;
  const y = node.y + node.height / 2;
  if (side === 'left') return { x: node.x, y };
  if (side === 'right') return { x: node.x + node.width, y };
  if (side === 'top') return { x, y: node.y };
  return { x, y: node.y + node.height };
}

function clamp(value, min, max) {
  return Math.max(min, Math.min(max, value));
}

function edgePath(from, to) {
  const fromCenter = {
    x: from.x + from.width / 2,
    y: from.y + from.height / 2
  };
  const toCenter = {
    x: to.x + to.width / 2,
    y: to.y + to.height / 2
  };
  const dx = toCenter.x - fromCenter.x;
  const dy = toCenter.y - fromCenter.y;
  const verticalRoute = Math.abs(dy) > Math.abs(dx) * 0.9;
  const startSide = verticalRoute ? (dy > 0 ? 'bottom' : 'top') : (dx > 0 ? 'right' : 'left');
  const endSide = verticalRoute ? (dy > 0 ? 'top' : 'bottom') : (dx > 0 ? 'left' : 'right');
  const start = endpoint(from, startSide);
  const end = endpoint(to, endSide);

  if (verticalRoute) {
    const distance = clamp(Math.abs(end.y - start.y) * 0.45, 18, 176);
    const sy = startSide === 'bottom' ? start.y + distance : start.y - distance;
    const ey = endSide === 'top' ? end.y - distance : end.y + distance;
    return `M${start.x} ${start.y} C${start.x} ${sy} ${end.x} ${ey} ${end.x} ${end.y}`;
  }

  const distance = clamp(Math.abs(end.x - start.x) * 0.42, 24, 188);
  const sx = startSide === 'right' ? start.x + distance : start.x - distance;
  const ex = endSide === 'left' ? end.x - distance : end.x + distance;
  return `M${start.x} ${start.y} C${sx} ${start.y} ${ex} ${end.y} ${end.x} ${end.y}`;
}

function updateEdges() {
  for (const edge of state.edges) {
    const from = state.nodes.get(edge.from);
    const to = state.nodes.get(edge.to);
    if (from && to) edge.path.path = edgePath(from, to);
  }
}

function createNode(spec) {
  const node = {
    ...spec,
    group: null,
    body: null,
    shadow: null,
    titleText: null
  };

  const group = new Group({
    x: spec.x,
    y: spec.y,
    draggable: true,
    zIndex: 20
  });
  const shadow = new Rect({
    x: 4,
    y: 7,
    width: spec.width,
    height: spec.height,
    fill: 'rgba(31, 45, 61, 0.16)',
    cornerRadius: 9,
    hittable: false
  });
  const body = new Rect({
    x: 0,
    y: 0,
    width: spec.width,
    height: spec.height,
    fill: spec.fill,
    stroke: '#8fa0b2',
    strokeWidth: 1.8,
    cornerRadius: 8
  });
  const cap = new Rect({
    x: 14,
    y: 10,
    width: spec.width - 28,
    height: 4,
    fill: spec.color,
    cornerRadius: 2,
    hittable: false
  });
  const title = new Text({
    x: 16,
    y: 24,
    width: spec.width - 34,
    text: spec.title,
    fill: '#1f2d3d',
    fontSize: 17,
    fontWeight: '700',
    hittable: false
  });
  const label = new Text({
    x: 16,
    y: 53,
    width: spec.width - 34,
    text: spec.label,
    fill: '#65758a',
    fontSize: 13,
    fontWeight: '500',
    hittable: false
  });
  const inputPort = new Ellipse({
    x: -5,
    y: spec.height / 2 - 5,
    width: 10,
    height: 10,
    fill: '#f8fafc',
    stroke: '#b5c3d2',
    strokeWidth: 2,
    hittable: false
  });
  const outputPort = new Ellipse({
    x: spec.width - 5,
    y: spec.height / 2 - 5,
    width: 10,
    height: 10,
    fill: spec.color,
    stroke: '#ffffff',
    strokeWidth: 2,
    hittable: false
  });

  group.add([shadow, body, cap, title, label, inputPort, outputPort]);
  group.on(PointerEvent.CLICK, () => selectNode(spec.id, `selected ${spec.title}`));
  group.on(PointerEvent.TAP, () => selectNode(spec.id, `selected ${spec.title}`));
  group.on(DragEvent.DRAG, () => {
    node.x = group.x;
    node.y = group.y;
    selectNode(spec.id, `dragged ${spec.title}`);
    updateEdges();
  });
  group.on(DragEvent.END, () => {
    node.x = group.x;
    node.y = group.y;
    updateEdges();
    updateReadouts(`dropped ${spec.title}`);
  });

  node.group = group;
  node.body = body;
  node.shadow = shadow;
  node.titleText = title;
  nodeGroup.add(group);
  state.nodes.set(spec.id, node);
  return node;
}

function createEdge(spec) {
  const from = state.nodes.get(spec.from);
  const to = state.nodes.get(spec.to);
  if (!from || !to) throw new Error(`edge ${spec.id} references missing node`);

  const line = new Path({
    path: edgePath(from, to),
    stroke: '#6f8299',
    strokeWidth: 3.2,
    strokeCap: 'round',
    opacity: 0.82,
    hittable: false
  });
  edgeGroup.add(line);
  const edge = { ...spec, path: line };
  state.edges.push(edge);
  return edge;
}

function buildStage() {
  assertLeaferSurface();

  if (leafer && typeof leafer.destroy === 'function') leafer.destroy();
  resetState();

  leafer = new Leafer({
    view: refs.stageFrame,
    width: STAGE.width,
    height: STAGE.height,
    fill: '#d9e4ef'
  });

  world = new Group({
    x: state.panX,
    y: state.panY,
    scaleX: state.zoom,
    scaleY: state.zoom
  });
  edgeGroup = new Group({ x: 0, y: 0, zIndex: 10 });
  nodeGroup = new Group({ x: 0, y: 0, zIndex: 20 });

  addGrid();
  world.add(edgeGroup);
  world.add(nodeGroup);
  leafer.add(world);

  for (const spec of nodeSeed) createNode(spec);
  for (const edge of edgeSeed) createEdge(edge);

  leafer.on(RenderEvent.END, () => {
    state.renderCount += 1;
    if (refs.render) refs.render.textContent = String(state.renderCount);
  });

  applyView('ready');
  selectNode(state.selectedId, 'ready');
}

function applyView(action) {
  if (!world) return;
  world.x = state.panX;
  world.y = state.panY;
  world.scaleX = state.zoom;
  world.scaleY = state.zoom;
  updateReadouts(action || state.lastAction);
}

function updateReadouts(action) {
  if (action) state.lastAction = action;
  const selected = state.nodes.get(state.selectedId);
  if (refs.status) {
    refs.status.textContent = `${state.nodes.size} nodes, ${state.edges.length} links`;
  }
  if (refs.selection) {
    refs.selection.textContent = selected ? `${selected.title} (${Math.round(selected.x)},${Math.round(selected.y)})` : 'none';
  }
  if (refs.view) {
    refs.view.textContent = `${Math.round(state.zoom * 100)}%, ${Math.round(state.panX)},${Math.round(state.panY)}`;
  }
  if (refs.nodeCount) refs.nodeCount.textContent = String(state.nodes.size);
  if (refs.edgeCount) refs.edgeCount.textContent = String(state.edges.length);
  if (refs.lastAction) refs.lastAction.textContent = state.lastAction;
  if (refs.toolbarNodeStat) refs.toolbarNodeStat.textContent = `${state.nodes.size} nodes`;
  if (refs.toolbarLinkStat) refs.toolbarLinkStat.textContent = `${state.edges.length} links`;
}

function selectNode(id, action) {
  if (!state.nodes.has(id)) return;
  state.selectedId = id;
  for (const node of state.nodes.values()) {
    const selected = node.id === id;
    node.body.stroke = selected ? '#6f8299' : '#8fa0b2';
    node.body.strokeWidth = selected ? 2.2 : 1.8;
    node.shadow.fill = selected ? 'rgba(31, 45, 61, 0.21)' : 'rgba(31, 45, 61, 0.16)';
  }
  updateReadouts(action);
}

function zoomIn() {
  state.zoom = Math.min(1.5, Number((state.zoom + 0.12).toFixed(2)));
  applyView('zoomed in');
}

function zoomOut() {
  state.zoom = Math.max(0.68, Number((state.zoom - 0.12).toFixed(2)));
  applyView('zoomed out');
}

function panBy(x, y) {
  state.panX += x;
  state.panY += y;
  applyView(x < 0 ? 'panned left' : 'panned right');
}

function centerView() {
  state.panX = 480;
  state.panY = 300;
  state.zoom = 1;
  applyView('centered view');
}

function moveSelectedNode() {
  const selected = state.nodes.get(state.selectedId);
  if (!selected) return;
  state.moveStep += 1;
  const dx = state.moveStep % 2 === 0 ? -78 : 78;
  const dy = state.moveStep % 3 === 0 ? -42 : 34;
  selected.x += dx;
  selected.y += dy;
  selected.group.x = selected.x;
  selected.group.y = selected.y;
  updateEdges();
  selectNode(selected.id, `moved ${selected.title}`);
}

function addIdeaNode() {
  state.nodeSerial += 1;
  const serial = state.nodeSerial;
  const selected = state.nodes.get(state.selectedId) || state.nodes.get('model');
  const baseX = selected ? selected.x + selected.width + 118 : 116;
  const baseY = selected ? selected.y + 30 + serial * 18 : 122;
  const id = `idea-${serial}`;
  const node = createNode({
    id,
    title: `Idea ${serial}`,
    label: 'branch',
    x: baseX,
    y: baseY,
    width: 168,
    height: 88,
    color: serial % 2 === 0 ? '#6f65a8' : '#4f84a8',
    fill: '#ffffff'
  });
  createEdge({
    id: `edge-${state.selectedId}-${id}`,
    from: state.selectedId,
    to: id,
    color: node.color
  });
  updateEdges();
  selectNode(id, `added ${node.title}`);
}

function traceLinks() {
  state.traceStep += 1;
  const step = state.traceStep;
  updateReadouts('trace scheduled');
  requestAnimationFrame(() => {
    const active = step % 2 === 1;
    for (const edge of state.edges) {
      edge.path.stroke = active ? edge.color : '#6f8299';
      edge.path.strokeWidth = active ? 4.5 : 3.2;
      edge.path.opacity = active ? 0.96 : 0.82;
    }
    updateReadouts(`trace ${step}`);
  });
}

function init() {
  if (globalThis.__leaferInfiniteCanvasDestroy) {
    globalThis.__leaferInfiniteCanvasDestroy();
  }

  setShellStyles();
  installStyles();
  buildDom();
  buildStage();

  globalThis.__leaferInfiniteCanvasDestroy = () => {
    if (leafer && typeof leafer.destroy === 'function') {
      leafer.destroy();
      leafer = null;
    }
  };

  console.log('leafer infinite canvas ready');
}

init();
