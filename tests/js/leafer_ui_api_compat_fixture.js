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
  PointerButton,
  PointerEvent,
  DragEvent,
  RenderEvent
} from '../../examples/leafer_ui_showcase/js/leafer-ui/web.module.min.js';

function assert(condition, message) {
  if (!condition) {
    throw new Error(message);
  }
}

const imageFixtureUrl = 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAQAAAAECAYAAACp8Z5+AAAAAXNSR0IArs4c6QAAAARnQU1BAACxjwv8YQUAAAAJcEhZcwAADsMAAA7DAcdvqGQAAAAYSURBVBhXY9BvePsfhH+dEQVjkgVE/wMAvCsrqWVhawoAAAAASUVORK5CYII=';

function createHost(id, width, height) {
  const host = document.createElement('div');
  host.id = id;
  host.style.width = `${width}px`;
  host.style.height = `${height}px`;
  host.style.display = 'block';
  host.style.position = 'relative';
  host.style.background = '#ffffff';
  document.body.appendChild(host);
  return host;
}

function assertLeaferExports() {
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
    assert(typeof value === 'function', `Leafer export missing: ${name}`);
  }
  assert(typeof PointerButton === 'object', 'PointerButton export missing');
  assert(typeof PointerButton.MIDDLE === 'number', 'PointerButton.MIDDLE missing');
  assert(typeof PointerButton.RIGHT === 'number', 'PointerButton.RIGHT missing');
  assert(typeof PointerEvent === 'function', 'Leafer PointerEvent export missing');
  assert(typeof PointerEvent.DOWN === 'string', 'PointerEvent.DOWN missing');
  assert(typeof PointerEvent.TAP === 'string', 'PointerEvent.TAP missing');
  assert(typeof PointerEvent.CLICK === 'string', 'PointerEvent.CLICK missing');
  assert(typeof DragEvent === 'function', 'Leafer DragEvent export missing');
  assert(typeof DragEvent.START === 'string', 'DragEvent.START missing');
  assert(typeof DragEvent.DRAG === 'string', 'DragEvent.DRAG missing');
  assert(typeof DragEvent.END === 'string', 'DragEvent.END missing');
  assert(typeof RenderEvent === 'function', 'Leafer RenderEvent export missing');
  assert(typeof RenderEvent.END === 'string', 'RenderEvent.END missing');
}

function assertDomCanvasSurface(canvas, context) {
  const nodeProbe = document.createElement('div');
  nodeProbe.appendChild(document.createElement('span'));
  assert(typeof nodeProbe.hasChildNodes === 'function', 'Node.hasChildNodes missing');
  assert(nodeProbe.hasChildNodes(), 'Node.hasChildNodes false for non-empty node');
  assert(!document.createElement('div').hasChildNodes(), 'Node.hasChildNodes true for empty node');

  assert(typeof CanvasRenderingContext2D === 'function', 'CanvasRenderingContext2D global missing');
  assert(context instanceof CanvasRenderingContext2D, '2d context is not a CanvasRenderingContext2D');
  context.fillStyle = '#2f80ed';
  context.fillRect(8, 8, 80, 44);
  context.font = 'normal 600 28px sans-serif';
  assert(context.font === '28px', 'Canvas font shorthand px parsing failed');
  assert(context.measureText('direct canvas text').width > 0, 'Canvas measureText returned zero width');
  context.fillStyle = '#ffffff';
  context.fillText('text', 16, 40);

  CanvasRenderingContext2D.prototype.__leaferCompatPatch = function() {
    return 'patched';
  };
  assert(context.__leaferCompatPatch() === 'patched', 'CanvasRenderingContext2D prototype patch failed');

  assert(typeof Path2D === 'function', 'Path2D global missing');
  const path2d = new Path2D();
  assert(path2d instanceof Path2D, 'Path2D instance check failed');
  Path2D.prototype.__leaferCompatPatch = function() {
    this.moveTo(0, 0);
    this.lineTo(4, 4);
    return 'patched';
  };
  assert(path2d.__leaferCompatPatch() === 'patched', 'Path2D prototype patch failed');

  assert(typeof ImageData === 'function', 'ImageData global missing');
  const imageData = new ImageData(2, 2);
  assert(imageData instanceof ImageData, 'ImageData instance check failed');
  assert(imageData.width === 2 && imageData.height === 2, 'ImageData dimensions wrong');
  assert(imageData.data.length === 16, 'ImageData data length wrong');

  assert(typeof CanvasGradient === 'function', 'CanvasGradient global missing');
  const gradient = context.createLinearGradient(0, 0, 10, 10);
  assert(gradient instanceof CanvasGradient, 'CanvasGradient instance check failed');
  assert(typeof CanvasPattern === 'function', 'CanvasPattern global missing');

  assert(typeof Element === 'function', 'Element global missing');
  assert(typeof HTMLElement === 'function', 'HTMLElement global missing');
  assert(typeof HTMLCanvasElement === 'function', 'HTMLCanvasElement global missing');
  assert(canvas instanceof Element, 'canvas is not an Element');
  assert(canvas instanceof HTMLElement, 'canvas is not an HTMLElement');
  assert(canvas instanceof HTMLCanvasElement, 'canvas is not an HTMLCanvasElement');

  assert(typeof navigator === 'object' && typeof navigator.userAgent === 'string', 'navigator.userAgent missing');
  assert(typeof devicePixelRatio === 'number' && devicePixelRatio > 0, 'devicePixelRatio missing');

  assert(typeof MouseEvent === 'function', 'MouseEvent global missing');
  assert(typeof globalThis.PointerEvent === 'function', 'DOM PointerEvent global missing');
  assert(typeof globalThis.DragEvent === 'function', 'DOM DragEvent global missing');
  assert(typeof KeyboardEvent === 'function', 'KeyboardEvent global missing');
  assert(new globalThis.PointerEvent('pointerdown', { clientX: 7, clientY: 9 }).clientX === 7, 'PointerEvent clientX wrong');
  assert(new globalThis.DragEvent('dragstart').dataTransfer, 'DragEvent dataTransfer missing');
  assert(new KeyboardEvent('keydown', { key: 'A', code: 'KeyA' }).key === 'A', 'KeyboardEvent key wrong');
}

function createDirectCanvas() {
  const canvas = document.createElement('canvas');
  canvas.id = 'leafer-api-canvas';
  canvas.width = 120;
  canvas.height = 80;
  canvas.style.display = 'block';
  canvas.style.width = '120px';
  canvas.style.height = '80px';
  canvas.style.background = '#ffffff';
  document.body.appendChild(canvas);
  return canvas;
}

function buildMainLeafer(host) {
  const leafer = new Leafer({
    view: host,
    width: 360,
    height: 260,
    fill: '#ffffff'
  });
  assert(leafer.width === 360, 'Leafer width did not initialize');
  assert(leafer.height === 260, 'Leafer height did not initialize');
  assert(host.querySelector('canvas'), 'Leafer did not create a canvas in div view');

  let renderCount = 0;
  leafer.on(RenderEvent.END, () => {
    renderCount += 1;
  });

  const group = new Group({ x: 12, y: 88 });
  const rect = new Rect({
    x: 10,
    y: 10,
    width: 70,
    height: 44,
    fill: '#2f80ed',
    stroke: '#102033',
    strokeWidth: 3,
    cornerRadius: 8,
    opacity: 0.95,
    zIndex: 1
  });
  const text = new Text({
    x: 20,
    y: 20,
    width: 90,
    text: 'Leafer',
    fill: '#ffffff',
    fontSize: 15,
    fontWeight: '700',
    zIndex: 2
  });
  const ellipse = new Ellipse({
    x: 96,
    y: 8,
    width: 44,
    height: 44,
    innerRadius: 0.45,
    fill: '#feb027',
    stroke: '#4338ca',
    strokeWidth: 2
  });
  const line = new Line({
    x: 156,
    y: 32,
    width: 48,
    stroke: '#334155',
    strokeWidth: 5,
    strokeCap: 'round'
  });
  const path = new Path({
    x: 218,
    y: 10,
    path: 'M0 40 L24 0 L48 40 Z',
    fill: '#e85d75',
    stroke: '#111827',
    strokeWidth: 2
  });
  const polygon = new Polygon({
    x: 278,
    y: 8,
    width: 42,
    height: 42,
    sides: 6,
    cornerRadius: 6,
    fill: '#20b486',
    stroke: '#0f766e',
    strokeWidth: 2
  });
  const star = new Star({
    x: 322,
    y: 7,
    width: 38,
    height: 38,
    corners: 5,
    innerRadius: 0.45,
    fill: '#d99a24',
    stroke: '#78350f',
    strokeWidth: 2
  });
  const hidden = new Rect({
    x: 260,
    y: 66,
    width: 36,
    height: 26,
    fill: '#7c3aed',
    visible: false
  });
  const customCanvas = new LeaferCanvas({ x: 306, y: 60, width: 36, height: 30 });
  assert(customCanvas.context, 'Leafer Canvas context missing');
  customCanvas.context.fillStyle = '#6d28d9';
  customCanvas.context.fillRect(0, 0, 36, 30);
  customCanvas.context.fillStyle = '#facc15';
  customCanvas.context.fillRect(10, 8, 16, 14);
  customCanvas.paint();

  const pen = new Pen();
  assert(typeof pen.setStyle === 'function', 'Pen.setStyle missing');
  pen.setStyle({ x: 204, y: 62, fill: '#ff4b4b', windingRule: 'evenodd' });
  pen.roundRect(0, 0, 38, 30, 8).arc(19, 15, 7);

  group.add([rect, text, ellipse, line, path, polygon, star, hidden, customCanvas, pen]);
  assert(group.children.length === 10, 'Group.add did not add children');
  leafer.add(group);

  assert(typeof group.addAt === 'function', 'Group.addAt missing');
  assert(typeof group.addBefore === 'function', 'Group.addBefore missing');
  assert(typeof group.addAfter === 'function', 'Group.addAfter missing');
  group.addAt(star, 0);
  assert(group.children[0] === star, 'Group.addAt did not move child');
  group.addBefore(path, ellipse);
  assert(group.children.indexOf(path) < group.children.indexOf(ellipse), 'Group.addBefore did not reorder child');
  group.remove(hidden);
  assert(group.children.indexOf(hidden) < 0, 'Group.remove did not remove child');
  group.addAfter(hidden, polygon);
  assert(group.children.indexOf(hidden) === group.children.indexOf(polygon) + 1, 'Group.addAfter did not insert child after target');
  hidden.visible = true;

  const image = new LeaferImage({
    x: 314,
    y: 106,
    width: 30,
    height: 30,
    url: imageFixtureUrl
  });
  leafer.add(image);

  const eventTarget = new Rect({
    x: 22,
    y: 196,
    width: 58,
    height: 34,
    fill: '#7c3aed',
    cornerRadius: 8
  });
  const dragTarget = new Rect({
    x: 104,
    y: 196,
    width: 58,
    height: 34,
    fill: '#0ea5e9',
    draggable: true,
    cornerRadius: 8
  });
  leafer.add(eventTarget);
  leafer.add(dragTarget);

  const state = { down: 0, tap: 0, click: 0, drag: 0 };
  eventTarget.on(PointerEvent.DOWN, () => {
    state.down += 1;
    eventTarget.fill = '#9333ea';
  });
  eventTarget.on(PointerEvent.TAP, () => {
    state.tap += 1;
    eventTarget.fill = '#c026d3';
  });
  eventTarget.on(PointerEvent.CLICK, () => {
    state.click += 1;
    eventTarget.stroke = '#facc15';
    eventTarget.strokeWidth = 2;
  });
  dragTarget.on(DragEvent.START, () => {
    dragTarget.fill = '#2563eb';
  });
  dragTarget.on(DragEvent.DRAG, () => {
    state.drag += 1;
    dragTarget.fill = '#ef4444';
  });
  dragTarget.on(DragEvent.END, () => {
    dragTarget.stroke = '#facc15';
    dragTarget.strokeWidth = 2;
  });

  return { leafer, rect, text, path, eventTarget, dragTarget, state, getRenderCount: () => renderCount };
}

function buildExtraLeaferMounts() {
  const canvasHost = document.createElement('canvas');
  canvasHost.id = 'leafer-canvas-view-host';
  canvasHost.width = 96;
  canvasHost.height = 56;
  canvasHost.style.width = '96px';
  canvasHost.style.height = '56px';
  document.body.appendChild(canvasHost);
  const canvasLeafer = new Leafer({ view: canvasHost, width: 96, height: 56, fill: '#ffffff' });
  canvasLeafer.add(new Rect({ x: 4, y: 4, width: 42, height: 28, fill: '#20b486' }));
  assert(canvasLeafer.canvas.view === canvasHost, 'Leafer canvas view did not use provided canvas');

  const idHost = createHost('leafer-id-view-host', 96, 56);
  const idLeafer = new Leafer({ view: 'leafer-id-view-host', width: 96, height: 56, fill: '#ffffff' });
  idLeafer.add(new Rect({ x: 4, y: 4, width: 42, height: 28, fill: '#d99a24' }));
  assert(idHost.querySelector('canvas'), 'Leafer id view did not create canvas');

  const repeatHost = createHost('leafer-repeat-view-host', 80, 48);
  const repeatA = new Leafer({ view: repeatHost, width: 80, height: 48, fill: '#ffffff' });
  const repeatB = new Leafer({ view: repeatHost, width: 80, height: 48, fill: '#ffffff' });
  assert(repeatHost.querySelectorAll('canvas').length >= 2, 'repeated Leafer init did not create independent canvases');
  repeatA.destroy(true);
  repeatB.destroy(true);
  assert(repeatA.destroyed === true || repeatA.running === false, 'repeat Leafer A did not enter cleanup state');
  assert(repeatB.destroyed === true || repeatB.running === false, 'repeat Leafer B did not enter cleanup state');

  return [canvasLeafer, idLeafer];
}

function verifyResizeObserver(canvas, status, onReady) {
  assert(typeof ResizeObserver === 'function', 'ResizeObserver global missing');
  let resizeObserved = false;
  const observer = new ResizeObserver((entries, instance) => {
    assert(instance === observer, 'ResizeObserver callback instance wrong');
    assert(entries.length === 1, 'ResizeObserver entry count wrong');
    assert(entries[0].target === canvas, 'ResizeObserver target wrong');
    assert(typeof entries[0].contentRect.width === 'number', 'ResizeObserver contentRect missing');
    resizeObserved = true;
    observer.disconnect();
  });
  observer.observe(canvas);
  setTimeout(() => {
    assert(resizeObserved, 'ResizeObserver did not fire');
    status.textContent = 'ready: leafer docs api compat';
    onReady();
  }, 250);
}

function waitForImageLoad() {
  return new Promise((resolve, reject) => {
    const image = new globalThis.Image();
    const timeout = setTimeout(() => reject(new Error('data URL image fixture did not load')), 450);
    image.onload = () => {
      clearTimeout(timeout);
      assert(image.naturalWidth === 4 && image.naturalHeight === 4, 'data URL image natural size wrong');
      resolve();
    };
    image.onerror = () => {
      clearTimeout(timeout);
      reject(new Error('data URL image fixture failed to load'));
    };
    image.src = imageFixtureUrl;
  });
}

function fail(error) {
  const detail = error && error.stack ? `${error.message || error}\n${error.stack}` : error;
  const message = String(detail);
  if (status) status.textContent = `failed: ${message}`;
  console.error(message);
}

document.body.style.margin = '0';
document.body.style.background = '#ffffff';

const canvas = createDirectCanvas();
const context = canvas.getContext('2d');
const root = createHost('leafer-api-compat', 360, 260);
const status = document.createElement('div');
status.id = 'leafer-api-status';
status.textContent = 'waiting: leafer docs api compat';
document.body.appendChild(status);

try {
  assertLeaferExports();
  assertDomCanvasSurface(canvas, context);
  const main = buildMainLeafer(root);
  const extraLeaferMounts = buildExtraLeaferMounts();

  const runInteractions = () => {
    const x = main.eventTarget.x + 14;
    const y = main.eventTarget.y + 14;
    main.leafer.interaction.pointerDown({ x, y });
    main.leafer.interaction.pointerUp({ x, y });

    const startX = main.dragTarget.x + 14;
    const startY = main.dragTarget.y + 14;
    main.leafer.interaction.pointerDown({ x: startX, y: startY });
    main.leafer.interaction.pointerMove({ x: startX + 36, y: startY + 18 });
    main.leafer.interaction.pointerUp({ x: startX + 36, y: startY + 18 });

    main.rect.set({
      x: 18,
      width: 86,
      fill: '#2563eb',
      opacity: 0.85,
      visible: true
    });
    main.text.text = 'Updated';
    main.path.fill = '#f97316';

    requestAnimationFrame(() => {
      main.rect.x = 116;
      main.rect.fill = '#14b8a6';
      status.setAttribute('data-raf', '1');
    });
  };

  const finish = () => {
    assert(main.state.down >= 1, 'PointerEvent.DOWN did not fire');
    assert(main.state.tap >= 1, 'PointerEvent.TAP did not fire');
    assert(main.state.click >= 1, 'PointerEvent.CLICK did not fire');
    assert(main.state.drag >= 1, 'DragEvent.DRAG did not fire');
    assert(status.getAttribute('data-raf') === '1', 'requestAnimationFrame did not run');
    assert(main.getRenderCount() >= 1, 'RenderEvent.END did not fire');
    status.textContent = 'ready: leafer docs api compat';
    globalThis.__leaferApiCompat = {
      ok: true,
      events: main.state,
      renderCount: main.getRenderCount(),
      mounts: extraLeaferMounts.length + 1
    };
    console.log('[leafer-api-compat] docs coverage ready');
    console.log('[leafer-api-compat] ready');
  };

  const afterViewReady = () => {
    runInteractions();
    Promise.all([waitForImageLoad()])
      .then(() => setTimeout(finish, 80))
      .catch(fail);
  };

  if (typeof main.leafer.waitViewReady === 'function') {
    main.leafer.waitViewReady(() => verifyResizeObserver(canvas, status, afterViewReady));
  } else {
    setTimeout(() => verifyResizeObserver(canvas, status, afterViewReady), 100);
  }
} catch (error) {
  fail(error);
}
