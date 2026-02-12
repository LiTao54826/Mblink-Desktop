// 回归检查：渲染树重建后滚动位置恢复必须走 ScrollTo 路径
// 目的：避免仅 SetScrollX/SetScrollY 导致已滚动容器在 resize/maximize/restore 后坐标缓存不同步
// 用法：node tests/js/window_scroll_restore_path_check.js

const fs = require('fs');
const path = require('path');

function assert(cond, msg) {
  if (!cond) throw new Error(msg);
}

function run() {
  const filePath = path.join(process.cwd(), 'core', 'window', 'window_renderer.cpp');
  const src = fs.readFileSync(filePath, 'utf8');

  const hasRestore = src.includes('void WindowRenderer::RestoreScrollPositions');
  assert(hasRestore, '未找到 RestoreScrollPositions 实现');

  // 必须使用 ScrollTo 恢复，确保 clamp + 缓存失效 + 重绘传播在同一路径
  assert(src.includes('render_obj->ScrollTo('),
    'RestoreScrollPositions 未使用 ScrollTo，可能导致滚动容器坐标缓存不同步');

  // 避免回退到分离的 SetScrollX/SetScrollY 恢复路径
  assert(!src.includes('render_obj->SetScrollX(') && !src.includes('render_obj->SetScrollY('),
    '检测到 SetScrollX/SetScrollY 恢复路径，存在回归风险');

  console.log('[PASS] 滚动位置恢复路径回归检查通过 (ScrollTo)');
}

run();

