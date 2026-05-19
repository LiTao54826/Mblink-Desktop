// Regression check: retained-present is on by default but still constrained by safety gates.
// Usage: node tests/js/window_retained_present_default_guard.js

const fs = require('fs');
const path = require('path');

function assert(cond, msg) {
  if (!cond) throw new Error(msg);
}

function run() {
  const windowPath = path.join(process.cwd(), 'core', 'window', 'window.cpp');
  const mousePath = path.join(process.cwd(), 'core', 'event', 'dispatch', 'mouse_event_dispatcher.cpp');
  const domObserverPath = path.join(process.cwd(), 'core', 'window', 'window_dom_observer.cpp');
  const windowSrc = fs.readFileSync(windowPath, 'utf8');
  const mouseSrc = fs.readFileSync(mousePath, 'utf8');
  const domObserverSrc = fs.readFileSync(domObserverPath, 'utf8');

  assert(windowSrc.includes('MBINK_DISABLE_RETAINED_PRESENT'),
    'Retained-present default must keep an emergency disable switch');
  assert(windowSrc.includes('retained_present_experiment_enabled') &&
         windowSrc.includes('retained_present_safety_ok'),
    'Retained-present enablement must keep separate default/env and safety predicates');
  assert(windowSrc.includes('retained_present_experiment_enabled && retained_present_safety_ok'),
    'Retained-present must only enable when not disabled and safety passes');
  assert(windowSrc.includes('retained_present_mode=') &&
         windowSrc.includes('"disabled_env"') &&
         windowSrc.includes('"default"') &&
         windowSrc.includes('"blocked_safety"'),
    'Window baseline logs must expose retained-present mode for diagnosis');

  const safetyStart = windowSrc.indexOf('const bool retained_present_safety_ok =');
  const safetyEnd = windowSrc.indexOf(';', safetyStart);
  const safetyBlock = windowSrc.slice(safetyStart, safetyEnd);
  for (const gate of [
    '!config_.transparent',
    '!has_active_animations',
    'retained_width_px > 0',
    'retained_height_px > 0',
    '!retained_main_scroll_fallback_blocked'
  ]) {
    assert(safetyBlock.includes(gate), `Retained-present safety gate must include ${gate}`);
  }

  assert(windowSrc.includes('dropdown_manager.Paint(canvas)') &&
         windowSrc.indexOf('dropdown_manager.Paint(canvas)') > windowSrc.indexOf('retained_main_surface_->makeImageSnapshot()'),
    'Dropdown overlay must be painted after retained-main copy');
  const reuseStart = windowSrc.indexOf('const bool can_reuse_retained_main =');
  const reuseEnd = windowSrc.indexOf(';', reuseStart);
  const reuseBlock = windowSrc.slice(reuseStart, reuseEnd);
  assert(reuseStart >= 0 && reuseBlock.includes('!force_full_repaint_'),
    'Retained-main reuse must be blocked by Window-level force_full_repaint_');
  assert(windowSrc.includes('can_update_retained_dirty_region') &&
         windowSrc.includes('UnionDirtyRects') &&
         windowSrc.includes('dirty_bounds_px') &&
         windowSrc.includes('can_update_retained_dirty_region ? &dirty_bounds : nullptr') &&
         windowSrc.includes('main_canvas->clipRect(dirty_bounds'),
    'Dirty retained-present frames must clip redraw work to the dirty bounds');
  const dirtyClipStart = windowSrc.indexOf('const bool retained_dirty_clip_allowed =');
  const dirtyClipEnd = windowSrc.indexOf(';', dirtyClipStart);
  const dirtyClipBlock = windowSrc.slice(dirtyClipStart, dirtyClipEnd);
  assert(dirtyClipStart >= 0 &&
         dirtyClipBlock.includes('CanUseRetainedDirtyClipForReason(last_repaint_reason_)') &&
         dirtyClipBlock.includes('(!had_pending_dom_changes || !had_structural_dom_changes)') &&
         dirtyClipBlock.includes('!render_tree_rebuild_required') &&
         dirtyClipBlock.includes('(!needs_layout_update || has_dirty_bounds)'),
    'Dirty retained-present clipping must stay blocked for structural/tree rebuild frames but allow bounded style/layout updates');
  assert(windowSrc.includes('AddRetainedDirtyRectsForPendingChanges(this, tracker)') &&
         windowSrc.includes('AddRetainedDirtyRectsForPaintDirtyTree(this, cached_render_tree_.get())'),
    'Non-structural DOM batches must collect old and new dirty bounds for retained-present clipping');
  const updateStart = windowSrc.indexOf('const bool can_update_retained_dirty_region =');
  const updateEnd = windowSrc.indexOf(';', updateStart);
  const updateBlock = windowSrc.slice(updateStart, updateEnd);
  assert(updateStart >= 0 && updateBlock.includes('retained_dirty_clip_allowed'),
    'Dirty retained-present update must be gated by the retained dirty-clip allowlist');
  assert(windowSrc.includes('RenderDevTools(canvas') &&
         windowSrc.indexOf('RenderDevTools(canvas') > windowSrc.indexOf('retained_main_surface_->makeImageSnapshot()'),
    'DevTools overlay must be painted after retained-main copy');
  assert(domObserverSrc.includes('SetNeedsRepaintFor(RepaintReason::PseudoClass)'),
    'Pseudo-class changes must trigger a real repaint reason');
  assert(mouseSrc.includes('SetNeedsRepaintFor(RepaintReason::MouseHover)') &&
         mouseSrc.includes('SetNeedsRepaintFor(RepaintReason::MouseButton)'),
    'Mouse hover and button paths must be attributed separately for retained-present diagnosis');

  console.log('[PASS] retained-present default safety guard is present');
}

run();
