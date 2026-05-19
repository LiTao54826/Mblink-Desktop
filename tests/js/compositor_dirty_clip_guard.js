// Regression check: retained dirty frames must composite only the clipped root bitmap region.
// Usage: node tests/js/compositor_dirty_clip_guard.js

const fs = require('fs');
const path = require('path');

function assert(cond, msg) {
  if (!cond) throw new Error(msg);
}

function run() {
  const compositorHeaderPath = path.join(process.cwd(), 'core', 'compositor', 'compositor.h');
  const compositorPath = path.join(process.cwd(), 'core', 'compositor', 'compositor.cpp');
  const pipelineHeaderPath = path.join(process.cwd(), 'core', 'render', 'pipeline', 'render_pipeline.h');
  const pipelinePath = path.join(process.cwd(), 'core', 'render', 'pipeline', 'render_pipeline.cpp');
  const compositorHeader = fs.readFileSync(compositorHeaderPath, 'utf8');
  const compositorSrc = fs.readFileSync(compositorPath, 'utf8');
  const pipelineHeader = fs.readFileSync(pipelineHeaderPath, 'utf8');
  const pipelineSrc = fs.readFileSync(pipelinePath, 'utf8');

  assert(compositorHeader.includes('CompositeToCanvas(CompositorLayer* root, SkCanvas* canvas, const SkRect* logical_clip') &&
         compositorHeader.includes('CompositeLayerCPU(CompositorLayer* layer') &&
         compositorHeader.includes('const SkRect* logical_clip'),
    'Compositor must accept an optional logical clip for dirty retained-present frames');
  assert(compositorSrc.includes('can_draw_clipped_region') &&
         compositorSrc.includes('transform_is_clip_compatible') &&
         compositorSrc.includes('drawImageRect') &&
         compositorSrc.includes('SkCanvas::kFast_SrcRectConstraint'),
    'CPU compositor must draw only the bitmap source rect intersecting the dirty clip');
  assert(pipelineHeader.includes('ProcessFrame(SkCanvas* canvas, const SkRect* logical_clip') &&
         pipelineHeader.includes('DoComposite(SkCanvas* canvas, const SkRect* logical_clip'),
    'RenderPipeline must thread the optional logical clip through the frame');
  assert(pipelineSrc.includes('DoComposite(canvas, logical_clip)') &&
         pipelineSrc.includes('CompositeToCanvas(root_layer_.get(), canvas, logical_clip)'),
    'RenderPipeline must pass the logical clip into compositor composition');
  const processFrameStart = pipelineSrc.indexOf('bool RenderPipeline::ProcessFrame(');
  const processFrameEnd = pipelineSrc.indexOf('void RenderPipeline::EnsureRenderTree()', processFrameStart);
  const processFrameSrc = pipelineSrc.slice(processFrameStart, processFrameEnd);
  assert(!/external_root_dirty_rects_[\s\S]{0,300}ClearDirtyRegions/.test(processFrameSrc),
    'RenderPipeline must not replace internally collected root dirty regions with external dirty hints');
  const forceRasterizeStart = pipelineSrc.indexOf('void RenderPipeline::ForceRasterize()');
  const forceRasterizeEnd = pipelineSrc.indexOf('void RenderPipeline::MarkDirty(', forceRasterizeStart);
  const forceRasterizeSrc = pipelineSrc.slice(forceRasterizeStart, forceRasterizeEnd);
  assert(forceRasterizeStart >= 0 &&
         forceRasterizeSrc.includes('MarkAllLayersDirty(root_layer_.get())') &&
         !forceRasterizeSrc.includes('root_layer_->MarkFullDirty()'),
    'ForceRasterize must mark every compositor layer dirty, not only the root layer');

  console.log('[PASS] compositor dirty clip guard is present');
}

run();
