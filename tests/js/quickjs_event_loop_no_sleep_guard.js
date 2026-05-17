// Regression check: QuickJS RunEventLoop must not sleep on the UI thread.
// Usage: node tests/js/quickjs_event_loop_no_sleep_guard.js

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
  const runtimePath = path.join(process.cwd(), 'core', 'quickjs', 'quickjs_runtime.cpp');
  const src = fs.readFileSync(runtimePath, 'utf8');
  const runEventLoop = extractFunction(src, 'void QuickJSRuntime::RunEventLoop');

  assert(!runEventLoop.includes('sleep_for('),
    'RunEventLoop must not block on future timers');
  assert(!runEventLoop.includes('SDL_Delay('),
    'RunEventLoop must not delay on the interactive UI path');
  assert(runEventLoop.includes('timer_queue_') &&
         runEventLoop.includes('timer_it->first > now'),
    'RunEventLoop must still keep future timers queued instead of spinning them');

  console.log('[PASS] QuickJS event loop does not sleep on the UI path');
}

run();
