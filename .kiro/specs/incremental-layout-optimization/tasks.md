# Implementation Plan

## Phase 1: 基础设施

- [ ] 1. 实现 ContentVersionManager
  - [ ] 1.1 创建 ContentVersionManager 类
    - 创建 `core/layout/content_version.h` 和 `content_version.cpp`
    - 实现单例模式和原子计数器
    - 提供 `GenerateVersion()` 和 `GetCurrentVersion()` 方法
    - _Requirements: 1.1, 1.2, 1.3_

  - [ ] 1.2 编写 ContentVersionManager 属性测试
    - **Property 1: 内容版本号递增一致性**
    - **Validates: Requirements 1.1, 1.2, 1.3**

- [ ] 2. 增强 Cache 结构
  - [ ] 2.1 修改 CacheEntry 添加 content_version 字段
    - 修改 `core/layout/types/cache.h`
    - 在 `CacheEntry` 模板中添加 `uint64_t content_version`
    - _Requirements: 1.4_

  - [ ] 2.2 修改 Cache::Get() 方法添加版本检查
    - 添加 `content_version` 参数
    - 在匹配逻辑中检查版本号是否一致
    - 版本不匹配时返回 `std::nullopt`
    - _Requirements: 1.4, 1.5_

  - [ ] 2.3 修改 Cache::Store() 方法保存版本号
    - 添加 `content_version` 参数
    - 在存储时保存当前版本号
    - _Requirements: 1.4_

  - [ ] 2.4 编写 Cache 版本检查属性测试
    - **Property 2: 缓存版本匹配**
    - **Validates: Requirements 1.4, 1.5**

- [ ] 3. 增强 LayoutNode 结构
  - [ ] 3.1 添加 content_version 字段到 LayoutNode
    - 修改 `core/layout/native_layout_engine.h` 中的 `LayoutNode` 结构
    - 添加 `uint64_t content_version = 0`
    - _Requirements: 1.1_

  - [ ] 3.2 添加 LayoutScope 枚举和字段
    - 定义 `LayoutScope` 枚举（SELF_ONLY, SUBTREE, SIBLINGS, ANCESTORS）
    - 添加 `LayoutScope layout_scope` 字段
    - 添加 `last_measured_width` 和 `last_measured_height` 字段
    - _Requirements: 3.1.1, 3.1.2, 3.1.3, 3.1.4_

- [ ] 4. Checkpoint - 确保所有测试通过
  - Ensure all tests pass, ask the user if questions arise.

## Phase 2: 版本号更新机制

- [ ] 5. 实现版本号更新触发点
  - [ ] 5.1 在 RenderText::SetText() 中触发版本更新
    - 修改 `core/render/render_object.cpp`
    - 当文本内容变化时，调用 `UpdateContentVersion()`
    - _Requirements: 1.1_

  - [ ] 5.2 在 RenderObject 子节点操作中触发版本更新
    - 修改 `AddChild()` 和 `RemoveChild()` 方法
    - 子节点变化时更新父节点版本号
    - _Requirements: 1.2_

  - [ ] 5.3 在 NativeLayoutEngine::UpdateStyle() 中判断版本更新
    - 只有布局相关样式变化时才更新版本号
    - 纯绘制样式（color, background-color 等）不更新版本
    - _Requirements: 1.3_

  - [ ] 5.4 编写版本号更新属性测试
    - **Property 1: 内容版本号递增一致性**
    - **Validates: Requirements 1.1, 1.2, 1.3**

- [ ] 6. 实现 LayoutScope 判断逻辑
  - [ ] 6.1 实现 DetermineLayoutScope() 方法
    - 在 `NativeLayoutEngine` 中添加方法
    - 根据节点样式判断布局影响范围
    - _Requirements: 3.1.3, 3.1.4, 3.1.5_

  - [ ] 6.2 编写 LayoutScope 判断属性测试
    - **Property 5: 固定尺寸容器隔离**
    - **Property 6: Auto 尺寸容器传播**
    - **Validates: Requirements 3.1.3, 3.1.4**

- [ ] 7. Checkpoint - 确保所有测试通过
  - Ensure all tests pass, ask the user if questions arise.

## Phase 3: 增量布局优化

- [ ] 8. 重构 ComputeLayout 方法
  - [ ] 8.1 提取 ComputeLayoutInternal() 内部方法
    - 将 `ComputeLayout()` 的核心逻辑提取到 `ComputeLayoutInternal()`
    - `ComputeLayoutInternal()` 不清除缓存
    - _Requirements: 2.2_

  - [ ] 8.2 修改 ComputeLayout() 调用内部方法
    - `ComputeLayout()` 先清除所有缓存，再调用 `ComputeLayoutInternal()`
    - 保持现有行为不变
    - _Requirements: 6.1_

- [ ] 9. 优化 ComputeIncrementalLayout 方法
  - [ ] 9.1 修改 ComputeIncrementalLayout() 不调用 ComputeLayout()
    - 只清除脏节点的缓存
    - 直接调用 `ComputeLayoutInternal()`
    - _Requirements: 2.1, 2.2_

  - [ ] 9.2 实现智能脏标记传播 PropagateLayoutDirty()
    - 根据 `LayoutScope` 决定传播范围
    - 固定尺寸容器不向上传播
    - Auto 尺寸容器向上传播
    - _Requirements: 4.1, 4.2, 4.3, 4.4_

  - [ ] 9.3 编写增量布局属性测试
    - **Property 3: 增量布局缓存保留**
    - **Property 4: 脏标记向上传播**
    - **Validates: Requirements 2.1, 2.4, 4.1, 4.2**

- [ ] 10. 修改 ComputeNodeLayout 传递版本号
  - [ ] 10.1 更新 ComputeNodeLayout() 方法签名
    - 在缓存 Get/Store 调用中传递 `content_version`
    - _Requirements: 1.4, 1.5_

  - [ ] 10.2 编写布局结果等价性属性测试
    - **Property 7: 布局结果等价性**
    - **Validates: Requirements 6.1**

- [ ] 11. Checkpoint - 确保所有测试通过
  - Ensure all tests pass, ask the user if questions arise.

## Phase 4: IFC 集成

- [ ] 12. 优化 IFCLayout 缓存机制
  - [ ] 12.1 修改 IFCLayout::LayoutCache 使用版本号
    - 将 `content_version` 字段改为从外部传入
    - 移除 `GetContentVersion()` 中的哈希计算
    - _Requirements: 3.1_

  - [ ] 12.2 修改 IFCLayout::IsCacheValid() 使用版本号
    - 添加 `content_version` 参数
    - 直接比较版本号而非计算哈希
    - _Requirements: 3.1, 3.3_

  - [ ] 12.3 修改 IFCLayout::Layout() 传递版本号
    - 从 `LayoutNode` 获取版本号并传递给缓存检查
    - _Requirements: 3.2, 3.4_

  - [ ] 12.4 编写 IFC 缓存属性测试
    - **Property 8: IFC 缓存独立性**
    - **Validates: Requirements 3.4, 3.5**

- [ ] 13. 处理滚动条检测场景
  - [ ] 13.1 优化滚动条检测不清除所有缓存
    - 在 `ComputeLayout()` 中，滚动条检测只清除受影响节点的缓存
    - 或者使用两遍布局时保留第一遍的缓存结果
    - _Requirements: 2.5_

- [ ] 14. Checkpoint - 确保所有测试通过
  - Ensure all tests pass, ask the user if questions arise.

## Phase 5: 测试和验证

- [ ] 15. 编写集成测试
  - [ ] 15.1 时钟更新场景测试
    - 验证只有时钟文本节点被重新布局
    - 验证其他节点使用缓存
    - _Requirements: 5.1, 5.2_

  - [ ] 15.2 列表项添加场景测试
    - 验证新项触发布局
    - 验证现有项使用缓存
    - _Requirements: 2.4_

  - [ ] 15.3 窗口 resize 场景测试
    - 验证全量布局正确执行
    - 验证缓存被正确清除
    - _Requirements: 6.3_

- [ ] 16. 编写性能测试
  - [ ] 16.1 增量布局性能基准测试
    - **Property 9: 增量布局性能**
    - 100 节点树，单节点更新
    - 目标：增量布局时间 < 全量布局时间的 10%
    - **Validates: Requirements 5.1, 5.2, 5.4**

- [ ] 17. 运行现有测试验证兼容性
  - [ ] 17.1 运行所有布局相关单元测试
    - 确保现有测试全部通过
    - _Requirements: 6.4_

  - [ ] 17.2 运行渲染集成测试
    - 确保渲染结果正确
    - _Requirements: 6.1_

- [ ] 18. Final Checkpoint - 确保所有测试通过
  - Ensure all tests pass, ask the user if questions arise.
