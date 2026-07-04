import * as NativeLeaferUI from './leafer-ui/web.module.min.js';

function assert(condition, message) {
  if (!condition) throw new Error(message);
}

function sleep(ms) {
  return new Promise((resolve) => setTimeout(resolve, ms));
}

function isIdentifier(name) {
  return /^[A-Za-z_$][0-9A-Za-z_$]*$/.test(name) && name !== 'default';
}

function stripParamTypes(params) {
  return params.split(',').map((part) => {
    return part.replace(/^(\s*[A-Za-z_$][0-9A-Za-z_$]*\s*)\??:\s*[A-Za-z_$][0-9A-Za-z_$.<>\[\]|&?, ]*(\s*(=.*)?)$/s, '$1$2');
  }).join(',');
}

function stripTypeScript(code) {
  let output = String(code || '').replace(/\r\n/g, '\n');
  output = output.replace(/^\s*import\s+[\s\S]*?\s+from\s+['"][^'"]+['"]\s*;?\s*$/gm, '');
  output = output.replace(/^\s*import\s+['"][^'"]+['"]\s*;?\s*$/gm, '');
  output = output.replace(/\s+as\s+[A-Za-z_$][0-9A-Za-z_$.\[\]<>|&?, ]*/g, '');
  output = output.replace(/\b(let|const|var)\s+([A-Za-z_$][0-9A-Za-z_$]*)\s*:\s*[A-Za-z_$][0-9A-Za-z_$.\[\]<>|&?, ]*(?=[=;\n])/g, '$1 $2');
  output = output.replace(/(function\s*(?:[A-Za-z_$][0-9A-Za-z_$]*\s*)?\()([^)]*)(\))/g, function(_match, start, params, end) {
    return start + stripParamTypes(params) + end;
  });
  output = output.replace(/(\()([^(){};]*:[^(){};]*)(\)\s*=>)/g, function(_match, start, params, end) {
    return start + stripParamTypes(params) + end;
  });
  output = output.replace(/(\))\s*:\s*[A-Za-z_$][0-9A-Za-z_$.\[\]<>|&?, ]*(?=\s*\{)/g, '$1');
  output = output.replace(/([A-Za-z_$][0-9A-Za-z_$]*)!\./g, '$1.');
  return output;
}

function extractLeaferBindings(source) {
  const bindings = [];
  const seen = {};
  const importPattern = /import\s*\{([\s\S]*?)\}\s*from\s*['"]leafer-ui['"]/g;
  let match;
  while ((match = importPattern.exec(String(source || '')))) {
    const body = match[1];
    const parts = body.split(',');
    for (let part of parts) {
      part = part.replace(/\/\*[\s\S]*?\*\//g, '').replace(/\/\/.*$/g, '').trim();
      if (!part) continue;
      part = part.replace(/^type\s+/, '').trim();
      const aliasMatch = /^([A-Za-z_$][0-9A-Za-z_$]*)\s+as\s+([A-Za-z_$][0-9A-Za-z_$]*)$/.exec(part);
      const imported = aliasMatch ? aliasMatch[1] : part;
      const local = aliasMatch ? aliasMatch[2] : part;
      if (!isIdentifier(imported) || !isIdentifier(local)) continue;
      const key = local + ':' + imported;
      if (!seen[key]) {
        bindings.push({ imported, local });
        seen[key] = true;
      }
    }
  }
  return bindings;
}

function createRoot() {
  document.body.style.margin = '0';
  document.body.style.overflow = 'hidden';
  document.body.style.background = '#eef2f7';
  document.body.style.fontFamily = 'Segoe UI, Arial, sans-serif';

  const style = document.createElement('style');
  style.textContent = `
    #official-example-runner {
      width: 100vw;
      height: 100vh;
      position: relative;
      overflow: hidden;
      color: #172033;
      background: #eef2f7;
    }
    #official-example-toolbar {
      height: 72px;
      display: flex;
      align-items: center;
      gap: 12px;
      padding: 12px 16px;
      border-bottom: 1px solid #c8d3df;
      background: #ffffff;
    }
    #official-example-run-button {
      height: 38px;
      padding: 0 14px;
      border: 1px solid #1d4ed8;
      border-radius: 6px;
      color: #ffffff;
      background: #1d4ed8;
      font-weight: 700;
      cursor: pointer;
    }
    #official-example-summary {
      min-width: 460px;
      font-size: 13px;
      line-height: 19px;
      font-weight: 700;
      white-space: pre-wrap;
    }
    #official-example-pass-badge {
      position: absolute;
      top: 14px;
      right: 16px;
      width: 218px;
      height: 44px;
      display: flex;
      align-items: center;
      justify-content: center;
      border: 1px solid #166534;
      border-radius: 6px;
      color: #ffffff;
      background: #64748b;
      font-size: 14px;
      font-weight: 800;
    }
    #official-example-pass-badge.pass { background: #16a34a; }
    #official-example-pass-badge.fail { background: #dc2626; }
    #official-example-canvas-host {
      position: absolute;
      left: 16px;
      top: 88px;
      width: 720px;
      height: 420px;
      overflow: hidden;
      border: 1px solid #94a3b8;
      background: #ffffff;
    }
    #official-example-proof {
      position: absolute;
      left: 760px;
      top: 88px;
      width: 240px;
      height: 160px;
      overflow: hidden;
      border: 1px solid #94a3b8;
      background: #ffffff;
    }
    #official-example-status {
      position: absolute;
      left: 760px;
      top: 268px;
      width: 240px;
      height: 232px;
      margin: 0;
      padding: 10px;
      overflow: hidden;
      border: 1px solid #cbd5e1;
      background: #ffffff;
      font-size: 12px;
      line-height: 17px;
      white-space: pre-wrap;
    }
  `;
  document.head.appendChild(style);

  const root = document.createElement('main');
  root.id = 'official-example-runner';
  root.innerHTML = `
    <section id="official-example-toolbar">
      <button id="official-example-run-button" type="button">Run official examples</button>
      <div id="official-example-summary">ready</div>
    </section>
    <div id="official-example-pass-badge">waiting</div>
    <div id="official-example-canvas-host"></div>
    <div id="official-example-proof"></div>
    <pre id="official-example-status">waiting</pre>
  `;
  document.body.appendChild(root);
  return {
    root,
    button: root.querySelector('#official-example-run-button'),
    summary: root.querySelector('#official-example-summary'),
    badge: root.querySelector('#official-example-pass-badge'),
    hostSlot: root.querySelector('#official-example-canvas-host'),
    proof: root.querySelector('#official-example-proof'),
    status: root.querySelector('#official-example-status')
  };
}

export function runOfficialExamples(config) {
  const examples = Array.isArray(config && config.examples) ? config.examples : [];
  const skipped = Array.isArray(config && config.skipped) ? config.skipped : [];
  const options = Object.assign({ exampleDelayMs: 70, progressEvery: 20 }, config && config.options);
  const ui = createRoot();
  const AsyncFunction = Object.getPrototypeOf(async function() {}).constructor;

  let currentHost = null;
  let instances = [];
  let runPromise = null;

  function normalizeConfig(input) {
    if (!input || typeof input !== 'object') return input;
    const copy = Object.assign({}, input);
    const view = copy.view;
    const isOfficialWindowProxy = view && view.__officialWindowMarker === '__MBLINK_OFFICIAL_WINDOW__';
    if (!view || view === window || view === document.body || view === currentHost || isOfficialWindowProxy) {
      copy.view = currentHost;
    }
    if (!copy.width) copy.width = 720;
    if (!copy.height) copy.height = 420;
    return copy;
  }

  const TrackedLeafer = class extends NativeLeaferUI.Leafer {
    constructor(input) {
      super(normalizeConfig(input));
      instances.push(this);
    }
  };

  const TrackedApp = typeof NativeLeaferUI.App === 'function'
    ? class extends NativeLeaferUI.App {
        constructor(input) {
          super(normalizeConfig(input));
          instances.push(this);
        }
      }
    : undefined;

  const LeaferUI = Object.assign({}, NativeLeaferUI, { Leafer: TrackedLeafer });
  if (TrackedApp) LeaferUI.App = TrackedApp;

  function buildPrelude(example) {
    return extractLeaferBindings(example.source)
      .filter((binding) => Object.prototype.hasOwnProperty.call(LeaferUI, binding.imported))
      .map((binding) => 'const ' + binding.local + ' = LeaferUI[' + JSON.stringify(binding.imported) + '];')
      .join('\n') + '\n';
  }

  function resetHost(path) {
    for (const instance of instances) {
      try {
        if (instance && typeof instance.destroy === 'function') instance.destroy(true);
      } catch (_error) {
        // Cleanup should never hide the next official example result.
      }
    }
    instances = [];
    ui.hostSlot.textContent = '';
    currentHost = document.createElement('div');
    currentHost.id = 'leafer';
    currentHost.setAttribute('data-official-example', path);
    currentHost.style.cssText = 'position:absolute;left:0;top:0;width:720px;height:420px;background:#fff;overflow:hidden;';
    ui.hostSlot.appendChild(currentHost);
    return currentHost;
  }

  function createDocumentProxy(host) {
    return {
      createElement: document.createElement.bind(document),
      createElementNS: document.createElementNS ? document.createElementNS.bind(document) : undefined,
      createTextNode: document.createTextNode.bind(document),
      getElementById(id) {
        return host.querySelector('#' + id) || document.getElementById(id);
      },
      querySelector(selector) {
        return host.querySelector(selector) || document.querySelector(selector);
      },
      querySelectorAll(selector) {
        const local = host.querySelectorAll(selector);
        return local.length ? local : document.querySelectorAll(selector);
      },
      addEventListener: document.addEventListener.bind(document),
      removeEventListener: document.removeEventListener.bind(document),
      body: host,
      head: document.head,
      documentElement: document.documentElement
    };
  }

  function createWindowProxy() {
    const proxy = Object.create(window);
    proxy.__officialWindowMarker = '__MBLINK_OFFICIAL_WINDOW__';
    return proxy;
  }

  async function runOne(example) {
    const host = resetHost(example.path);
    const documentProxy = createDocumentProxy(host);
    const windowProxy = createWindowProxy();
    const timers = [];
    const asyncErrors = [];
    const localLogs = [];

    function recordAsyncError(error) {
      asyncErrors.push(error && (error.stack || error.message || String(error)));
    }

    function localSetTimeout(callback, ms) {
      const args = Array.prototype.slice.call(arguments, 2);
      const delay = Math.max(0, Math.min(Number(ms) || 0, 35));
      const id = setTimeout(() => {
        try {
          if (typeof callback === 'function') callback.apply(null, args);
        } catch (error) {
          recordAsyncError(error);
        }
      }, delay);
      timers.push(id);
      return id;
    }

    function localClearTimeout(id) {
      clearTimeout(id);
    }

    function localRequestAnimationFrame(callback) {
      return localSetTimeout(() => callback(performance.now()), 16);
    }

    const localConsole = {
      log: function() { localLogs.push(Array.prototype.map.call(arguments, String).join(' ')); },
      warn: function() { localLogs.push('[warn] ' + Array.prototype.map.call(arguments, String).join(' ')); },
      error: function() { localLogs.push('[error] ' + Array.prototype.map.call(arguments, String).join(' ')); }
    };

    const ObjectShim = Object.create(Object);
    ObjectShim.defineProperty = function(target, prop, descriptor) {
      try {
        return Object.defineProperty(target, prop, descriptor);
      } catch (error) {
        const existing = Object.getOwnPropertyDescriptor(target, prop);
        if (existing && !existing.configurable) return target;
        throw error;
      }
    };

    const code = buildPrelude(example) + stripTypeScript(example.source);
    const fn = new AsyncFunction(
      'LeaferUI',
      'host',
      'window',
      'document',
      'console',
      'Object',
      'setTimeout',
      'clearTimeout',
      'requestAnimationFrame',
      'cancelAnimationFrame',
      'sleep',
      code
    );

    await fn(LeaferUI, host, windowProxy, documentProxy, localConsole, ObjectShim, localSetTimeout, localClearTimeout, localRequestAnimationFrame, localClearTimeout, sleep);
    await sleep(options.exampleDelayMs);
    for (const id of timers) clearTimeout(id);
    if (asyncErrors.length) throw new Error(asyncErrors.join('\n'));
    return {
      path: example.path,
      ok: true,
      canvas: !!host.querySelector('canvas'),
      logs: localLogs.slice(0, 5)
    };
  }

  function drawProof(ok) {
    ui.proof.textContent = '';
    const leafer = new TrackedLeafer({ view: ui.proof, width: 240, height: 160, fill: '#ffffff' });
    leafer.add(new NativeLeaferUI.Rect({
      x: 20,
      y: 20,
      width: 92,
      height: 64,
      fill: ok ? '#16a34a' : '#dc2626',
      stroke: '#0f172a',
      strokeWidth: 2,
      cornerRadius: 8
    }));
    leafer.add(new NativeLeaferUI.Text({
      x: 32,
      y: 42,
      width: 120,
      text: ok ? 'PASS' : 'FAIL',
      fill: '#ffffff',
      fontSize: 18,
      fontWeight: '700'
    }));
  }

  async function startRun() {
    assert(examples.length > 0, 'no executable official examples were provided');
    const results = [];
    const failures = [];
    ui.button.disabled = true;
    ui.badge.className = '';
    ui.badge.textContent = 'running';
    ui.summary.textContent = 'running official examples';
    ui.status.textContent = 'starting';

    for (let index = 0; index < examples.length; index += 1) {
      const example = examples[index];
      try {
        const result = await runOne(example);
        results.push(result);
      } catch (error) {
        const failure = {
          path: example.path,
          ok: false,
          name: error && error.name ? error.name : 'Error',
          message: error && error.message ? error.message : String(error),
          error: error && (error.stack || error.message || String(error))
        };
        failures.push(failure);
        results.push(failure);
      }

      if ((index + 1) % options.progressEvery === 0 || index + 1 === examples.length) {
        ui.status.textContent = 'running ' + (index + 1) + '/' + examples.length +
          '\nfailed: ' + failures.length +
          '\nlast: ' + example.path;
        await sleep(1);
      }
    }

    const passed = examples.length - failures.length;
    const ok = failures.length === 0;
    const summary = 'checked: ' + examples.length +
      '\npassed: ' + passed +
      '\nfailed: ' + failures.length +
      '\nskipped: ' + skipped.length;
    ui.summary.textContent = summary;
    ui.status.textContent = summary + '\n' + failures.slice(0, 10).map((failure) => failure.path + ': ' + failure.message).join('\n');
    ui.badge.className = ok ? 'pass' : 'fail';
    ui.badge.textContent = ok ? 'official examples pass' : 'official examples fail';
    drawProof(ok);

    const payload = {
      ok,
      total: examples.length + skipped.length,
      executable: examples.length,
      passed,
      failed: failures.length,
      skipped: skipped.length,
      results,
      failures,
      skippedExamples: skipped
    };
    window.__LEAFER_OFFICIAL_EXAMPLE_RESULTS__ = payload;
    let resultNode = document.getElementById('official-example-results-json');
    if (!resultNode) {
      resultNode = document.createElement('script');
      resultNode.id = 'official-example-results-json';
      resultNode.type = 'application/json';
      document.body.appendChild(resultNode);
    }
    resultNode.textContent = JSON.stringify(payload);
    console.log('[leafer-official-examples] checked ' + examples.length + ' passed ' + passed + ' failed ' + failures.length + ' skipped ' + skipped.length);
    if (failures.length) console.error('[leafer-official-examples] failures ' + JSON.stringify(failures.slice(0, 40)));
    ui.button.disabled = false;
    return payload;
  }

  ui.button.addEventListener('click', () => {
    if (!runPromise) {
      runPromise = startRun().catch((error) => {
        const message = error && (error.stack || error.message || String(error));
        ui.badge.className = 'fail';
        ui.badge.textContent = 'runner fatal';
        ui.summary.textContent = 'fatal: ' + message;
        ui.status.textContent = String(message);
        console.error('[leafer-official-examples] fatal ' + message);
        throw error;
      });
    }
  });

  window.__LEAFER_OFFICIAL_EXAMPLE_RUNNER__ = {
    startRun: () => {
      if (!runPromise) runPromise = startRun();
      return runPromise;
    },
    getResults: () => window.__LEAFER_OFFICIAL_EXAMPLE_RESULTS__ || null
  };

  ui.summary.textContent = 'ready: executable ' + examples.length + ', skipped ' + skipped.length;
  ui.status.textContent = 'ready';
  console.log('[leafer-official-examples] ready executable ' + examples.length + ' skipped ' + skipped.length);
}
