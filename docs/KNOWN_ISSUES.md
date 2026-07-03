# Known Issues | 已知问题

This page tracks open runtime/tooling issues that should not be treated as fixed
until a minimal reproduction or targeted verification says so.

本页跟踪仍需复现或专项验证后才能关闭的运行时/工具问题。

## Open Issues | 待跟踪问题

### Wheel scrolling near bottom edge may be inconsistent | 滚轮滚动到底部附近可能不一致

Related source locations | 相关源码位置：

- `core/render/objects/render_object.cpp`
- `core/compositor/scroll_layer_manager.cpp`
- `core/event/dispatch/wheel_event_dispatcher.cpp`

Current status | 当前状态：

- The issue record still exists because there is no fresh minimal
  wheel-near-bottom reproduction proving the behavior is fully fixed.
  该问题仍保持打开，因为目前还没有新的最小复现或专项验证可以证明
  wheel-near-bottom 场景已经完全稳定。
- 2026-07-03 re-verification found no failure in the available scroll-related
  guards listed below. These checks are adjacent evidence only; they do not
  close or downgrade the specific wheel-near-bottom issue.
  2026-07-03 的复验中，下列可用的滚动相关 guard 没有失败。但这些只属于
  邻近证据，不能直接证明 wheel-near-bottom 这个特定问题已经修复。
  - `tests\regression\test_mblink_css_layout_stability.ps1`
  - `tests\regression\test_mblink_complex_layout_dynamics.ps1`
  - `tests\js\window_retained_scroll_fallback_check.js`
  - `tests\js\window_scroll_restore_path_check.js`
  - `tests\js\devtools_box_model_scroll_offset_check.js`
- Keep this issue open until a targeted wheel-near-bottom regression either
  reproduces the problem or proves the specific case stable.
  在有专项 wheel-near-bottom 回归测试复现问题或证明该特定场景稳定之前，
  继续保持该问题打开。

## Issue Entry Guidelines | 问题记录规范

Each issue entry should include:
每条 issue 建议包含：

- observed behavior / 现象
- minimal reproduction / 最小复现
- impact scope / 影响范围
- related source locations / 相关源码位置
- verification status / 验证状态
