# Implementation Plan

**注意：优先保持与现有代码的兼容性。如果原有设计存在问题且有更优方案，则进行完整重构。**

**重要：确保新旧系统不会混乱，要么完全使用新系统，要么在过渡期间有清晰的边界。**

## Phase 1: 基础设施 - DirtyNodeTracker

- [x] 1. 实现 DirtyNodeTracker 类


  - [x] 1.1 创建 DirtyNodeTracker 头文件和实现文件


    - 创建 `core/dom/dirty_node_tracker.h` 和 `dirty_node_tracker.cpp`
    - 定义 StructuralChangeType 枚举
    - 定义 StructuralChange、StyleChange、TextChange 结构体
    - _Requirements: 1.1, 1.2, 1.3, 1.4, 1.5_

  - [x] 1.2 实现变化记录方法

    - 实现 RecordNodeAdded()
    - 实现 RecordNodeRemoved()
    - 实现 RecordNodeReplaced()
    - 实现 RecordStyleChanged()
    - 实现 RecordTextChanged()
    - _Requirements: 1.1, 1.2, 1.3, 1.4, 1.5_

  - [x] 1.3 实现查询和管理方法

    - 实现 HasPendingChanges()
    - 实现 GetStructuralChanges()、GetStyleChanges()、GetTextChanges()
    - 实现 Clear()
    - _Requirements: 1.6_

  - [x] 1.4 实现 Optimize() 变化合并

    - 合并 add-then-remove 同一节点的操作
    - 合并同一节点的多次样式变化
    - _Requirements: 1.7_

- [x] 2. Checkpoint - 确保编译通过


  - Ensure all code compiles, ask the user if questions arise.

## Phase 2: RenderTreeSynchronizer

- [x] 3. 实现 RenderTreeSynchronizer 类


  - [x] 3.1 创建 RenderTreeSynchronizer 头文件和实现文件


    - 创建 `core/render/render_tree_synchronizer.h` 和 `render_tree_synchronizer.cpp`
    - 定义 Synchronize() 方法签名
    - _Requirements: 2.1, 2.2, 2.3_

  - [x] 3.2 实现 ProcessStructuralChanges()

    - 处理 Added 类型变化
    - 处理 Removed 类型变化
    - 处理 Replaced 类型变化（原子操作）
    - _Requirements: 2.1, 2.5_

  - [x] 3.3 实现 ProcessStyleChanges()

    - 更新 RenderObject 的样式
    - 标记需要重新布局的节点
    - _Requirements: 2.2_

  - [x] 3.4 实现 ProcessTextChanges()

    - 更新 RenderText 的文本内容
    - 标记需要重新布局的节点
    - _Requirements: 2.3_

  - [x] 3.5 实现 NeedsSubtreeRebuild() 策略选择

    - 检查变化数量是否超过 10
    - 检查被替换节点子节点数是否超过 5
    - 检查同一父节点变化数是否超过 3
    - _Requirements: 5.1, 5.2, 5.3_

  - [x] 3.6 实现 RebuildSubtree() 和 UpdateNode()

    - 子树重建逻辑
    - 单节点增量更新逻辑
    - _Requirements: 5.4, 5.5_

- [x] 4. Checkpoint - 确保编译通过

  - Ensure all code compiles, ask the user if questions arise.

## Phase 3: RenderPipeline

- [x] 5. 实现 RenderPipeline 类


  - [x] 5.1 创建 RenderPipeline 头文件和实现文件


    - 创建 `core/render/render_pipeline.h` 和 `render_pipeline.cpp`
    - 定义 RenderLifecycle 枚举
    - _Requirements: 4.1, 4.6_

  - [x] 5.2 实现 ProcessFrame() 方法

    - 实现 RenderTreeSync 阶段
    - 实现 StyleRecalc 阶段
    - 实现 Layout 阶段
    - 实现 Paint 阶段
    - _Requirements: 4.1, 4.2, 4.3, 4.4, 4.5_

  - [x] 5.3 实现辅助方法

    - 实现 GetLifecycle()
    - 实现 SetDirtyTracker()、SetRenderTree()、SetLayoutEngine()
    - 实现 MarkNeedsStyleRecalc()、MarkNeedsLayout()
    - _Requirements: 4.2, 4.3, 4.4_

- [x] 6. Checkpoint - 确保编译通过

  - Ensure all code compiles, ask the user if questions arise.

## Phase 4: DOM 操作集成

- [x] 7. 修改 Node 类的 DOM 操作


  - [x] 7.1 修改 AppendChild() 方法

    - 修改 DOM 树后记录变化到 DirtyNodeTracker
    - 评估是否需要移除旧的立即更新逻辑
    - _Requirements: 3.1_

  - [x] 7.2 修改 RemoveChild() 方法

    - 先记录变化到 DirtyNodeTracker
    - 然后修改 DOM 树
    - _Requirements: 3.2_

  - [x] 7.3 修改 ReplaceChild() 方法

    - 记录为原子替换操作
    - 解决旧的先删后加时序问题
    - _Requirements: 3.3_

  - [x] 7.4 修改 InsertBefore() 方法

    - 记录变化时包含正确的索引
    - _Requirements: 3.4_

  - [x] 7.5 修改 Text 节点的 SetTextContent() 方法

    - 记录文本变化到 DirtyNodeTracker
    - _Requirements: 3.5_

- [x] 8. 修改 Document 类

  - [x] 8.1 添加 DirtyNodeTracker 成员

    - 在 Document 类中持有 DirtyNodeTracker 实例
    - 提供 GetDirtyTracker() 方法
    - _Requirements: 1.1_

- [x] 9. 评估并处理旧的 DOMObserver 机制


  - [x] 9.1 评估 WindowDOMObserver 的必要性


    - 如果旧机制设计合理，保持兼容并集成新系统
    - 如果旧机制设计有问题，进行重构或移除
    - _Requirements: 3.1, 3.2, 3.3_

- [x] 10. Checkpoint - 确保编译通过

  - Ensure all code compiles, ask the user if questions arise.

## Phase 5: Window 集成

- [x] 11. 修改 Window 类集成新系统


  - [x] 11.1 添加 RenderPipeline 成员

    - 在 Window 类中持有 RenderPipeline 实例
    - 初始化时配置 DirtyTracker、RenderTree、LayoutEngine
    - _Requirements: 4.1_

  - [x] 11.2 修改渲染循环

    - 在渲染循环中调用 RenderPipeline::ProcessFrame()
    - 评估是否需要移除旧的立即更新逻辑
    - _Requirements: 4.1, 4.2, 4.3, 4.4, 4.5_

  - [x] 11.3 评估并处理旧的 RenderTreeUpdater

    - 如果旧机制设计合理，保持兼容并集成新系统
    - 如果旧机制设计有问题，进行重构或移除
    - _Requirements: 2.1_

- [x] 12. Checkpoint - 确保编译通过

  - Ensure all code compiles, ask the user if questions arise.

## Phase 6: 窗口实际渲染测试

- [x] 13. 编写窗口实际渲染测试


  - [x] 13.1 基础渲染测试

    - 创建窗口并添加 DOM 元素
    - 验证元素正确渲染到窗口
    - _Requirements: 7.1_

  - [x] 13.2 动态添加元素测试

    - 动态添加 DOM 元素
    - 验证窗口更新显示新元素
    - _Requirements: 7.2_

  - [x] 13.3 动态删除元素测试

    - 动态删除 DOM 元素
    - 验证窗口更新隐藏已删除元素
    - _Requirements: 7.3_

  - [x] 13.4 ReplaceChild 测试

    - 执行 ReplaceChild 操作
    - 验证新元素正确显示在旧元素位置
    - _Requirements: 7.4_

  - [x] 13.5 文本更新测试

    - 修改文本节点内容
    - 验证窗口显示更新后的文本
    - _Requirements: 7.5_

  - [x] 13.6 样式变化测试

    - 修改元素样式（颜色、尺寸等）
    - 验证窗口反映样式变化
    - _Requirements: 7.6_

- [x] 14. 运行 Preact Demo 验证

  - [x] 14.1 运行现有 Preact demo

    - 验证 demo 功能正常
    - 验证页面切换正常
    - _Requirements: 6.3_

  - [x] 14.2 运行时钟 demo

    - 验证时钟更新正常
    - 验证性能无明显下降
    - _Requirements: 6.3_

- [x] 15. Final Checkpoint - 确保所有测试通过


  - Ensure all tests pass, ask the user if questions arise.

