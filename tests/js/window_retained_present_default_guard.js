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

  assert(windowSrc.includes('MBLINK_DISABLE_RETAINED_PRESENT'),
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
  for (const gate of [
    '!had_pending_dom_changes',
    '!render_tree_rebuild_required',
    '!needs_layout_update'
  ]) {
    assert(reuseBlock.includes(gate), `Retained-main reuse must be blocked by ${gate}`);
  }
  assert(windowSrc.includes('bool needs_dom_raster_update = false') &&
         windowSrc.includes('needs_dom_raster_update = true') &&
         windowSrc.includes('(needs_layout_update || needs_dom_raster_update) && render_pipeline_') &&
         windowSrc.includes('render_pipeline_->ForceRasterize()'),
    'DOM/style/text batches must force pipeline rasterization even when layout does not rebuild');
  assert(windowSrc.includes('can_update_retained_dirty_region') &&
         windowSrc.includes('UnionDirtyRects') &&
         windowSrc.includes('ComputeRetainedDirtyBounds') &&
         windowSrc.includes('retained_dirty_bounds.logical') &&
         windowSrc.includes('retained_dirty_bounds.physical'),
    'Dirty retained-present frames must derive all dirty bounds from a shared pixel-aligned rectangle');
  const dirtyReasonStart = windowSrc.indexOf('inline bool CanUseRetainedDirtyClipForReason');
  const dirtyReasonEnd = windowSrc.indexOf('inline bool HasExplicitDirtyRectsForUnknownReason', dirtyReasonStart);
  const dirtyReasonBlock = windowSrc.slice(dirtyReasonStart, dirtyReasonEnd);
  assert(dirtyReasonStart >= 0 &&
         dirtyReasonBlock.includes('case RepaintReason::DOMMutation:') &&
         dirtyReasonBlock.includes('case RepaintReason::KeyboardInput:') &&
         dirtyReasonBlock.includes('case RepaintReason::WheelScroll:') &&
         dirtyReasonBlock.includes('case RepaintReason::Terminal:'),
    'Dirty retained-present clipping must keep non-interaction repaint reasons eligible');
  for (const reason of ['PseudoClass', 'Focus', 'MouseHover', 'MouseButton']) {
    assert(dirtyReasonBlock.includes(`case RepaintReason::${reason}:`),
      `${reason} must be handled explicitly in retained dirty-clip allowlist`);
    const caseIndex = dirtyReasonBlock.indexOf(`case RepaintReason::${reason}:`);
    const nextReturnTrue = dirtyReasonBlock.indexOf('return true;', caseIndex);
    const nextReturnFalse = dirtyReasonBlock.indexOf('return false;', caseIndex);
    assert(nextReturnTrue >= 0 && (nextReturnFalse < 0 || nextReturnTrue < nextReturnFalse),
      `${reason} must stay eligible for retained dirty clipping; interaction repaint is too frequent for full redraw fallback`);
  }
  assert(windowSrc.includes('struct RetainedDirtyBounds') &&
         windowSrc.includes('ComputeRetainedDirtyBounds') &&
         windowSrc.includes('ClampPhysicalRect(physical_dirty') &&
         windowSrc.includes('bounds.logical = SkRect::MakeLTRB(') &&
         windowSrc.includes('bounds.present_physical = SkRect::Make(bounds.physical)'),
    'Retained dirty clipping must normalize dirty bounds through one physical pixel-aligned rectangle');
  const dirtyClipStart = windowSrc.indexOf('const bool retained_dirty_clip_allowed =');
  const dirtyClipEnd = windowSrc.indexOf(';', dirtyClipStart);
  const dirtyClipBlock = windowSrc.slice(dirtyClipStart, dirtyClipEnd);
  assert(dirtyClipStart >= 0 &&
         windowSrc.includes('const bool retained_dirty_reason_allowed =') &&
         windowSrc.includes('HasExplicitDirtyRectsForUnknownReason(last_repaint_reason_, has_dirty_bounds)') &&
         dirtyClipBlock.includes('retained_dirty_reason_allowed') &&
         dirtyClipBlock.includes('!had_pending_dom_changes') &&
         dirtyClipBlock.includes('!had_structural_dom_changes') &&
         dirtyClipBlock.includes('can_use_structural_dirty_rects') &&
         dirtyClipBlock.includes('!render_tree_rebuild_required') &&
         dirtyClipBlock.includes('!needs_layout_update') &&
         dirtyClipBlock.includes('!dirty_union_too_broad'),
    'Dirty retained-present clipping must stay blocked for unsafe structural/tree rebuild/layout update frames');
  assert(windowSrc.includes('AddRetainedDirtyRectsForPendingChanges(this, tracker)') &&
         windowSrc.includes('AddRetainedDirtyRectsForPaintDirtyTree(this, cached_render_tree_.get(), app_width, app_height)'),
    'Non-structural DOM batches must collect old and new dirty bounds for retained-present clipping');
  const updateStart = windowSrc.indexOf('const bool can_update_retained_dirty_region =');
  const updateEnd = windowSrc.indexOf(';', updateStart);
  const updateBlock = windowSrc.slice(updateStart, updateEnd);
  assert(updateStart >= 0 && updateBlock.includes('retained_dirty_clip_allowed'),
    'Dirty retained-present update must be gated by the retained dirty-clip allowlist');
  const rasterLimitStart = windowSrc.indexOf('const bool can_limit_pipeline_raster_to_dirty_rects =');
  const rasterLimitEnd = windowSrc.indexOf(';', rasterLimitStart);
  const rasterLimitBlock = windowSrc.slice(rasterLimitStart, rasterLimitEnd);
  assert(rasterLimitStart >= 0 &&
         rasterLimitBlock.includes('retained_dirty_reason_allowed') &&
         rasterLimitBlock.includes('!had_pending_dom_changes') &&
         rasterLimitBlock.includes('!had_structural_dom_changes') &&
         rasterLimitBlock.includes('can_use_structural_dirty_rects') &&
         rasterLimitBlock.includes('!render_tree_rebuild_required') &&
         rasterLimitBlock.includes('!needs_layout_update'),
    'Pipeline raster dirty-rect limiting must be disabled for unsafe structural/tree rebuild/layout update frames');
  assert(windowSrc.includes('std::vector<SkRect>{retained_dirty_bounds.logical}') &&
         windowSrc.includes('can_limit_pipeline_raster_to_dirty_rects ? &retained_pipeline_dirty_rects : nullptr'),
    'Pipeline raster dirty-rect limiting must use the same pixel-aligned logical dirty bounds as retained-present clipping');
  assert(windowSrc.includes('const SkRect dirty_bounds_px = SkRect::Make(retained_dirty_bounds.physical)') &&
         windowSrc.includes('const SkRect present_dirty_bounds_px = retained_dirty_bounds.present_physical') &&
         windowSrc.includes('main_canvas->clipRect(dirty_bounds_px, SkClipOp::kIntersect, false)') &&
         windowSrc.includes('main_canvas->drawRect(dirty_bounds_px, clear_paint)') &&
         windowSrc.includes('const SkRect dirty_bounds = retained_dirty_bounds.logical') &&
         windowSrc.includes('main_canvas->clipRect(dirty_bounds, SkClipOp::kIntersect, false)') &&
         windowSrc.includes('ClampPhysicalRect(') &&
         windowSrc.includes('dirty_bounds_px, retained_image->width(), retained_image->height()') &&
         windowSrc.includes('last_dirty_bounds_ = present_dirty_bounds_px'),
    'Retained dirty clear, clip, copy, and present bounds must share the same hard pixel-aligned rectangle');
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
