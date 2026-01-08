# 崩溃修复设计

## 问题根源
`NativeLayoutEngine` 持有 `render_to_node_` 映射（`RenderObject*` -> `NodeId`），当渲染树重建时，旧的 RenderObject 被销毁，但映射中的指针变成悬空指针。

## 修复方案

### 方案 1：在 InvalidateRenderTree 时清理 LayoutEngine（推荐）
在 `Window::InvalidateRenderTree()` 中调用 `layout_engine_->Clear()`，确保映射被清空。

优点：简单直接，从源头解决问题
缺点：无

### 方案 2：在 EnsureRenderTree 开始时清理
在 `EnsureRenderTree()` 重建渲染树前，先清理 LayoutEngine。

优点：集中管理
缺点：可能遗漏其他调用路径

### 方案 3：防御性编程
在所有访问 RenderObject 的地方添加有效性检查。

优点：更健壮
缺点：性能开销，治标不治本

## 选择方案 1

### 实现步骤
1. 在 `Window::InvalidateRenderTree()` 中添加 `layout_engine_->Clear()` 调用
2. 移除之前添加的 `use_count()` 检查（不需要了）
3. 编译测试
4. 运行 20 次验证
