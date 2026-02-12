// 窗口最大化/还原 WM_SIZE 去抖逻辑回归检查
// 目的：防止真实状态切换事件被误拦截，导致滚动容器坐标不更新
// 用法：node tests/js/window_wm_size_resize_filter_check.js

const fs = require('fs');
const path = require('path');

function assert(cond, msg) {
  if (!cond) throw new Error(msg);
}

function run() {
  const filePath = path.join(process.cwd(), 'core', 'window', 'window_win32.cpp');
  const src = fs.readFileSync(filePath, 'utf8');

  // 旧逻辑：100ms 内超过 2 次 WM_SIZE 就直接拦截（过于激进）
  assert(!src.includes('if (size_count > 2)'),
    '检测到旧 WM_SIZE 过滤阈值(size_count>2)，可能误拦截 maximize/restore');

  // 新逻辑应识别窗口状态变化并放行
  assert(src.includes('SIZE_MAXIMIZED') && src.includes('SIZE_RESTORED') && src.includes('SIZE_MINIMIZED'),
    '未检测到 maximize/restore/minimize 状态识别逻辑');

  // 新逻辑应基于“短时间 + 完全相同尺寸事件”去重
  assert(src.includes('same_size_event') && src.includes('last_width') && src.includes('last_height'),
    '未检测到同尺寸重复 WM_SIZE 去重逻辑');

  // 新逻辑应使用更保守阈值，避免吞掉真实 resize
  assert(src.includes('if (size_count > 6)'),
    '未检测到保守阈值(size_count>6)');

  console.log('[PASS] WM_SIZE 窗口状态切换过滤回归检查通过');
}

run();

