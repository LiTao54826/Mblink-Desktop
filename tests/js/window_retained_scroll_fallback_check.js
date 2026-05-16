// Regression check: retained-present must not reuse main content across scroll full-dirty fallbacks.
// Usage: node tests/js/window_retained_scroll_fallback_check.js

const fs = require('fs');
const path = require('path');

function assert(cond, msg) {
  if (!cond) throw new Error(msg);
}

function run() {
  const windowPath = path.join(process.cwd(), 'core', 'window', 'window.cpp');
  const pipelineHeaderPath = path.join(process.cwd(), 'core', 'render', 'pipeline', 'render_pipeline.h');
  const windowSrc = fs.readFileSync(windowPath, 'utf8');
  const pipelineHeader = fs.readFileSync(pipelineHeaderPath, 'utf8');

  assert(pipelineHeader.includes('bool HasPendingScrollFullDirtyFallback() const;'),
    'RenderPipeline must expose pending scroll full-dirty fallback state before ProcessFrame');

  assert(windowSrc.includes('render_pipeline_->HasPendingScrollFullDirtyFallback()'),
    'Window::Render must check pending scroll full-dirty fallback before retained-main reuse');

  assert(windowSrc.includes('GetLastFrameStats().scroll_full_dirty_fallbacks > 0'),
    'Window::Render must also guard against the previous rendered scroll fallback frame');

  assert(windowSrc.includes('retained_main_has_content_ = false;') &&
         windowSrc.includes('retained_main_scroll_fallback_invalidated'),
    'Window::Render must invalidate retained main content when a scroll fallback is observed');

  const reuseDecl = windowSrc.indexOf('const bool can_reuse_retained_main =');
  const reuseEnd = windowSrc.indexOf(';', reuseDecl);
  const reuseBlock = windowSrc.slice(reuseDecl, reuseEnd);
  assert(reuseDecl >= 0 &&
         reuseBlock.includes('!retained_main_scroll_fallback_blocked') &&
         reuseBlock.includes('!render_pipeline_->NeedsUpdate()') &&
         reuseBlock.indexOf('!retained_main_scroll_fallback_blocked') <
           reuseBlock.indexOf('!render_pipeline_->NeedsUpdate()'),
    'retained-main reuse must be blocked by scroll fallback state before NeedsUpdate is considered');

  console.log('[PASS] retained-present scroll fallback guard is present');
}

run();
