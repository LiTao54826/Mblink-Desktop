// Regression check: shutdown must break Preact and DOM wrapper reference chains.
// Usage: node tests/js/quickjs_shutdown_cleanup_guard.js

const fs = require('fs');
const path = require('path');

function assert(cond, msg) {
  if (!cond) throw new Error(msg);
}

function run() {
  const root = process.cwd();
  const bootstrapSrc = fs.readFileSync(path.join(root, 'js', 'runtime', 'bootstrap.js'), 'utf8');
  const preactRenderSrc = fs.readFileSync(path.join(root, 'third_party', 'preact', 'src', 'render.js'), 'utf8');
  const windowBindingsSrc = fs.readFileSync(path.join(root, 'core', 'quickjs', 'window_bindings.cpp'), 'utf8');
  const domBindingMapSrc = fs.readFileSync(path.join(root, 'core', 'quickjs', 'dom_binding_map.cpp'), 'utf8');
  const domBindingMapHeader = fs.readFileSync(path.join(root, 'core', 'quickjs', 'dom_binding_map.h'), 'utf8');
  const renderTreeSynchronizerSrc = fs.readFileSync(path.join(root, 'core', 'render', 'pipeline', 'render_tree_synchronizer.cpp'), 'utf8');

  assert(preactRenderSrc.includes('__mbinkRegisterPreactRoot(vnode, parentDom, render)'),
    'Preact render roots must be registered so shutdown can unmount them');
  assert(bootstrapSrc.includes('item.renderImpl(null, item.container)') &&
         bootstrapSrc.includes('item.container.__preactRoot = null'),
    'Runtime cleanup must unmount Preact roots before dropping root references');

  for (const name of [
    'addEventListener',
    'removeEventListener',
    'dispatchEvent',
    'setTimeout',
    'setInterval',
    'requestAnimationFrame',
    'clearTimeout',
    'clearInterval',
    'cancelAnimationFrame'
  ]) {
    assert(windowBindingsSrc.includes(`JS_SetPropertyStr(ctx, global, "${name}", JS_UNDEFINED)`),
      `WindowBindings::Cleanup must clear global ${name}`);
  }

  assert(windowBindingsSrc.includes('ClearKnownDOMWrapperBackrefs') &&
         windowBindingsSrc.includes('"_children"') &&
         windowBindingsSrc.includes('"_listeners"') &&
         windowBindingsSrc.includes('"__preactRoot"'),
    'WindowBindings::Cleanup must clear DOM wrapper Preact/listener backrefs');
  assert(domBindingMapSrc.includes('g_dom_binding_map_clearing') &&
         domBindingMapSrc.includes('if (g_dom_binding_map_clearing)') &&
         domBindingMapSrc.includes('g_dom_binding_map_clearing = true') &&
         domBindingMapSrc.includes('g_dom_binding_map_clearing = false'),
    'DOMBindingMap::Clear must guard against finalizer reentrancy');
  assert(domBindingMapHeader.includes('GetJSValueWithContext') &&
         domBindingMapSrc.includes('DOMBindingMap::GetJSValueWithContext'),
    'DOMBindingMap must expose context lookup so detached subtree cleanup can clear wrapper refs before removal');
  assert(renderTreeSynchronizerSrc.includes('CleanupDetachedDOMBinding') &&
         renderTreeSynchronizerSrc.includes('GetJSValueWithContext') &&
         renderTreeSynchronizerSrc.includes('ClearElementListenerBindings') &&
         renderTreeSynchronizerSrc.includes('ClearDetachedDOMWrapperBackrefs') &&
         renderTreeSynchronizerSrc.indexOf('CleanupDetachedDOMBinding(node.get())') <
         renderTreeSynchronizerSrc.indexOf('binding_map.Remove(node.get())'),
    'Detached subtree cleanup must clear JS listeners/backrefs before removing DOMBindingMap entries');
  assert(domBindingMapSrc.includes('std::vector<std::pair<Node*, JSValueEntry>> entries') &&
         domBindingMapSrc.includes('JS_DupValue(entry.ctx, entry.value)') &&
         domBindingMapSrc.includes('JS_FreeValue(entry.ctx, entry.value)'),
    'DOMBindingMap::ForEach must use a JSValue snapshot because cleanup visitors can trigger finalizers');

  console.log('[PASS] QuickJS shutdown cleanup guard is present');
}

run();
