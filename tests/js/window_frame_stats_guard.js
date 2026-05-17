// Regression check: baseline frame stats must expose full-frame timing and repaint reasons.
// Usage: node tests/js/window_frame_stats_guard.js

const fs = require('fs');
const path = require('path');

function assert(cond, msg) {
  if (!cond) throw new Error(msg);
}

function run() {
  const eventLoopPath = path.join(process.cwd(), 'core', 'event', 'loop', 'event_loop.cpp');
  const windowPath = path.join(process.cwd(), 'core', 'window', 'window.cpp');
  const windowHeaderPath = path.join(process.cwd(), 'core', 'window', 'window.h');
  const eventLoopSrc = fs.readFileSync(eventLoopPath, 'utf8');
  const windowSrc = fs.readFileSync(windowPath, 'utf8');
  const windowHeader = fs.readFileSync(windowHeaderPath, 'utf8');

  assert(windowHeader.includes('enum class RepaintReason'),
    'Window must define coarse repaint reason buckets');
  assert(windowHeader.includes('SetNeedsRepaintFor(RepaintReason reason)'),
    'Window must expose tagged repaint requests without removing SetNeedsRepaint compatibility');
  assert(windowHeader.includes('GetLastRepaintReasonName()') &&
         windowHeader.includes('GetRepaintReasonCount()'),
    'Window must expose repaint reason name and repeat count for frame logs');

  assert(eventLoopSrc.includes('boundary=event_loop') &&
         eventLoopSrc.includes('class=window_frame'),
    'EventLoop::Render must emit per-window full-frame baseline records');
  for (const field of [
    'render_ms=',
    'swap_ms=',
    'render_plus_swap_ms=',
    'needs_repaint_before=',
    'needs_repaint_after=',
    'repaint_reason=',
    'repaint_reason_count='
  ]) {
    assert(eventLoopSrc.includes(field), `Event loop frame stats must include ${field}`);
  }
  assert(eventLoopSrc.includes('class=render_callback') &&
         eventLoopSrc.includes('loop_render_callback_ms='),
    'Render callback timing must stay separate from per-window render plus swap timing');

  for (const field of [
    'animation_probe_ms=',
    'viewport_ms=',
    'pipeline_init_ms=',
    'size_check_ms=',
    'ensure_tree_ms=',
    'dirty_sync_ms=',
    'style_dirty_scan_ms=',
    'layout_ms=',
    'animation_update_ms=',
    'pending_animation_retry_count=',
    'pending_animation_retry_exhausted='
  ]) {
    assert(windowSrc.includes(field), `Window frame stats must include ${field}`);
  }

  assert(eventLoopSrc.includes('MarkElementPaintDirty') &&
         eventLoopSrc.includes('window->AddDirtyRect(dirty_rect)') &&
         eventLoopSrc.includes('window->SetNeedsRepaintFor(RepaintReason::Focus)'),
    'Native caret blink must mark the focused input dirty before requesting focus repaint');

  console.log('[PASS] full-frame baseline stats and repaint reason guard is present');
}

run();
