# Known Issues | 已知问题

## Open Issues | 待跟踪问题

### Wheel scrolling near bottom edge may be inconsistent | 滚轮滚动到底部附近可能不一致

Related source locations | 相关源码位置：

- `core/render/objects/render_object.cpp`
- `core/compositor/scroll_layer_manager.cpp`
- `core/event/dispatch/wheel_event_dispatcher.cpp`

Current status | 当前状态：

- issue record still exists in repository materials
  仓库材料中仍保留该问题记录
- current codebase still needs re-verification for a confirmed fix
  当前代码库仍需重新验证是否已完全修复

## Issue Entry Guidelines | 问题记录规范

Each issue entry should include:
每条 issue 建议包含：

- observed behavior / 现象
- minimal reproduction / 最小复现
- impact scope / 影响范围
- related source locations / 相关源码位置
- verification status / 验证状态
