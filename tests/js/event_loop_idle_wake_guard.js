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
  const runOnceNonBlocking = extractFunction(eventLoopSrc, 'void EventLoop::RunOnceNonBlocking');
  const runOnceInternal = extractFunction(eventLoopSrc, 'void EventLoop::RunOnceInternal');
  assert(runOnce.includes('RunOnceInternal(true)'),
    'EventLoop::RunOnce must use the blocking idle-wait path');
  assert(runOnceNonBlocking.includes('RunOnceInternal(false)'),
    'Manual poll path must have a non-blocking event-loop entrypoint');
  assert(runOnceInternal.includes('CollectIdleWorkState(') &&
         runOnceInternal.includes('WaitForIdleWork(') &&
         runOnceInternal.includes('did_front_idle_wait'),
    'RunOnceInternal must front-load the idle wait before the full frame pass');
  assert(runOnceInternal.includes('allow_idle_wait') &&
         runOnceInternal.includes('idle_callback_') &&
         runOnceInternal.includes('!idle_state.has_frame_deadline_work'),
    'RunOnceInternal must only block/return early when idle and not waiting for rAF');
  assert(runOnceInternal.includes('if (allow_idle_wait && !did_front_idle_wait)') &&
         runOnceInternal.includes('WaitForIdleWork(CollectIdleWorkState('),
    'RunOnceInternal should keep the tail idle sleep only for the blocking EventLoop::Run path');
  const cApiPath = path.join(process.cwd(), 'core', 'api', 'mblink.cpp');
  const cApiSrc = fs.readFileSync(cApiPath, 'utf8');
  const pollEvents = extractFunction(cApiSrc, 'bool mblink_poll_events');
  assert(pollEvents.includes('RunOnceNonBlocking()') && !pollEvents.includes('RunOnce();'),
    'mblink_poll_events must not perform the blocking idle wait used by EventLoop::Run');
  const waitEvents = extractFunction(cApiSrc, 'bool mblink_wait_events');
  assert(waitEvents.includes('RunOnce()') && !waitEvents.includes('RunOnceNonBlocking()'),
    'mblink_wait_events must expose the blocking idle-wait step for C API hosts');

  const esmLoaderSrc = fs.readFileSync(path.join(process.cwd(), 'tools', 'esm_loader', 'main_c_api.cpp'), 'utf8');
  assert(esmLoaderSrc.includes('while (mblink_wait_events(app))') &&
         !esmLoaderSrc.includes('while (mblink_poll_events(app))'),
    'esm_loader must use the blocking C API event step instead of a busy poll loop');
  assert(!esmLoaderSrc.includes('ui_dev_enabled ? 2 : 8'),
    'esm_loader must not hide idle spinning behind fixed millisecond sleeps');
  assert(esmLoaderSrc.includes('next_command_check') &&
         esmLoaderSrc.includes('std::chrono::milliseconds(250)'),
    'esm_loader command-file probing must be throttled when the runtime is idle');

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
