# 崩溃修复任务

## 任务列表

- [x] 1. 在 InvalidateRenderTree 中清理 LayoutEngine
- [x] 2. 修复 MarkNeedsLayout 中的 MarkAncestorsWithChildNeedsLayout 调用时机
- [x] 3. 编译 Release 版本
- [x] 4. 运行 20 次稳定性测试 - 全部通过

## 已排除的方向
- use_count() 检查无效：weak_ptr 本身内存已损坏，use_count() 也会崩溃
- 仅 reset cached_render_tree_ 无效：问题在 NativeLayoutEngine 的映射

## 修复内容
1. `Window::InvalidateRenderTree()` 中添加 `layout_engine_->Clear()` 调用
2. `RenderObject::MarkNeedsLayout()` 中将 `MarkAncestorsWithChildNeedsLayout()` 移到 `if (propagate_to_parent)` 块内

## 根本原因
当 `MarkNeedsLayout(false)` 被调用时，`propagate_to_parent` 为 false，但 `MarkAncestorsWithChildNeedsLayout()` 仍然会被调用，导致访问可能无效的 `parent_` weak_ptr。
