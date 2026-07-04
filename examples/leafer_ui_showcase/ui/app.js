import { Leafer, Rect, Text } from '../js/leafer-ui/web.module.min.js';

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

let refs = {};
let leafer = null;
let stageObjects = {};

const state = {
  mode: 'compose',
  selectedId: 'brief',
  activeColor: swatches[0].fill,
  motionStep: 0,
  customSerial: 0,
  lastAction: 'ready',
  shapes: initialShapes.map((shape) => ({ ...shape }))
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
  if (typeof node.forceUpdate === 'function') node.forceUpdate();
}

function addStageText(props) {
  const text = new Text(props);
  leafer.add(text);
  return text;
}

function createShapeObjects(shape) {
  const rect = new Rect({
    x: shape.x,
    y: shape.y,
    width: shape.width,
    height: shape.height,
    fill: shape.fill,
    cornerRadius: 16
  });
  const label = new Text({
    x: shape.x + 16,
    y: shape.y + 20,
    width: Math.max(72, shape.width - 28),
    text: shape.name,
    fill: '#ffffff',
    fontSize: 20,
    fontWeight: '700'
  });
  const detail = new Text({
    x: shape.x + 16,
    y: shape.y + 52,
    width: Math.max(72, shape.width - 28),
    text: shape.detail,
    fill: '#eaf2ff',
    fontSize: 13
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
    y: 49,
    width: 320,
    text: 'Leafer scene: compose mode',
    fill: '#ffffff',
    fontSize: 16,
    fontWeight: '700'
  });

  stageObjects.counterText = addStageText({
    x: 432,
    y: 50,
    width: 130,
    text: '3 objects',
    fill: '#dbeafe',
    fontSize: 14,
    fontWeight: '700'
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
    x: 78,
    y: 368,
    width: 460,
    text: 'DOM controls mutate these Leafer Rect and Text nodes.',
    fill: '#40576f',
    fontSize: 13,
    fontWeight: '600'
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
    if (state.mode === mode) refs[`mode-${mode}`].classList.add('active');
    else refs[`mode-${mode}`].classList.remove('active');
  }
  for (const swatch of swatches) {
    if (state.activeColor === swatch.fill) refs[`palette-${swatch.id}`].classList.add('active');
    else refs[`palette-${swatch.id}`].classList.remove('active');
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
      x: shape.x + wobble + 16,
      y: shape.y + lift + 18,
      width: Math.max(72, shape.width - 28),
      text: shape.name,
      fill: '#ffffff',
      opacity
    });
    applyLeaferProps(nodes.detail, {
      x: shape.x + wobble + 16,
      y: shape.y + lift + 50,
      width: Math.max(72, shape.width - 28),
      text: isSelected ? meta.label : shape.detail,
      fill: '#eaf2ff',
      opacity
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
        lastAction: state.lastAction
      })
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
