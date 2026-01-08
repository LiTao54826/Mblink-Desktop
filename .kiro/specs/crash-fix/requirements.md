# 崩溃修复需求

## 问题描述
在 `Window::Render` 中调用 `MarkRenderObjectsDirty` 时发生崩溃，崩溃位置在 `RenderObject::MarkAncestorsWithChildNeedsLayout`，访问 `parent_` weak_ptr 时触发 Access Violation。

## 崩溃调用栈
```
Window::Render (line 1028)
  -> MarkRenderObjectsDirty (line 1434)
    -> layout_engine_->UpdateStyle (line 1349)
      -> NativeLayoutEngine::UpdateStyle (line 855)
        -> AddElement (line 1356)
          -> render_obj->MarkNeedsLayout
            -> MarkAncestorsWithChildNeedsLayout
              -> parent_.use_count() CRASH (0xffffffffffffffff)
```

## 根本原因分析
1. `InvalidateRenderTree()` 被调用后，`cached_render_tree_` 被 reset
2. `EnsureRenderTree()` 重建新的渲染树
3. 但 `NativeLayoutEngine` 的 `render_to_node_` 映射中仍然保留了旧的 RenderObject 指针
4. 当 `UpdateStyle` 找不到 RenderObject 时，调用 `AddElement` 添加
5. `AddElement` 遍历祖先节点时，访问了已销毁的 RenderObject

## 需求
1. 确保 `InvalidateRenderTree` 时同步清理 `NativeLayoutEngine` 的映射
2. 确保不会访问已销毁的 RenderObject
3. 修复后运行 20 次以上不崩溃

## 验收标准
- `build\bin\Release\esm_loader.exe examples\component_demo\app.js` 连续运行 20 次无崩溃
