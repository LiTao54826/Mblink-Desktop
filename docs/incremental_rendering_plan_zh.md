# MBink 增量渲染实现计划

## 1. 目标
为 MBink 实现“企业级”的增量渲染性能。核心目标是通过仅处理实际变化或可见的元素，最小化 CPU 使用率和绘制命令。

**成功标准：**
1.  **布局**：修改样式属性（如 `width`）时，仅重新布局受影响的子树，而不是整个文档。
2.  **树结构**：DOM 节点的插入/移除在局部更新渲染树，不进行全量重建。
3.  **绘制**：激进地剔除（不绘制）屏幕外的元素。
4.  **光栅化**：仅对脏区域进行光栅化（部分已实现）。

---

## 2. 现状分析
-   **局部脏矩形**：支持。系统追踪脏区域，并期望使用 `clipRect` 进行增量绘制。
-   **视口剔除**：部分实现（最近添加了 `quickReject`），但需要全面覆盖。
-   **增量布局**：**已禁用/有缺陷**。代码注释明确指出“增量布局有 Bug，目前每帧强制全量布局”。
-   **增量树更新**：**未实现**。`WindowDOMObserver` 在任何节点添加/移除时都会调用 `InvalidateRenderTree()`，强制重建整棵树。

---

## 3. 实现步骤

### 阶段 1：修复并启用增量布局
**目标**：当单个元素改变时，停止重新计算整个布局树。

1.  **优化脏标记传播**：
    -   确保 `RenderObject::MarkNeedsLayout()` 正确标记对象，并潜在地标记其父链。
    -   验证 `MarkNeedsLayout` 设置的标志能被 `LayoutDirtySubtree` 遍历到。

2.  **实现 `RenderTreeUpdater::UpdateStyle`**：
    -   确保样式变化（如 `color`, `font-size`）传播到 `RenderObject::SetComputedStyle`。
    -   区分“仅绘制”变化（如 color）和“布局”变化（如 width, font-size）。

3.  **修复 `Window::Render` 逻辑**：
    -   移除“强制全量布局”的回退代码块。
    -   启用 `LayoutDirtySubtree` 路径。
    -   **关键**：确保 `LayoutDirtySubtree` 针对特定节点正确调用底层布局引擎（Taffy 或 Native）。
        -   *注意*：由于 MBink 使用 Taffy 进行 Flexbox 布局，必须确保 Taffy 的缓存不会被不必要地清除。

### 阶段 2：渲染树增量更新
**目标**：停止在 DOM 变化时销毁/重建 `RenderObject`。

1.  **重构 `WindowDOMObserver::OnNodeAdded`**：
    -   移除 `InvalidateRenderTree()`。
    -   实现查找父 `RenderObject` 的逻辑。
    -   根据 DOM 节点类型创建正确的 `RenderObject` 子类（Block, Inline, Text, Image）。
    -   将新 `RenderObject` 插入到父节点的子列表中正确的位置（匹配 DOM 顺序）。
    -   在该 *父节点* 上调用 `MarkNeedsLayout()`。

2.  **重构 `WindowDOMObserver::OnNodeRemoved`**：
    -   移除 `InvalidateRenderTree()`。
    -   找到与被移除 DOM 节点关联的 `RenderObject`。
    -   在父 `RenderObject` 上调用 `RemoveChild`。
    -   在该 *父节点* 上调用 `MarkNeedsLayout()`。

3.  **处理特殊情况**：
    -   移动节点（Reparenting）。
    -   `TextNode` 内容更新（已通过属性变更处理，需验证）。

### 阶段 3：全面的视口剔除 (Frustum Culling)
**目标**：在屏幕外元素上消耗的 CPU 时间为零。

1.  **标准化剔除**：
    -   将 `quickReject` 逻辑（上一步已添加）应用到 *所有* 容器类型（`RenderBlock`, `RenderInline`, `RenderTable`, `RenderFlex`, `RenderGrid`）。
    -   确保“安全边距”（如 50px）是一致的，并且足以覆盖阴影/轮廓。

2.  **优化长列表**：
    -   验证隐式列表（如长的 `<ul>` 或 `<div>` 序列）能否从中获益。当父节点绘制子节点时，应在调用子节点 `Paint()` 之前检查剔除矩形。
    -   *改进*：在 `RenderObject::PaintChildren` 循环中添加检查，跳过布局边界不与脏区域/裁剪区域相交的子节点。

---

## 4. 执行计划 (分步进行)

| 步骤 | 任务 | 描述 |
| :--- | :--- | :--- |
| **01** | **准备测试用例** | 创建包含 1000 个项目的“性能测试” HTML，并添加按钮来修改一个项目/添加一个项目。通过日志验证当前的卡顿/重建行为。 |
| **02** | **启用增量布局** | 修改 `Window::Render` 以优先使用增量布局路径。修复 `NativeLayoutEngine` 或 `RenderBlock::Layout` 以尊重 `needs_layout_` 标志。 |
| **03** | **布局调试** | 验证测试用例在更新时显示正确，无布局错乱。 |
| **04** | **实现视口剔除** | 将 `quickReject` 应用到其余所有 RenderObject。 |
| **05** | **实现树更新** | 重写 `WindowDOMObserver` 以直接管理 RenderObjects。 |

---

## 5. 验证
实现完成后，我们将使用测试用例确认：
1.  日志显示“增量渲染 (Incremental Rendering)”模式处于激活状态。
2.  日志显示初始加载后，“渲染树重建 (RenderTree rebuild)”计数为 0。
3.  在 DOM 操作期间，FPS 保持在高位。
