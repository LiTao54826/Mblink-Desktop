// Regression check: CPU partial-present dirty bounds must enclose float paint bounds.
// Usage: node tests/js/window_partial_present_dirty_rect_guard.js

const fs = require('fs');
const path = require('path');

function assert(cond, msg) {
  if (!cond) throw new Error(msg);
}

function run() {
  const windowPath = path.join(process.cwd(), 'core', 'window', 'window.cpp');
  const windowSrc = fs.readFileSync(windowPath, 'utf8');

  assert(windowSrc.includes('SkIRect ClampPhysicalRect(const SkRect& rect, int width, int height)'),
    'Window must keep a shared helper for enclosing and clamping physical dirty rects');
  assert(windowSrc.includes('std::floor(rect.left())') &&
         windowSrc.includes('std::floor(rect.top())') &&
         windowSrc.includes('std::ceil(rect.right())') &&
         windowSrc.includes('std::ceil(rect.bottom())'),
    'ClampPhysicalRect must floor left/top and ceil right/bottom to cover antialiased edge pixels');

  const swapStart = windowSrc.indexOf('void Window::SwapBuffers()');
  const swapEnd = windowSrc.indexOf('void Window::OnResize()', swapStart);
  const swapBlock = windowSrc.slice(swapStart, swapEnd);
  assert(swapStart >= 0 && swapEnd > swapStart,
    'Window::SwapBuffers block must be discoverable');
  assert(swapBlock.includes('SkIRect dirty_rect = ClampPhysicalRect(last_dirty_bounds_, surface_width, surface_height)') &&
         swapBlock.includes('if (!dirty_rect.isEmpty())'),
    'CPU partial-present path must enclose float dirty bounds before PresentPartial');
  assert(!swapBlock.includes('static_cast<int>(last_dirty_bounds_.width())') &&
         !swapBlock.includes('static_cast<int>(last_dirty_bounds_.height())'),
    'CPU partial-present path must not truncate dirty width/height');

  const retainedCopyStart = windowSrc.indexOf('const bool can_partial_retained_copy');
  const retainedCopyEnd = windowSrc.indexOf('partial_retained_copy_used = true', retainedCopyStart);
  const retainedCopyBlock = windowSrc.slice(retainedCopyStart, retainedCopyEnd);
  assert(retainedCopyStart >= 0 && retainedCopyEnd > retainedCopyStart,
    'Window retained partial-copy block must be discoverable');
  assert(retainedCopyBlock.includes('SkSamplingOptions(), nullptr') &&
         retainedCopyBlock.includes('SkCanvas::kStrict_SrcRectConstraint'),
    'CPU retained partial copy must use strict unfiltered source rect blits');

  const retainedUpdateStart = windowSrc.indexOf('if (can_update_retained_dirty_region)');
  const processFrameStart = windowSrc.indexOf('process_ok = render_pipeline_->ProcessFrame', retainedUpdateStart);
  const retainedUpdateBlock = windowSrc.slice(retainedUpdateStart, processFrameStart);
  assert(retainedUpdateStart >= 0 && processFrameStart > retainedUpdateStart,
    'Window retained dirty update block must be discoverable');
  assert(retainedUpdateBlock.includes('main_canvas->clipRect(dirty_bounds_px, SkClipOp::kIntersect, false)') &&
         retainedUpdateBlock.includes('main_canvas->clipRect(dirty_bounds, SkClipOp::kIntersect, false)'),
    'Retained dirty-region clear and redraw clips must be hard rect clips, not antialiased clips');
  assert(!retainedUpdateBlock.includes('main_canvas->clipRect(dirty_bounds_px, SkClipOp::kIntersect, true)') &&
         !retainedUpdateBlock.includes('main_canvas->clipRect(dirty_bounds, SkClipOp::kIntersect, true)'),
    'Retained dirty-region clips must not leave antialiased clip-edge pixels');

  console.log('[PASS] CPU partial-present dirty rect guard is present');
}

run();
