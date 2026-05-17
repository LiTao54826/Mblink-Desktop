// Regression check: inert animation declarations must not keep windows repaint-hot forever.
// Usage: node tests/js/window_animation_pending_guard.js

const fs = require('fs');
const path = require('path');

function assert(cond, msg) {
  if (!cond) throw new Error(msg);
}

function run() {
  const rendererPath = path.join(process.cwd(), 'core', 'window', 'window_renderer.cpp');
  const applicatorHeaderPath = path.join(process.cwd(), 'core', 'render', 'animation', 'animation_applicator.h');
  const applicatorPath = path.join(process.cwd(), 'core', 'render', 'animation', 'animation_applicator.cpp');
  const rendererSrc = fs.readFileSync(rendererPath, 'utf8');
  const applicatorHeader = fs.readFileSync(applicatorHeaderPath, 'utf8');
  const applicatorSrc = fs.readFileSync(applicatorPath, 'utf8');

  const fnStart = rendererSrc.indexOf('bool WindowRenderer::HasPendingAnimations');
  assert(fnStart >= 0, 'WindowRenderer::HasPendingAnimations must exist');
  const fnEnd = rendererSrc.indexOf('\nbool WindowRenderer::LayoutDirtySubtree', fnStart);
  assert(fnEnd > fnStart, 'WindowRenderer::HasPendingAnimations body must be bounded by LayoutDirtySubtree');
  const fnBody = rendererSrc.slice(fnStart, fnEnd);

  assert(fnBody.includes('applicator->HasPendingAnimationStartup(root)'),
    'WindowRenderer pending animation query must delegate to AnimationApplicator startup state');
  assert(!fnBody.includes('style.animations'),
    'WindowRenderer must not treat every animation declaration as pending');
  assert(!fnBody.includes('anim.IsValid() && !anim.name.empty() && anim.name != "none"'),
    'Declaration-only pending predicate must not return true forever');

  assert(applicatorHeader.includes('HasPendingAnimationStartup(RenderObject* object) const') &&
         applicatorHeader.includes('CountPendingAnimationStartupRetries(RenderObject* root) const') &&
         applicatorHeader.includes('CountExhaustedAnimationStartupRetries(RenderObject* root) const'),
    'AnimationApplicator must expose bounded pending startup queries');
  assert(applicatorHeader.includes('startup_retry_attempts_') &&
         applicatorHeader.includes('kMaxStartupRetryAttempts = 3'),
    'AnimationApplicator must keep a three-frame startup retry budget');
  assert(applicatorSrc.includes('retry_it->second >= kMaxStartupRetryAttempts') &&
         applicatorSrc.includes('++retry_attempts[anim.name]') &&
         applicatorSrc.includes('startup_retry_attempts_.clear()'),
    'AnimationApplicator must increment, exhaust, and clear startup retry state');
  assert(applicatorSrc.includes('PruneStaleStartupRetries') &&
         applicatorSrc.includes('CollectElementsInTree') &&
         applicatorSrc.includes('live_tree_elements'),
    'AnimationApplicator must prune stale startup retry state against the live render tree');

  console.log('[PASS] bounded animation startup pending guard is present');
}

run();
