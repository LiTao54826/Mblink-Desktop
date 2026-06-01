// Regression check: CSS window-control clicks must not leak a half mouse sequence into DOM.
// Usage: node tests/js/window_control_mouse_sequence_guard.js

const fs = require('fs');
const path = require('path');

function assert(cond, msg) {
  if (!cond) throw new Error(msg);
}

function run() {
  const filePath = path.join(process.cwd(), 'core', 'window', 'window_win32.cpp');
  const src = fs.readFileSync(filePath, 'utf8');

  assert(src.includes('g_pressed_window_controls'),
    'Win32 window-control handling must remember the pressed control');
  assert(src.includes('case WM_LBUTTONDOWN:') &&
         src.includes('g_pressed_window_controls[hwnd] = control') &&
         src.includes('SetCapture(hwnd)') &&
         src.includes('return 0;'),
    'Window-control mousedown must be consumed before DOM mouse dispatch sees it');
  assert(src.includes('case WM_LBUTTONUP:') &&
         src.includes('GetCapture() == hwnd') &&
         src.includes('ReleaseCapture()') &&
         src.includes('released_control == pressed_control') &&
         src.includes('RunWindowControlAction(hwnd, window, released_control)'),
    'Window-control mouseup must only run the action after a matching captured press/release');
  assert(src.includes('g_pressed_window_controls.erase(hwnd);') &&
         src.includes('case WM_CAPTURECHANGED:') &&
         src.includes('void UnsubclassWindow(HWND hwnd)'),
    'Pressed window-control state must be cleaned up on capture loss and unsubclass');

  console.log('[PASS] Win32 window-control mouse sequence guard is present');
}

run();
