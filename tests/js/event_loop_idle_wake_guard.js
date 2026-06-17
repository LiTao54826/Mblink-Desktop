// Regression check: the outer UI event loop should not spin at ~250Hz
// when no timers, events, repaint, or user frame callback are pending.

const fs = require('fs');
const path = require('path');

function assert(cond, msg) {
  if (!cond) throw new Error(msg);
}

function extractFunction(src, signature) {
  const start = src.indexOf(signature);
  assert(start >= 0, `Missing function ${signature}`);
  const bodyStart = src.indexOf('{', start);
  let depth = 0;
  for (let i = bodyStart; i < src.length; i++) {
    if (src[i] === '{') depth++;
    if (src[i] === '}') {
      depth--;
      if (depth === 0) return src.slice(start, i + 1);
    }
  }
  throw new Error(`Could not extract function ${signature}`);
}

function run() {
  const eventLoopPath = path.join(process.cwd(), 'core', 'event', 'loop', 'event_loop.cpp');
  const schedulerPath = path.join(process.cwd(), 'core', 'event', 'loop', 'task_scheduler.cpp');
  const uiDevRuntimePath = path.join(process.cwd(), 'tools', 'esm_loader', 'ui_dev_runtime_support.cpp');

  const eventLoopSrc = fs.readFileSync(eventLoopPath, 'utf8');
  const schedulerSrc = fs.readFileSync(schedulerPath, 'utf8');
  const uiDevRuntimeSrc = fs.readFileSync(uiDevRuntimePath, 'utf8');

  const clamp = extractFunction(eventLoopSrc, 'Sint32 ClampIdleDelayMs');
  assert(!clamp.includes('kDefaultIdleDelayMs = 4'),
    'No-work idle delay must not default to 4ms');
  assert(clamp.includes('kDefaultIdleDelayMs = 64'),
    'No-work idle delay should use the documented 64ms default');

  const runOnce = extractFunction(eventLoopSrc, 'void EventLoop::RunOnce');
  assert(runOnce.includes('CollectIdleWorkState(') &&
         runOnce.includes('WaitForIdleWork(') &&
         runOnce.includes('did_front_idle_wait'),
    'RunOnce must front-load the idle wait before the full frame pass');
  assert(runOnce.includes('idle_callback_') &&
         runOnce.includes('!idle_state.has_frame_deadline_work'),
    'RunOnce must only return early when idle and not waiting for rAF');
  assert(!runOnce.includes('MillisecondsUntilNextTask()') ||
         !runOnce.includes('SDL_WaitEventTimeout(nullptr, ClampIdleDelayMs(next_delay_ms))'),
    'RunOnce should stop using the old tail-only idle sleep path');

  const collectIdle = extractFunction(eventLoopSrc, 'IdleWorkState CollectIdleWorkState');
  assert(collectIdle.includes('WindowManagerHasPendingUiTasks()') &&
         collectIdle.includes('WindowManagerHasActiveAnimations()') &&
         collectIdle.includes('WindowManagerNeedsQuitCheck()'),
    'Idle state must account for UI tasks, animations, and window lifecycle checks');

  const schedulerReady = extractFunction(schedulerSrc, 'bool TaskScheduler::HasReadyTasks');
  assert(!schedulerReady.includes('HasPendingAnimationFrames()'),
    'requestAnimationFrame should be a frame-deadline wake source, not immediate busy work');

  const windowSrc = fs.readFileSync(path.join(process.cwd(), 'core', 'window', 'window.cpp'), 'utf8');
  const postUiTask = extractFunction(windowSrc, 'void Window::PostUiTask');
  assert(postUiTask.includes('ui_task_wake_callback_') &&
         postUiTask.includes('wake_callback'),
    'PostUiTask must wake the event loop after enqueuing UI work');
  assert(windowSrc.includes('bool Window::HasPendingUiTasks() const'),
    'Window must expose pending UI task state to the idle loop');

  assert(uiDevRuntimeSrc.includes('SetUpdateCallback(') &&
         uiDevRuntimeSrc.includes('needs_frame_cadence') &&
         uiDevRuntimeSrc.includes('quit_after_seconds > 0'),
    'UI-dev runtime support should not force per-frame polling for command files');

  console.log('[PASS] Event loop idle wake guard');
}

run();
