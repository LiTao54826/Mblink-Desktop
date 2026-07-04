import {
  Leafer,
  Rect,
  Text,
  Group,
  Path,
  RenderEvent
} from './leafer-ui/web.module.min.js';

const COLORS = {
  background: '#d9e4ef',
  card: '#ffffff',
  bodyStroke: '#6f8299',
  accent: '#2f8f7a',
  altAccent: '#b8687a',
  text: '#1f2d3d'
};

const STAGE = { width: 420, height: 260 };
const WORLD = { x: 128, y: 96 };
const NODE = { x: -72, y: -48, width: 160, height: 80 };
const TOP_LEFT_NODE = { x: -116, y: -84 };

let leafer = null;
let renderCount = 0;
let world = null;
let nodeGroup = null;
let body = null;
let cap = null;
let connector = null;
let status = null;

function assert(condition, message) {
  if (!condition) throw new Error(message);
}

function setStatus(action) {
  status.textContent = `${action}; node ${Math.round(nodeGroup.x)},${Math.round(nodeGroup.y)}; renders ${renderCount}`;
  status.setAttribute('data-action', action);
  status.setAttribute('data-node-x', String(Math.round(nodeGroup.x)));
  status.setAttribute('data-node-y', String(Math.round(nodeGroup.y)));
}

function edgePath() {
  const startX = nodeGroup.x + NODE.width;
  const startY = nodeGroup.y + NODE.height / 2;
  const endX = nodeGroup.x + NODE.width + 132;
  const endY = nodeGroup.y + NODE.height / 2 + 28;
  return `M${startX} ${startY} C${startX + 42} ${startY} ${endX - 42} ${endY} ${endX} ${endY}`;
}

function updateConnector() {
  connector.path = edgePath();
}

function moveTopLeft() {
  nodeGroup.x = TOP_LEFT_NODE.x;
  nodeGroup.y = TOP_LEFT_NODE.y;
  updateConnector();
  setStatus('top-left');
}

function hideAccent() {
  cap.visible = false;
  connector.visible = false;
  setStatus('accent hidden');
}

function recolorAccent() {
  cap.visible = true;
  connector.visible = true;
  cap.fill = COLORS.altAccent;
  connector.stroke = COLORS.altAccent;
  setStatus('accent recolored');
}

function createButton(id, text, handler) {
  const button = document.createElement('button');
  button.id = id;
  button.type = 'button';
  button.textContent = text;
  button.addEventListener('click', handler);
  return button;
}

function sampleCanvasPixel(x, y) {
  const canvas = document.querySelector('#isolation-stage canvas');
  assert(canvas, 'isolation canvas missing');
  const context = canvas.getContext('2d');
  assert(context, '2d context missing');
  const data = context.getImageData(x, y, 1, 1).data;
  return [data[0], data[1], data[2], data[3]];
}

function installDom() {
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
    background: '#edf2f7',
    color: COLORS.text,
    fontFamily: 'Segoe UI, Arial, sans-serif'
  });

  const style = document.createElement('style');
  style.textContent = `
    * { box-sizing: border-box; }
    #isolation-root {
      width: 100%;
      height: 100%;
      display: grid;
      grid-template-rows: 44px ${STAGE.height}px minmax(0, 1fr);
      gap: 8px;
      padding: 10px;
      overflow: hidden;
    }
    #isolation-toolbar {
      display: flex;
      align-items: center;
      gap: 8px;
    }
    #isolation-toolbar button {
      min-width: 132px;
      min-height: 30px;
      border: 1px solid #b8c6d7;
      border-radius: 6px;
      background: #ffffff;
      color: #24364b;
      font: 700 12px Segoe UI, Arial, sans-serif;
    }
    #isolation-stage {
      width: ${STAGE.width}px;
      height: ${STAGE.height}px;
      overflow: hidden;
      background: ${COLORS.background};
    }
    #isolation-stage canvas {
      display: block;
    }
    #isolation-status {
      min-height: 28px;
      padding: 6px 8px;
      border: 1px solid #cad6e4;
      border-radius: 6px;
      background: #ffffff;
      color: #33465c;
      font-size: 12px;
      line-height: 16px;
    }
  `;
  document.head.appendChild(style);

  const root = document.createElement('main');
  root.id = 'isolation-root';

  const toolbar = document.createElement('div');
  toolbar.id = 'isolation-toolbar';
  toolbar.appendChild(createButton('isolation-move-top-left-button', 'Move top-left', moveTopLeft));
  toolbar.appendChild(createButton('isolation-hide-accent-button', 'Hide accent', hideAccent));
  toolbar.appendChild(createButton('isolation-recolor-accent-button', 'Recolor accent', recolorAccent));

  const stage = document.createElement('div');
  stage.id = 'isolation-stage';

  status = document.createElement('div');
  status.id = 'isolation-status';
  status.textContent = 'booting';

  root.appendChild(toolbar);
  root.appendChild(stage);
  root.appendChild(status);
  document.body.textContent = '';
  document.body.appendChild(root);
}

function buildLeafer() {
  assert(typeof Leafer === 'function', 'Leafer export missing');
  assert(typeof Rect === 'function', 'Rect export missing');
  assert(typeof Path === 'function', 'Path export missing');

  const stage = document.getElementById('isolation-stage');
  leafer = new Leafer({
    view: stage,
    width: STAGE.width,
    height: STAGE.height,
    fill: COLORS.background
  });

  world = new Group({ x: WORLD.x, y: WORLD.y });
  connector = new Path({
    path: 'M0 0 L1 1',
    stroke: COLORS.accent,
    strokeWidth: 4,
    strokeCap: 'round',
    opacity: 0.95,
    hittable: false
  });
  nodeGroup = new Group({ x: NODE.x, y: NODE.y });
  body = new Rect({
    x: 0,
    y: 0,
    width: NODE.width,
    height: NODE.height,
    fill: COLORS.card,
    stroke: COLORS.bodyStroke,
    strokeWidth: 2,
    cornerRadius: 8
  });
  cap = new Rect({
    x: 14,
    y: 10,
    width: NODE.width - 28,
    height: 4,
    fill: COLORS.accent,
    cornerRadius: 2,
    hittable: false
  });
  const label = new Text({
    x: 16,
    y: 28,
    width: NODE.width - 32,
    text: 'Stroke isolation',
    fill: COLORS.text,
    fontSize: 14,
    fontWeight: '700',
    hittable: false
  });

  nodeGroup.add([body, cap, label]);
  world.add([connector, nodeGroup]);
  leafer.add(world);
  updateConnector();

  leafer.on(RenderEvent.END, () => {
    renderCount += 1;
    status.setAttribute('data-renders', String(renderCount));
  });
}

installDom();
buildLeafer();
setStatus('ready');

globalThis.__leaferTopLeftStrokeIsolation = {
  sampleCanvasPixel,
  moveTopLeft,
  hideAccent,
  recolorAccent,
  state() {
    return {
      nodeX: nodeGroup.x,
      nodeY: nodeGroup.y,
      bodyStroke: body.stroke,
      capFill: cap.fill,
      capVisible: cap.visible !== false,
      connectorStroke: connector.stroke,
      connectorVisible: connector.visible !== false,
      renderCount
    };
  }
};

console.log('[leafer-top-left-stroke-isolation] ready');
