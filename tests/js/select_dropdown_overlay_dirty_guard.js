// Regression check: select popup overlays must invalidate their old paint bounds.
// Usage: node tests/js/select_dropdown_overlay_dirty_guard.js

const fs = require('fs');
const path = require('path');

function assert(cond, msg) {
  if (!cond) throw new Error(msg);
}

function run() {
  const dropdownPath = path.join(process.cwd(), 'core', 'render', 'objects', 'select_dropdown.cpp');
  const src = fs.readFileSync(dropdownPath, 'utf8');

  assert(src.includes('SkRect DropdownPaintBounds(const SelectDropdownInfo& info)') &&
         src.includes('kDropdownDirtyOutset'),
    'Select dropdown overlay must compute a conservative paint dirty bound');
  assert(src.includes('void QueueDropdownRepaint(const SelectDropdownInfo& info, RepaintReason reason)') &&
         src.includes('owner_window->AddDirtyRect(dirty_bounds)') &&
         src.includes('owner_window->SetNeedsRepaintFor(reason)'),
    'Select dropdown overlay must enqueue a window repaint for popup paint bounds');

  const openStart = src.indexOf('void SelectDropdownManager::OpenDropdown');
  const closeStart = src.indexOf('void SelectDropdownManager::CloseDropdown');
  const updateStart = src.indexOf('void SelectDropdownManager::UpdatePosition');
  const getActiveStart = src.indexOf('std::shared_ptr<HTMLSelectElement> SelectDropdownManager::GetActiveSelect');
  assert(openStart >= 0 && closeStart > openStart && updateStart > closeStart,
    'Select dropdown lifecycle blocks must be discoverable');

  const openBlock = src.slice(openStart, closeStart);
  const closeBlock = src.slice(closeStart, getActiveStart);
  const updateEnd = src.indexOf('void SelectDropdownManager::UpdatePositionFromRenderTree', updateStart);
  const updateBlock = src.slice(updateStart, updateEnd);

  assert(openBlock.includes('QueueDropdownRepaint(current_dropdown_, RepaintReason::MouseButton)'),
    'Opening a new select popup must dirty the old popup overlay first');
  assert(closeBlock.includes('QueueDropdownRepaint(current_dropdown_, RepaintReason::MouseButton)') &&
         closeBlock.indexOf('QueueDropdownRepaint(current_dropdown_, RepaintReason::MouseButton)') <
           closeBlock.indexOf('current_dropdown_ = {}'),
    'Closing a select popup must dirty the old popup overlay before clearing state');
  assert(updateBlock.includes('SelectDropdownInfo previous_dropdown = current_dropdown_') &&
         updateBlock.includes('QueueDropdownRepaint(previous_dropdown, RepaintReason::MouseButton)') &&
         updateBlock.includes('QueueDropdownRepaint(current_dropdown_, RepaintReason::MouseButton)'),
    'Moving a select popup must dirty both previous and current overlay bounds');

  console.log('[PASS] select dropdown overlay dirty guard is present');
}

run();
