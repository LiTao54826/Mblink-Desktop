// DevTools BoxModel 高亮滚动偏移回归检查
// 目的：防止高亮叠加目标元素自身 scroll 偏移，导致区域错位。
// 用法：node tests/js/devtools_box_model_scroll_offset_check.js

const fs = require('fs');
const path = require('path');

function assert(cond, msg) {
  if (!cond) throw new Error(msg);
}

function run() {
  const filePath = path.join(process.cwd(), 'core', 'devtools', 'inspector', 'element_highlighter.cpp');
  const src = fs.readFileSync(filePath, 'utf8');

  const hasLegacyX = src.includes('float x = result.abs_x - result.render_obj->GetScrollX();');
  const hasLegacyY = src.includes('float y = result.abs_y - result.render_obj->GetScrollY();');
  assert(!hasLegacyX && !hasLegacyY,
    '检测到旧逻辑：RenderBoxModelHighlight 仍在扣减目标元素自身滚动偏移');

  const hasExpectedX = src.includes('float x = result.abs_x;');
  const hasExpectedY = src.includes('float y = result.abs_y;');
  assert(hasExpectedX && hasExpectedY,
    '未检测到新逻辑：RenderBoxModelHighlight 应直接使用目标 border box 绝对坐标');

  const hasAncestorScrollTraverse = src.includes('float child_offset_x = current_x - obj->GetScrollX();') &&
    src.includes('float child_offset_y = current_y - obj->GetScrollY();');
  assert(hasAncestorScrollTraverse,
    '祖先滚动扣减逻辑缺失：子元素坐标将无法跟随外层滚动正确映射');

  console.log('[PASS] DevTools BoxModel 高亮滚动偏移回归检查通过');
}

run();

