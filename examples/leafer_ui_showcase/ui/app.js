import {
  Leafer,
  Rect,
  Text,
  Group,
  Ellipse,
  Line,
  Path,
  Polygon,
  Star,
  Canvas as LeaferCanvas,
  Pen,
  Image as LeaferImage,
  PointerEvent,
  PointerButton,
  DragEvent,
  RenderEvent
} from '../js/leafer-ui/web.module.min.js';

const STAGE = { width: 620, height: 430 };

const modeMeta = {
  compose: {
    label: 'Compose',
    accent: '#f59e0b',
    banner: '#17324d',
    caption: 'compose mode'
  },
  inspect: {
    label: 'Inspect',
    accent: '#0f766e',
    banner: '#0f3f3a',
    caption: 'inspect mode'
  },
  motion: {
    label: 'Motion',
    accent: '#b45309',
    banner: '#5b3512',
    caption: 'motion mode'
  }
};

const swatches = [
  { id: 'blue', name: 'Blue', fill: '#2f80ed' },
  { id: 'green', name: 'Green', fill: '#20b486' },
  { id: 'coral', name: 'Coral', fill: '#e85d75' },
  { id: 'gold', name: 'Gold', fill: '#d99a24' }
];

const initialShapes = [
  { id: 'brief', name: 'Brief', detail: 'canvas card', x: 54, y: 94, width: 190, height: 112, fill: '#2f80ed' },
  { id: 'flow', name: 'Flow', detail: 'state layer', x: 276, y: 120, width: 142, height: 96, fill: '#20b486' },
  { id: 'signal', name: 'Signal', detail: 'event panel', x: 450, y: 82, width: 118, height: 150, fill: '#24364f' }
];

const shuffleSlots = [
  { x: 52, y: 88, width: 176, height: 110 },
  { x: 254, y: 72, width: 150, height: 132 },
  { x: 430, y: 108, width: 136, height: 92 },
  { x: 88, y: 252, width: 140, height: 78 },
  { x: 264, y: 248, width: 160, height: 84 },
  { x: 456, y: 242, width: 112, height: 86 }
];

const imageFixtureUrl = 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAQAAAAECAYAAACp8Z5+AAAAAXNSR0IArs4c6QAAAARnQU1BAACxjwv8YQUAAAAJcEhZcwAADsMAAA7DAcdvqGQAAAAYSURBVBhXY9BvePsfhH+dEQVjkgVE/wMAvCsrqWVhawoAAAAASUVORK5CYII=';
const docsShapeNames = ['Rect', 'Text', 'Group', 'Ellipse', 'Line', 'Path', 'Polygon', 'Star', 'Canvas', 'Pen', 'Image'];

let refs = {};
let leafer = null;
let stageObjects = {};

const state = {
  mode: 'compose',
  selectedId: 'brief',
  activeColor: swatches[0].fill,
  motionStep: 0,
  customSerial: 0,
  docsGroupStep: 0,
  docsGroupRemoved: false,
  eventCount: 0,
  tapCount: 0,
  clickCount: 0,
  dragCount: 0,
  rafStep: 0,
  renderCount: 0,
  animationFrame: 0,
  animationRun: 0,
  animationRunning: false,
  visibilityOn: false,
  assetStatus: 'pending',
  lastAction: 'ready',
  shapes: initialShapes.map((shape) => ({ ...shape }))
};

let animationRequestId = 0;
const animationDemo = {
  startX: 64,
  startY: 235,
  endX: 528,
  trailY: 245,
  totalFrames: 18,
  idleLabel: 'RAF animation drives Leafer node props'
};

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
  let style = document.getElementById('leafer-ui-showcase-styles');
  if (!style) {
    style = document.createElement('style');
    style.id = 'leafer-ui-showcase-styles';
    document.head.appendChild(style);
  }
  style.textContent = `
    * { box-sizing: border-box; }
    body {
      font-family: Segoe UI, Arial, sans-serif;
      color: #182235;
      background: #edf2f7;
    }
    button { font: inherit; }
    #leafer-showcase-root {
      width: 100%;
      height: 100%;
      display: grid;
      grid-template-columns: 328px minmax(0, 1fr);
      gap: 14px;
      padding: 14px;
      overflow: hidden;
    }
    .control-pane,
    .stage-pane {
      min-height: 0;
      border: 1px solid #c5d0dd;
      border-radius: 8px;
      background: #ffffff;
      overflow: hidden;
    }
    .control-pane {
      display: grid;
      grid-template-rows: auto auto auto auto minmax(0, 1fr) auto;
    }
    .panel-header {
      padding: 14px 14px 12px;
      border-bottom: 1px solid #e2e8f0;
    }
    .eyebrow {
      margin: 0 0 6px;
      color: #2563eb;
      font-size: 11px;
      font-weight: 800;
      letter-spacing: 0;
      text-transform: uppercase;
    }
    h1 {
      margin: 0;
      font-size: 18px;
      line-height: 24px;
      letter-spacing: 0;
    }
    .status-pill {
      margin-top: 10px;
      min-height: 32px;
      display: flex;
      align-items: center;
      padding: 7px 10px;
      border: 1px solid #b8c8d8;
      border-radius: 6px;
      background: #f8fbff;
      color: #184469;
      font-size: 13px;
      line-height: 18px;
    }
    .control-group {
      padding: 10px 14px;
      border-bottom: 1px solid #e2e8f0;
    }
    .group-label {
      margin: 0 0 7px;
      color: #475569;
      font-size: 12px;
      font-weight: 800;
      letter-spacing: 0;
      text-transform: uppercase;
    }
    .mode-grid {
      display: grid;
      grid-template-columns: repeat(3, minmax(0, 1fr));
      gap: 6px;
    }
    .segment-button,
    .control-button {
      border: 1px solid #aab8c7;
      border-radius: 6px;
      background: #f8fafc;
      color: #1f2937;
      cursor: pointer;
      display: flex;
      align-items: center;
      justify-content: center;
      text-align: center;
    }
    .segment-button {
      height: 34px;
      padding: 0 6px;
      font-size: 12px;
      font-weight: 700;
    }
    .segment-button.active {
      border-color: #1d4ed8;
      background: #1d4ed8;
      color: #ffffff;
    }
    .action-grid {
      display: grid;
      grid-template-columns: repeat(2, minmax(0, 1fr));
      gap: 6px;
    }
    .control-button {
      min-height: 34px;
      padding: 6px 8px;
      font-size: 12px;
      font-weight: 700;
      line-height: 18px;
    }
    .control-button.primary {
      border-color: #2563eb;
      background: #2563eb;
      color: #ffffff;
    }
    .swatch-grid {
      display: grid;
      grid-template-columns: repeat(4, minmax(0, 1fr));
      gap: 8px;
    }
    .swatch-button {
      height: 32px;
      border: 2px solid #d8e1ea;
      border-radius: 6px;
      cursor: pointer;
    }
    .swatch-button.active {
      border-color: #111827;
    }
    .readout-list {
      min-height: 0;
      overflow: auto;
      padding: 8px 14px;
      display: grid;
      align-content: start;
      gap: 6px;
      background: #fbfdff;
    }
    .readout-row {
      display: grid;
      grid-template-columns: 86px minmax(0, 1fr);
      gap: 8px;
      align-items: center;
      min-height: 30px;
      padding: 6px 8px;
      border: 1px solid #d8e1ea;
      border-radius: 6px;
      background: #ffffff;
      font-size: 12px;
      line-height: 17px;
    }
    .readout-key {
      color: #64748b;
      font-weight: 800;
    }
    .readout-value {
      min-width: 0;
      color: #172033;
      font-weight: 700;
      white-space: nowrap;
      overflow: hidden;
      text-overflow: ellipsis;
    }
    .runtime-strip {
      min-height: 40px;
      display: grid;
      gap: 3px;
      padding: 7px 14px;
      border-top: 1px solid #e2e8f0;
      background: #f8fafc;
      color: #475569;
      font-size: 11px;
      line-height: 15px;
    }
    .stage-pane {
      display: grid;
      grid-template-rows: auto minmax(0, 1fr) 58px;
    }
    .stage-header {
      min-height: 62px;
      display: flex;
      align-items: center;
      justify-content: space-between;
      gap: 14px;
      padding: 12px 16px;
      border-bottom: 1px solid #e2e8f0;
    }
    .stage-title {
      min-width: 0;
      display: grid;
      gap: 3px;
    }
    .stage-title strong {
      font-size: 17px;
      line-height: 22px;
    }
    .stage-title span {
      color: #64748b;
      font-size: 12px;
      line-height: 17px;
    }
    .stage-badge {
      min-width: 148px;
      padding: 8px 10px;
      border: 1px solid #b8c8d8;
      border-radius: 6px;
      background: #f8fbff;
      color: #184469;
      font-size: 12px;
      font-weight: 800;
      line-height: 17px;
      text-align: center;
    }
    #leafer-stage-host {
      min-width: 0;
      min-height: 0;
      display: flex;
      align-items: center;
      justify-content: center;
      padding: 18px;
      overflow: hidden;
      background: #eef4f9;
    }
    #leafer-stage-frame {
      width: 620px;
      height: 430px;
      position: relative;
      overflow: hidden;
      border: 1px solid #9fb2c8;
      border-radius: 8px;
      background: #f8fbff;
    }
    .stage-footer {
      display: grid;
      grid-template-columns: repeat(3, minmax(0, 1fr));
      gap: 10px;
      align-items: center;
      padding: 10px 14px;
      border-top: 1px solid #e2e8f0;
      background: #ffffff;
    }
    .footer-chip {
      min-width: 0;
      padding: 8px 10px;
      border: 1px solid #d8e1ea;
      border-radius: 6px;
      background: #fbfdff;
      font-size: 12px;
      font-weight: 700;
      line-height: 17px;
      white-space: nowrap;
      overflow: hidden;
      text-overflow: ellipsis;
    }
  `;
}

function el(tagName, attrs = {}, children = []) {
  const node = document.createElement(tagName);
  for (const [key, value] of Object.entries(attrs)) {
    if (key === 'text') node.textContent = value;
    else if (key === 'className') node.className = value;
    else if (key === 'style') Object.assign(node.style, value);
    else if (key.startsWith('on') && typeof value === 'function') {
      node.addEventListener(key.slice(2).toLowerCase(), value);
    } else if (value !== undefined && value !== null) {
      node.setAttribute(key, String(value));
    }
  }
  for (const child of children) {
    if (typeof child === 'string') node.appendChild(document.createTextNode(child));
    else if (child) node.appendChild(child);
  }
  return node;
}

function readout(id, key, value) {
  const readoutIds = {
    sceneCount: 'scene-count-readout',
    shapeList: 'shape-list-readout',
    selection: 'selection-readout',
    mode: 'mode-readout',
    motion: 'motion-readout',
    color: 'color-readout',
    apiSurface: 'api-shapes-readout',
    groupState: 'group-state-readout',
    canvasEvent: 'canvas-event-readout',
    dragState: 'drag-state-readout',
    rafState: 'raf-readout',
    animationState: 'animation-readout',
    assetState: 'asset-readout',
    lastAction: 'last-action-readout'
  };
  const valueNode = el('span', {
    id: readoutIds[id] || `${id}-readout`,
    className: 'readout-value',
    text: value
  });
  refs[id] = valueNode;
  return el('div', { className: 'readout-row' }, [
    el('span', { className: 'readout-key', text: key }),
    valueNode
  ]);
}

function buildDom() {
  const root = el('main', { id: 'leafer-showcase-root' });

  const modeButtons = Object.keys(modeMeta).map((mode) => {
    const button = el('button', {
      id: `mode-${mode}`,
      className: 'segment-button',
      type: 'button',
      text: modeMeta[mode].label,
      onclick: () => setMode(mode)
    });
    refs[`mode-${mode}`] = button;
    return button;
  });

  const swatchButtons = swatches.map((swatch) => {
    const button = el('button', {
      id: `palette-${swatch.id}`,
      className: 'swatch-button',
      type: 'button',
      title: swatch.name,
      style: { background: swatch.fill },
      onclick: () => setActiveColor(swatch.fill, swatch.name)
    });
    refs[`palette-${swatch.id}`] = button;
    return button;
  });

  refs.status = el('div', {
    id: 'leafer-showcase-status',
    className: 'status-pill',
    text: 'initializing'
  });

  refs.stageBadge = el('div', {
    id: 'leafer-stage-badge',
    className: 'stage-badge',
    text: 'initializing'
  });

  refs.stageCaption = el('span', {
    id: 'leafer-stage-caption',
    text: 'Leafer scene will mount here.'
  });

  refs.stageHost = el('div', { id: 'leafer-stage-host' }, [
    el('div', { id: 'leafer-stage-frame' })
  ]);
  refs.stageFrame = refs.stageHost.firstChild;

  const controlPane = el('aside', { id: 'leafer-control-pane', className: 'control-pane' }, [
    el('section', { className: 'panel-header' }, [
      el('p', { className: 'eyebrow', text: 'npm canvas package' }),
      el('h1', { text: 'Leafer UI Playground' }),
      refs.status
    ]),
    el('section', { className: 'control-group' }, [
      el('p', { className: 'group-label', text: 'Mode' }),
      el('div', { className: 'mode-grid' }, modeButtons)
    ]),
    el('section', { className: 'control-group' }, [
      el('p', { className: 'group-label', text: 'Scene controls' }),
      el('div', { className: 'action-grid' }, [
        el('button', {
          id: 'add-shape-button',
          className: 'control-button primary',
          type: 'button',
          text: 'Add shape',
          onclick: addShape
        }),
        el('button', {
          id: 'select-next-button',
          className: 'control-button',
          type: 'button',
          text: 'Select next',
          onclick: selectNextShape
        }),
        el('button', {
          id: 'shuffle-scene-button',
          className: 'control-button',
          type: 'button',
          text: 'Shuffle',
          onclick: shuffleScene
        }),
        el('button', {
          id: 'step-motion-button',
          className: 'control-button',
          type: 'button',
          text: 'Step motion',
          onclick: stepMotion
        }),
        el('button', {
          id: 'clear-selection-button',
          className: 'control-button',
          type: 'button',
          text: 'Clear selection',
          onclick: clearSelection
        }),
        el('button', {
          id: 'simulate-canvas-tap-button',
          className: 'control-button',
          type: 'button',
          text: 'Canvas tap',
          onclick: simulateCanvasTap
        }),
        el('button', {
          id: 'simulate-drag-button',
          className: 'control-button',
          type: 'button',
          text: 'Drag node',
          onclick: simulateDrag
        }),
        el('button', {
          id: 'reorder-group-button',
          className: 'control-button',
          type: 'button',
          text: 'Reorder group',
          onclick: reorderDocsGroup
        }),
        el('button', {
          id: 'remove-group-child-button',
          className: 'control-button',
          type: 'button',
          text: 'Remove child',
          onclick: toggleDocsChild
        }),
        el('button', {
          id: 'raf-step-button',
          className: 'control-button',
          type: 'button',
          text: 'RAF frame',
          onclick: stepRafFrame
        }),
        el('button', {
          id: 'play-animation-button',
          className: 'control-button',
          type: 'button',
          text: 'Play animation',
          onclick: playAnimationDemo
        }),
        el('button', {
          id: 'toggle-visible-button',
          className: 'control-button',
          type: 'button',
          text: 'Visible node',
          onclick: toggleVisibilityNode
        })
      ])
    ]),
    el('section', { className: 'control-group' }, [
      el('p', { className: 'group-label', text: 'Fill swatches' }),
      el('div', { className: 'swatch-grid' }, swatchButtons)
    ]),
    el('section', { id: 'leafer-readouts', className: 'readout-list' }, [
      readout('sceneCount', 'Objects', '0 objects'),
      readout('shapeList', 'Shapes', 'none'),
      readout('selection', 'Selection', 'none'),
      readout('mode', 'Mode', 'compose'),
      readout('motion', 'Motion', 'step 0'),
      readout('color', 'Fill', 'Blue'),
      readout('apiSurface', 'API', 'pending'),
      readout('groupState', 'Group', 'pending'),
      readout('canvasEvent', 'Events', 'tap 0 click 0'),
      readout('dragState', 'Drag', '0 drags'),
      readout('rafState', 'RAF', 'step 0'),
      readout('animationState', 'Animation', 'idle frame 0'),
      readout('assetState', 'Assets', 'pending'),
      readout('lastAction', 'Action', 'ready')
    ]),
    el('section', { id: 'leafer-runtime-strip', className: 'runtime-strip' }, [
      el('div', { id: 'leafer-vendor-path', text: 'vendor: js/leafer-ui/web.module.min.js' }),
      el('div', { id: 'leafer-runtime-checks', text: 'runtime checks pending' })
    ])
  ]);

  const stagePane = el('section', { id: 'leafer-stage-pane', className: 'stage-pane' }, [
    el('header', { className: 'stage-header' }, [
      el('div', { className: 'stage-title' }, [
        el('strong', { text: 'Live Leafer stage' }),
        refs.stageCaption
      ]),
      refs.stageBadge
    ]),
    refs.stageHost,
    el('footer', { className: 'stage-footer' }, [
      el('div', { id: 'stage-object-chip', className: 'footer-chip', text: 'objects: 0' }),
      el('div', { id: 'stage-selection-chip', className: 'footer-chip', text: 'selection: none' }),
      el('div', { id: 'stage-action-chip', className: 'footer-chip', text: 'action: ready' })
    ])
  ]);

  refs.stageObjectChip = stagePane.querySelector('#stage-object-chip');
  refs.stageSelectionChip = stagePane.querySelector('#stage-selection-chip');
  refs.stageActionChip = stagePane.querySelector('#stage-action-chip');
  refs.runtimeChecks = controlPane.querySelector('#leafer-runtime-checks');

  root.appendChild(controlPane);
  root.appendChild(stagePane);
  document.body.appendChild(root);
}

function assertRuntimeSurface() {
  const requiredConstructors = [
    'CanvasRenderingContext2D',
    'CanvasGradient',
    'CanvasPattern',
    'ImageData',
    'Path2D',
    'Element',
    'HTMLElement',
    'HTMLCanvasElement',
    'HTMLImageElement',
    'MouseEvent',
    'PointerEvent',
    'DragEvent',
    'KeyboardEvent',
    'ResizeObserver'
  ];
  for (const name of requiredConstructors) {
    if (typeof globalThis[name] !== 'function') {
      throw new Error(`${name} missing`);
    }
  }
  if (typeof refs.stageHost.hasChildNodes !== 'function') {
    throw new Error('Node.hasChildNodes missing');
  }
}

function applyLeaferProps(node, props) {
  if (!node) return;
  if (typeof node.set === 'function') node.set(props);
  else Object.assign(node, props);
}

function addStageText(props) {
  const text = new Text(props);
  leafer.add(text);
  return text;
}

function shapeTextLayout(shape, wobble = 0, lift = 0) {
  const centerY = shape.y + lift + Math.round(shape.height / 2);
  return {
    x: shape.x + wobble,
    width: shape.width,
    labelY: centerY - 24,
    detailY: centerY + 2,
    labelHeight: 24,
    detailHeight: 18
  };
}

function createShapeObjects(shape) {
  const textLayout = shapeTextLayout(shape);
  const rect = new Rect({
    x: shape.x,
    y: shape.y,
    width: shape.width,
    height: shape.height,
    fill: shape.fill,
    stroke: '#102033',
    strokeWidth: 2,
    cornerRadius: 16
  });
  const label = new Text({
    x: textLayout.x,
    y: textLayout.labelY,
    width: textLayout.width,
    height: textLayout.labelHeight,
    text: shape.name,
    fill: '#ffffff',
    fontSize: 20,
    fontWeight: '700',
    textAlign: 'center',
    verticalAlign: 'middle'
  });
  const detail = new Text({
    x: textLayout.x,
    y: textLayout.detailY,
    width: textLayout.width,
    height: textLayout.detailHeight,
    text: shape.detail,
    fill: '#eaf2ff',
    fontSize: 13,
    textAlign: 'center',
    verticalAlign: 'middle'
  });

  leafer.add(rect);
  leafer.add(label);
  leafer.add(detail);

  for (const target of [rect, label, detail]) {
    if (target && typeof target.on === 'function') {
      target.on('click', () => selectShape(shape.id, 'canvas select'));
      target.on('tap', () => selectShape(shape.id, 'canvas select'));
    }
  }

  stageObjects[shape.id] = { rect, label, detail };
}

function updateDocsReadouts(actionLabel) {
  if (refs.apiSurface) refs.apiSurface.textContent = docsShapeNames.join(', ');
  if (refs.groupState) {
    const children = stageObjects.docsGroup && stageObjects.docsGroup.children
      ? stageObjects.docsGroup.children.length
      : 0;
    refs.groupState.textContent = `step ${state.docsGroupStep}, ${children} children, removed ${state.docsGroupRemoved ? 'yes' : 'no'}`;
  }
  if (refs.canvasEvent) refs.canvasEvent.textContent = `tap ${state.tapCount} click ${state.clickCount} events ${state.eventCount}`;
  if (refs.dragState) refs.dragState.textContent = `${state.dragCount} drags`;
  if (refs.rafState) refs.rafState.textContent = `step ${state.rafStep}, render ${state.renderCount}`;
  if (refs.animationState) {
    const label = state.animationRunning ? 'running' : (state.animationFrame > 0 ? 'complete' : 'idle');
    refs.animationState.textContent = `${label} frame ${state.animationFrame}, run ${state.animationRun}`;
  }
  if (refs.assetState) refs.assetState.textContent = state.assetStatus;
  if (actionLabel && refs.lastAction && refs.stageActionChip) {
    refs.lastAction.textContent = actionLabel;
    refs.stageActionChip.textContent = `action: ${actionLabel}`;
  }
}

function assertLeaferApiSurface() {
  const constructors = {
    Leafer,
    Rect,
    Text,
    Group,
    Ellipse,
    Line,
    Path,
    Polygon,
    Star,
    LeaferCanvas,
    Pen,
    LeaferImage
  };
  for (const [name, value] of Object.entries(constructors)) {
    if (typeof value !== 'function') throw new Error(`Leafer export missing: ${name}`);
  }
  if (typeof PointerEvent !== 'function' || typeof DragEvent !== 'function') {
    throw new Error('Leafer pointer or drag event exports missing');
  }
  if (typeof PointerButton !== 'object' || typeof PointerButton.MIDDLE !== 'number' || typeof PointerButton.RIGHT !== 'number') {
    throw new Error('Leafer pointer button constants missing');
  }
  if (typeof PointerEvent.TAP !== 'string' || typeof DragEvent.DRAG !== 'string') {
    throw new Error('Leafer event constants missing');
  }
  if (typeof RenderEvent !== 'function' || typeof RenderEvent.END !== 'string') {
    throw new Error('Leafer render event constants missing');
  }
}

function drawLeaferCanvasFixture(canvasNode, colorA, colorB) {
  const { context } = canvasNode;
  if (!context) throw new Error('Leafer Canvas context missing');
  context.clearRect(0, 0, 60, 52);
  context.fillStyle = colorA;
  context.roundRect(0, 0, 60, 52, 12);
  context.fill();
  context.fillStyle = colorB;
  context.beginPath();
  context.arc(30, 26, 14, 0, Math.PI * 2);
  context.fill();
  canvasNode.paint();
}

function buildDocsCoverage() {
  assertLeaferApiSurface();

  const group = new Group({ x: 44, y: 270 });
  const ellipse = new Ellipse({
    x: 0,
    y: 4,
    width: 58,
    height: 46,
    innerRadius: 0.35,
    fill: '#feb027',
    stroke: '#4338ca',
    strokeWidth: 4,
    opacity: 0.92
  });
  const line = new Line({
    x: 74,
    y: 27,
    width: 76,
    stroke: '#334155',
    strokeWidth: 7,
    strokeCap: 'round'
  });
  const path = new Path({
    x: 166,
    y: 4,
    path: 'M0 44 L28 0 L56 44 Z',
    fill: '#e85d75',
    stroke: '#111827',
    strokeWidth: 2,
    opacity: 0.95
  });
  const polygon = new Polygon({
    x: 246,
    y: 0,
    width: 54,
    height: 54,
    sides: 6,
    cornerRadius: 8,
    fill: '#20b486',
    stroke: '#0f766e',
    strokeWidth: 3
  });
  const star = new Star({
    x: 324,
    y: 0,
    width: 56,
    height: 56,
    innerRadius: 0.48,
    corners: 5,
    cornerRadius: 4,
    fill: '#d99a24',
    stroke: '#78350f',
    strokeWidth: 2
  });
  const hidden = new Rect({
    x: 398,
    y: 8,
    width: 50,
    height: 40,
    fill: '#7c3aed',
    stroke: '#facc15',
    strokeWidth: 2,
    cornerRadius: 8,
    visible: false,
    zIndex: 4
  });
  const docsCanvas = new LeaferCanvas({ x: 460, y: 2, width: 60, height: 52 });
  drawLeaferCanvasFixture(docsCanvas, '#6d28d9', '#facc15');

  const pen = new Pen();
  pen.setStyle({ x: 536, y: 5, fill: '#ff4b4b', windingRule: 'evenodd' });
  pen.roundRect(0, 0, 46, 42, 12).arc(23, 21, 11);

  group.add([ellipse, line, path, polygon, star, hidden, docsCanvas, pen]);
  leafer.add(group);

  const image = new LeaferImage({
    x: 542,
    y: 72,
    width: 34,
    height: 34,
    url: imageFixtureUrl,
    opacity: 1
  });
  leafer.add(image);

  const eventTarget = new Rect({
    x: 44,
    y: 326,
    width: 76,
    height: 30,
    fill: '#7c3aed',
    stroke: '#111827',
    strokeWidth: 2,
    cornerRadius: 8,
    zIndex: 20
  });
  const eventText = new Text({
    x: eventTarget.x,
    y: eventTarget.y,
    width: eventTarget.width,
    height: eventTarget.height,
    text: 'tap',
    fill: '#ffffff',
    fontSize: 12,
    fontWeight: '700',
    textAlign: 'center',
    verticalAlign: 'middle',
    hittable: false,
    zIndex: 21
  });
  leafer.add(eventTarget);
  leafer.add(eventText);

  eventTarget.on(PointerEvent.DOWN, () => {
    state.eventCount += 1;
    eventTarget.fill = '#9333ea';
    updateDocsReadouts('pointer down');
  });
  eventTarget.on(PointerEvent.TAP, () => {
    state.tapCount += 1;
    eventTarget.fill = '#c026d3';
    eventText.text = `tap ${state.tapCount}`;
    updateDocsReadouts('tap event');
  });
  eventTarget.on(PointerEvent.CLICK, () => {
    state.clickCount += 1;
    eventTarget.stroke = '#facc15';
    updateDocsReadouts('click event');
  });

  const dragTarget = new Rect({
    x: 144,
    y: 326,
    width: 78,
    height: 30,
    fill: '#0ea5e9',
    stroke: '#0f172a',
    strokeWidth: 2,
    cornerRadius: 8,
    draggable: true,
    zIndex: 20
  });
  const dragText = new Text({
    x: dragTarget.x,
    y: dragTarget.y,
    width: dragTarget.width,
    height: dragTarget.height,
    text: 'drag',
    fill: '#ffffff',
    fontSize: 12,
    fontWeight: '700',
    textAlign: 'center',
    verticalAlign: 'middle',
    hittable: false,
    zIndex: 21
  });
  leafer.add(dragTarget);
  leafer.add(dragText);

  dragTarget.on(DragEvent.START, () => {
    dragTarget.fill = '#2563eb';
    updateDocsReadouts('drag start');
  });
  dragTarget.on(DragEvent.DRAG, () => {
    state.dragCount += 1;
    dragTarget.fill = '#ef4444';
    dragText.x = dragTarget.x;
    dragText.y = dragTarget.y;
    dragText.text = `drag ${state.dragCount}`;
    updateDocsReadouts('drag event');
  });
  dragTarget.on(DragEvent.END, () => {
    dragTarget.stroke = '#facc15';
    updateDocsReadouts('drag end');
  });

  const rafRect = new Rect({
    x: 248,
    y: 326,
    width: 82,
    height: 30,
    fill: '#14b8a6',
    stroke: '#0f172a',
    strokeWidth: 2,
    cornerRadius: 8,
    zIndex: 20
  });
  leafer.add(rafRect);

  leafer.on(RenderEvent.END, () => {
    state.renderCount += 1;
    if (refs.rafState) refs.rafState.textContent = `step ${state.rafStep}, render ${state.renderCount}`;
  });

  stageObjects.docsGroup = group;
  stageObjects.docsEllipse = ellipse;
  stageObjects.docsLine = line;
  stageObjects.docsPath = path;
  stageObjects.docsPolygon = polygon;
  stageObjects.docsStar = star;
  stageObjects.docsHidden = hidden;
  stageObjects.docsCanvas = docsCanvas;
  stageObjects.docsPen = pen;
  stageObjects.docsImage = image;
  stageObjects.eventTarget = eventTarget;
  stageObjects.eventText = eventText;
  stageObjects.dragTarget = dragTarget;
  stageObjects.dragText = dragText;
  stageObjects.rafRect = rafRect;

  const probeImage = new globalThis.Image();
  probeImage.onload = () => {
    state.assetStatus = `image ${probeImage.naturalWidth}x${probeImage.naturalHeight} loaded`;
    updateDocsReadouts('image loaded');
  };
  probeImage.onerror = () => {
    throw new Error('data URL image fixture failed to load');
  };
  probeImage.src = imageFixtureUrl;

  updateDocsReadouts('docs coverage ready');
}

function buildAnimationDemo() {
  const lane = new Rect({
    x: 54,
    y: 238,
    width: 514,
    height: 22,
    fill: '#e3edf7',
    stroke: '#b3c4d6',
    strokeWidth: 1,
    cornerRadius: 11,
    zIndex: 18
  });
  const trail = new Rect({
    x: animationDemo.startX,
    y: animationDemo.trailY,
    width: 34,
    height: 8,
    fill: '#fdba74',
    opacity: 0.38,
    cornerRadius: 4,
    zIndex: 19
  });
  const puck = new Ellipse({
    x: animationDemo.startX,
    y: animationDemo.startY,
    width: 30,
    height: 30,
    fill: '#f97316',
    stroke: '#7c2d12',
    strokeWidth: 2,
    zIndex: 20
  });
  const label = new Text({
    x: lane.x,
    y: lane.y,
    width: lane.width,
    height: lane.height,
    text: animationDemo.idleLabel,
    fill: '#40576f',
    fontSize: 12,
    fontWeight: '700',
    textAlign: 'center',
    verticalAlign: 'middle',
    hittable: false,
    zIndex: 21
  });

  leafer.add(lane);
  leafer.add(trail);
  leafer.add(puck);
  leafer.add(label);

  stageObjects.animationLane = lane;
  stageObjects.animationTrail = trail;
  stageObjects.animationPuck = puck;
  stageObjects.animationLabel = label;
}

function simulateCanvasTap() {
  if (!leafer || !leafer.interaction || !stageObjects.eventTarget) {
    throw new Error('Leafer interaction surface missing for pointer simulation');
  }
  const x = stageObjects.eventTarget.x + 18;
  const y = stageObjects.eventTarget.y + 15;
  leafer.interaction.pointerDown({ x, y });
  leafer.interaction.pointerUp({ x, y });
  state.lastAction = 'simulated canvas tap';
  updateDocsReadouts(state.lastAction);
}

function simulateDrag() {
  if (!leafer || !leafer.interaction || !stageObjects.dragTarget) {
    throw new Error('Leafer interaction surface missing for drag simulation');
  }
  const startX = stageObjects.dragTarget.x + 18;
  const startY = stageObjects.dragTarget.y + 15;
  const endX = startX + 42;
  const endY = startY + 18;
  leafer.interaction.pointerDown({ x: startX, y: startY });
  leafer.interaction.pointerMove({ x: endX, y: endY });
  leafer.interaction.pointerUp({ x: endX, y: endY });
  state.lastAction = 'simulated drag';
  updateDocsReadouts(state.lastAction);
}

function reorderDocsGroup() {
  const { docsGroup, docsEllipse, docsPath, docsPolygon, docsStar } = stageObjects;
  if (!docsGroup || typeof docsGroup.addAt !== 'function' || typeof docsGroup.addBefore !== 'function' || typeof docsGroup.addAfter !== 'function') {
    throw new Error('Leafer Group reorder APIs missing');
  }
  state.docsGroupStep += 1;
  const variant = state.docsGroupStep % 3;
  if (variant === 1) {
    docsGroup.addAt(docsStar, 0);
    docsPath.fill = '#2563eb';
    docsPolygon.x = 252;
  } else if (variant === 2) {
    docsGroup.addBefore(docsPath, docsEllipse);
    docsPath.fill = '#f97316';
    docsPolygon.x = 238;
  } else {
    docsGroup.remove(docsStar);
    docsGroup.addAfter(docsStar, docsPolygon);
    if (docsGroup.children.indexOf(docsStar) !== docsGroup.children.indexOf(docsPolygon) + 1) {
      throw new Error('Leafer Group.addAfter failed to insert after target');
    }
    docsPath.fill = '#e85d75';
    docsPolygon.x = 246;
  }
  state.lastAction = `reordered group ${state.docsGroupStep}`;
  updateDocsReadouts(state.lastAction);
}

function toggleDocsChild() {
  const { docsGroup, docsHidden } = stageObjects;
  if (!docsGroup || !docsHidden || typeof docsGroup.remove !== 'function' || typeof docsGroup.addAt !== 'function') {
    throw new Error('Leafer Group remove/addAt APIs missing');
  }
  if (state.docsGroupRemoved) {
    docsGroup.addAt(docsHidden, 5);
    state.docsGroupRemoved = false;
    state.lastAction = 'restored group child';
  } else {
    docsGroup.remove(docsHidden);
    state.docsGroupRemoved = true;
    state.lastAction = 'removed group child';
  }
  updateDocsReadouts(state.lastAction);
}

function stepRafFrame() {
  const { rafRect } = stageObjects;
  if (!rafRect || typeof requestAnimationFrame !== 'function') {
    throw new Error('requestAnimationFrame or RAF target missing');
  }
  state.lastAction = 'raf scheduled';
  updateDocsReadouts(state.lastAction);
  requestAnimationFrame(() => {
    state.rafStep += 1;
    const even = state.rafStep % 2 === 0;
    rafRect.x = even ? 248 : 372;
    rafRect.width = even ? 82 : 112;
    rafRect.fill = even ? '#14b8a6' : '#f97316';
    state.lastAction = `raf frame ${state.rafStep}`;
    updateDocsReadouts(state.lastAction);
  });
}

function resetAnimationDemoVisual() {
  const { animationPuck, animationTrail, animationLabel } = stageObjects;
  if (!animationPuck || !animationTrail || !animationLabel) {
    throw new Error('Leafer animation demo nodes missing');
  }

  applyLeaferProps(animationTrail, {
    x: animationDemo.startX,
    y: animationDemo.trailY,
    width: 34,
    fill: '#fdba74',
    opacity: 0.38
  });
  applyLeaferProps(animationPuck, {
    x: animationDemo.startX,
    y: animationDemo.startY,
    width: 30,
    height: 30,
    fill: '#f97316',
    stroke: '#7c2d12',
    opacity: 1
  });
  applyLeaferProps(animationLabel, {
    text: animationDemo.idleLabel
  });
}

function applyAnimationFrame(frame, totalFrames) {
  const { animationPuck, animationTrail, animationLabel } = stageObjects;
  if (!animationPuck || !animationTrail || !animationLabel) {
    throw new Error('Leafer animation demo nodes missing');
  }

  const progress = Math.min(1, Math.max(0, frame / totalFrames));
  const legProgress = progress <= 0.5 ? progress * 2 : (1 - progress) * 2;
  const eased = legProgress < 0.5
    ? 2 * legProgress * legProgress
    : 1 - Math.pow(-2 * legProgress + 2, 2) / 2;
  const x = Math.round(animationDemo.startX + (animationDemo.endX - animationDemo.startX) * eased);
  const y = animationDemo.startY + Math.round(Math.sin(frame * 0.72) * 4);
  const hot = frame % 2 === 0;

  applyLeaferProps(animationTrail, {
    x: Math.max(animationDemo.startX, x - 38),
    y: y + 10,
    width: Math.min(44, Math.max(20, x - animationDemo.startX + 10)),
    fill: hot ? '#fdba74' : '#c4b5fd',
    opacity: 0.42
  });
  applyLeaferProps(animationPuck, {
    x,
    y,
    width: hot ? 30 : 34,
    height: hot ? 30 : 34,
    fill: hot ? '#f97316' : '#14b8a6',
    stroke: '#7c2d12',
    opacity: 1
  });
  applyLeaferProps(animationLabel, {
    text: `RAF animation frame ${frame}/${totalFrames}`
  });
}

function playAnimationDemo() {
  if (state.animationRunning) return;
  if (typeof requestAnimationFrame !== 'function') {
    throw new Error('requestAnimationFrame missing for Leafer animation demo');
  }

  const totalFrames = animationDemo.totalFrames;
  state.animationRun += 1;
  state.animationFrame = 0;
  state.animationRunning = true;
  state.lastAction = 'animation started';
  applyAnimationFrame(0, totalFrames);
  updateDocsReadouts(state.lastAction);

  const advance = () => {
    state.animationFrame += 1;
    applyAnimationFrame(state.animationFrame, totalFrames);
    if (state.animationFrame >= totalFrames) {
      state.animationRunning = false;
      state.lastAction = 'animation complete';
      animationRequestId = 0;
      resetAnimationDemoVisual();
      updateDocsReadouts(state.lastAction);
      return;
    }
    state.lastAction = `animation frame ${state.animationFrame}`;
    updateDocsReadouts(state.lastAction);
    animationRequestId = requestAnimationFrame(advance);
  };

  animationRequestId = requestAnimationFrame(advance);
}

function toggleVisibilityNode() {
  const { docsHidden } = stageObjects;
  if (!docsHidden) throw new Error('Leafer visible test node missing');
  state.visibilityOn = !state.visibilityOn;
  docsHidden.visible = state.visibilityOn;
  docsHidden.fill = state.visibilityOn ? '#7c3aed' : '#7c3aed';
  state.lastAction = state.visibilityOn ? 'visible node on' : 'visible node off';
  updateDocsReadouts(state.lastAction);
}

function buildStage() {
  leafer = new Leafer({
    view: refs.stageFrame,
    width: STAGE.width,
    height: STAGE.height,
    fill: '#f8fbff'
  });

  stageObjects.backdrop = new Rect({
    x: 24,
    y: 22,
    width: 572,
    height: 386,
    fill: '#f8fbff',
    cornerRadius: 18
  });
  leafer.add(stageObjects.backdrop);

  stageObjects.banner = new Rect({
    x: 44,
    y: 42,
    width: 532,
    height: 34,
    fill: modeMeta.compose.banner,
    cornerRadius: 10
  });
  leafer.add(stageObjects.banner);

  stageObjects.bannerText = addStageText({
    x: 60,
    y: 42,
    width: 320,
    height: 34,
    text: 'Leafer scene: compose mode',
    fill: '#ffffff',
    fontSize: 16,
    fontWeight: '700',
    textAlign: 'left',
    verticalAlign: 'middle'
  });

  stageObjects.counterText = addStageText({
    x: 432,
    y: 42,
    width: 130,
    height: 34,
    text: '3 objects',
    fill: '#dbeafe',
    fontSize: 14,
    fontWeight: '700',
    textAlign: 'center',
    verticalAlign: 'middle'
  });

  stageObjects.track = new Rect({
    x: 54,
    y: 360,
    width: 514,
    height: 32,
    fill: '#eef4fa',
    cornerRadius: 10
  });
  leafer.add(stageObjects.track);

  stageObjects.trackText = addStageText({
    x: stageObjects.track.x,
    y: stageObjects.track.y,
    width: stageObjects.track.width,
    height: stageObjects.track.height,
    text: 'DOM controls mutate these Leafer Rect and Text nodes.',
    fill: '#40576f',
    fontSize: 13,
    fontWeight: '600',
    textAlign: 'center',
    verticalAlign: 'middle'
  });

  stageObjects.selectionHalo = new Rect({
    x: 0,
    y: 0,
    width: 1,
    height: 1,
    fill: '#f59e0b',
    opacity: 0,
    cornerRadius: 20
  });
  leafer.add(stageObjects.selectionHalo);

  for (const shape of state.shapes) createShapeObjects(shape);
  buildDocsCoverage();
  buildAnimationDemo();
  updateScene('ready');
}

function getSelectedShape() {
  return state.shapes.find((shape) => shape.id === state.selectedId) || null;
}

function getColorName(fill) {
  const swatch = swatches.find((item) => item.fill === fill);
  return swatch ? swatch.name : fill;
}

function selectShape(id, action = 'selected shape') {
  if (state.shapes.some((shape) => shape.id === id)) {
    state.selectedId = id;
    state.lastAction = action;
    updateScene(action);
  }
}

function selectNextShape() {
  if (state.shapes.length < 1) return;
  const currentIndex = state.shapes.findIndex((shape) => shape.id === state.selectedId);
  const nextIndex = currentIndex < 0 ? 0 : (currentIndex + 1) % state.shapes.length;
  state.selectedId = state.shapes[nextIndex].id;
  state.lastAction = `selected ${state.shapes[nextIndex].name}`;
  updateScene(state.lastAction);
}

function clearSelection() {
  state.selectedId = null;
  state.lastAction = 'cleared selection';
  updateScene(state.lastAction);
}

function setMode(mode) {
  if (!modeMeta[mode]) return;
  state.mode = mode;
  state.lastAction = `${modeMeta[mode].label.toLowerCase()} mode`;
  if (mode === 'motion') state.motionStep += 1;
  updateScene(state.lastAction);
}

function setActiveColor(fill, name) {
  state.activeColor = fill;
  const selected = getSelectedShape();
  if (selected) selected.fill = fill;
  state.lastAction = `fill ${name}`;
  updateScene(state.lastAction);
}

function addShape() {
  state.customSerial += 1;
  const slot = shuffleSlots[(state.shapes.length + 1) % shuffleSlots.length];
  const shape = {
    id: `custom-${state.customSerial}`,
    name: `Layer ${state.customSerial}`,
    detail: 'added live',
    x: slot.x,
    y: slot.y,
    width: slot.width,
    height: slot.height,
    fill: state.activeColor
  };
  state.shapes.push(shape);
  createShapeObjects(shape);
  state.selectedId = shape.id;
  state.lastAction = `added ${shape.name}`;
  updateScene(state.lastAction);
}

function shuffleScene() {
  state.motionStep += 1;
  state.shapes.forEach((shape, index) => {
    const slot = shuffleSlots[(index + state.motionStep) % shuffleSlots.length];
    shape.x = slot.x;
    shape.y = slot.y;
    shape.width = slot.width;
    shape.height = slot.height;
  });
  state.lastAction = `shuffled scene ${state.motionStep}`;
  updateScene(state.lastAction);
}

function stepMotion() {
  state.mode = 'motion';
  state.motionStep += 1;
  state.lastAction = `motion step ${state.motionStep}`;
  updateScene(state.lastAction);
}

function updateButtonState() {
  for (const mode of Object.keys(modeMeta)) {
    refs[`mode-${mode}`].classList.toggle('active', state.mode === mode);
  }
  for (const swatch of swatches) {
    refs[`palette-${swatch.id}`].classList.toggle('active', state.activeColor === swatch.fill);
  }
}

function updateScene(actionLabel) {
  if (!leafer) return;
  const meta = modeMeta[state.mode];
  const selected = getSelectedShape();
  const selectedName = selected ? selected.name : 'none selected';
  const colorName = getColorName(state.activeColor);

  applyLeaferProps(stageObjects.banner, { fill: meta.banner });
  applyLeaferProps(stageObjects.bannerText, {
    text: `Leafer scene: ${meta.caption}`,
    fill: '#ffffff'
  });
  applyLeaferProps(stageObjects.counterText, {
    text: `${state.shapes.length} objects`
  });
  applyLeaferProps(stageObjects.trackText, {
    text: selected
      ? `${selected.name}: controls update real Leafer nodes.`
      : `No selection: ${state.shapes.length} Leafer objects remain on stage.`
  });

  if (selected) {
    const selectedIndex = state.shapes.findIndex((shape) => shape.id === selected.id);
    const haloWobble = state.mode === 'motion'
      ? Math.round(Math.sin((state.motionStep + selectedIndex) * 0.85) * 14)
      : 0;
    applyLeaferProps(stageObjects.selectionHalo, {
      x: selected.x + haloWobble - 6,
      y: selected.y - 10,
      width: selected.width + 12,
      height: selected.height + 12,
      fill: meta.accent,
      opacity: 1,
      cornerRadius: 20
    });
  } else {
    applyLeaferProps(stageObjects.selectionHalo, { opacity: 0 });
  }

  state.shapes.forEach((shape, index) => {
    const nodes = stageObjects[shape.id];
    if (!nodes) return;
    const isSelected = shape.id === state.selectedId;
    const wobble = state.mode === 'motion'
      ? Math.round(Math.sin((state.motionStep + index) * 0.85) * 14)
      : 0;
    const lift = isSelected ? -4 : 0;
    const opacity = isSelected || !state.selectedId ? 1 : 0.72;
    const textLayout = shapeTextLayout(shape, wobble, lift);

    applyLeaferProps(nodes.rect, {
      x: shape.x + wobble,
      y: shape.y + lift,
      width: shape.width,
      height: shape.height,
      fill: shape.fill,
      opacity,
      cornerRadius: isSelected ? 18 : 14
    });
    applyLeaferProps(nodes.label, {
      x: textLayout.x,
      y: textLayout.labelY,
      width: textLayout.width,
      height: textLayout.labelHeight,
      text: shape.name,
      fill: '#ffffff',
      opacity,
      textAlign: 'center',
      verticalAlign: 'middle'
    });
    applyLeaferProps(nodes.detail, {
      x: textLayout.x,
      y: textLayout.detailY,
      width: textLayout.width,
      height: textLayout.detailHeight,
      text: isSelected ? meta.label : shape.detail,
      fill: '#eaf2ff',
      opacity,
      textAlign: 'center',
      verticalAlign: 'middle'
    });
  });

  refs.status.textContent = `ready: ${meta.caption}, ${selectedName}`;
  refs.stageBadge.textContent = `${state.shapes.length} objects`;
  refs.stageCaption.textContent = `Mode ${meta.label}; selection ${selectedName}.`;
  refs.sceneCount.textContent = `${state.shapes.length} objects`;
  refs.shapeList.textContent = state.shapes.map((shape) => shape.name).join(', ');
  refs.selection.textContent = selectedName;
  refs.mode.textContent = meta.label;
  refs.motion.textContent = `step ${state.motionStep}`;
  refs.color.textContent = colorName;
  refs.lastAction.textContent = actionLabel || state.lastAction;
  refs.stageObjectChip.textContent = `objects: ${state.shapes.length}`;
  refs.stageSelectionChip.textContent = `selection: ${selectedName}`;
  refs.stageActionChip.textContent = `action: ${actionLabel || state.lastAction}`;
  updateDocsReadouts(actionLabel || state.lastAction);
  updateButtonState();
}

function installResizeGuard() {
  const observer = new ResizeObserver((entries, instance) => {
    if (instance !== observer || entries.length < 1) {
      throw new Error('ResizeObserver callback payload invalid');
    }
    observer.disconnect();
  });
  observer.observe(refs.stageHost);
}

function readTextAlignment(node) {
  if (!node) return null;
  return {
    text: node.text,
    x: node.x,
    y: node.y,
    width: node.width,
    height: node.height,
    textAlign: node.textAlign,
    verticalAlign: node.verticalAlign
  };
}

function readVisualNode(node) {
  if (!node) return null;
  return {
    x: node.x,
    y: node.y,
    width: node.width,
    height: node.height,
    fill: node.fill,
    stroke: node.stroke,
    opacity: node.opacity
  };
}

function getAlignmentState() {
  return {
    shapes: state.shapes.map((shape) => {
      const nodes = stageObjects[shape.id] || {};
      return {
        id: shape.id,
        label: readTextAlignment(nodes.label),
        detail: readTextAlignment(nodes.detail)
      };
    }),
    eventText: readTextAlignment(stageObjects.eventText),
    dragText: readTextAlignment(stageObjects.dragText),
    animationLabel: readTextAlignment(stageObjects.animationLabel),
    animationPuck: readVisualNode(stageObjects.animationPuck),
    animationTrail: readVisualNode(stageObjects.animationTrail),
    trackText: readTextAlignment(stageObjects.trackText),
    counterText: readTextAlignment(stageObjects.counterText)
  };
}

function boot() {
  setShellStyles();
  installStyles();
  buildDom();

  try {
    assertRuntimeSurface();
    installResizeGuard();
    buildStage();
    refs.runtimeChecks.textContent = 'runtime checks passed';
    globalThis.__leaferShowcase = {
      ok: true,
      version: 'interactive',
      getState: () => ({
        mode: state.mode,
        selectedId: state.selectedId,
        activeColor: state.activeColor,
        motionStep: state.motionStep,
        shapeCount: state.shapes.length,
        docsGroupStep: state.docsGroupStep,
        eventCount: state.eventCount,
        tapCount: state.tapCount,
        clickCount: state.clickCount,
        dragCount: state.dragCount,
        rafStep: state.rafStep,
        renderCount: state.renderCount,
        animationFrame: state.animationFrame,
        animationRun: state.animationRun,
        animationRunning: state.animationRunning,
        visibilityOn: state.visibilityOn,
        assetStatus: state.assetStatus,
        lastAction: state.lastAction
      }),
      getAlignmentState
    };
    console.log('leafer-ui interactive showcase ready');
  } catch (error) {
    const message = String((error && error.stack) || error);
    refs.status.textContent = `leafer-ui showcase failed: ${message}`;
    refs.stageBadge.textContent = 'failed';
    refs.runtimeChecks.textContent = 'runtime checks failed';
    globalThis.__leaferShowcase = { ok: false, message };
    console.error(message);
  }
}

boot();
