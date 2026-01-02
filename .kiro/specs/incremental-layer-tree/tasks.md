# Implementation Plan

## 测试规范说明

本实现计划遵循 `.kiro/steering/build-and-test.md` 规范：
- 使用 JavaScript 测试脚本配合 C++ 日志进行真实 UI 测试
- 测试文件放在 `tests/js/` 目录
- 使用 `esm_loader -q 5` 自动退出，全程无需人工干预
- 通过日志标记 `[TEST_PASS]`/`[TEST_FAIL]` 验证结果

## Phase 1: 引入 LayerTreeManager 基础框架

- [x] 1. 创建 LayerTreeManager 类文件

  - [x] 1.1 创建 layer_tree_manager.h 头文件
    - 定义 LayerUpdateType 枚举
    - 定义 PendingLayerUpdate 结构
    - 定义 ScrollState 结构
    - 定义 CoordinateSpace 枚举
    - 声明 LayerTreeManager 类接口
    - _Requirements: 1.1, 1.6, 2.1_

  - [x] 1.2 创建 layer_tree_manager.cpp 实现文件
    - 实现构造函数和析构函数
    - 实现 Initialize() 方法
    - 实现 SetViewport() 方法
    - _Requirements: 1.1_

  - [x]* 1.3 创建 tests/js/test_incremental_layer_tree.js 测试文件


    - 创建测试辅助函数 (logTest, assertEqual)
    - 实现 LayerTreeManager 初始化测试
    - **Property 4: Scroll Offset Consistency**
    - **Validates: Requirements 2.1, 2.3**
    - 运行命令: `build\bin\Release\esm_loader.exe tests\js\test_incremental_layer_tree.js -q 5`

- [x] 2. 实现增量更新队列

  - [x] 2.1 实现 RequestAddLayer() 方法
    - 创建 PendingLayerUpdate 并添加到队列
    - 记录调试信息
    - _Requirements: 1.1, 1.3, 1.4_

  - [x] 2.2 实现 RequestRemoveLayer() 方法
    - 创建移除操作并添加到队列
    - _Requirements: 1.2, 1.5_

  - [x] 2.3 实现 RequestUpdateBounds() 方法
    - 创建边界更新操作
    - _Requirements: 5.1_


  - [ ]* 2.4 添加增量层添加 JS 测试
    - 测试添加 fixed 元素时层数量变化
    - **Property 1: Incremental Layer Addition Preserves Existing Layers**
    - **Validates: Requirements 1.1, 1.3, 1.4**


  - [ ]* 2.5 添加增量层移除 JS 测试
    - 测试移除 fixed 元素时层数量变化
    - **Property 2: Incremental Layer Removal Preserves Unrelated Layers**
    - **Validates: Requirements 1.2, 1.5**

- [x] 3. Checkpoint - 确保所有测试通过
  - Ensure all tests pass, ask the user if questions arise.

## Phase 2: 滚动状态管理（SSOT）

- [x] 4. 实现滚动状态存储

  - [x] 4.1 实现 RegisterScrollContainer() 方法
    - 创建 ScrollState 并初始化
    - 计算滚动范围
    - _Requirements: 2.4_

  - [x] 4.2 实现 UnregisterScrollContainer() 方法
    - 从 scroll_states_ 中移除
    - _Requirements: 2.4_

  - [x] 4.3 实现 GetScrollState() 方法
    - 返回只读滚动状态
    - _Requirements: 2.3_


  - [ ]* 4.4 添加滚动容器初始化 JS 测试
    - 测试滚动容器注册后的初始滚动偏移
    - **Property 6: Scroll Offset Initialization**
    - **Validates: Requirements 2.4**

- [x] 5. 实现滚动位置更新

  - [x] 5.1 实现 SetScrollPosition() 方法
    - 更新滚动位置
    - Clamp 到有效范围
    - 通知监听器
    - _Requirements: 2.1_

  - [x] 5.2 实现 ScrollBy() 方法
    - 增量滚动
    - 委托给 SetScrollPosition()
    - _Requirements: 2.1_

  - [x] 5.3 实现 UpdateScrollContentSize() 方法
    - 更新内容尺寸
    - 重新计算滚动范围
    - _Requirements: 2.2_


  - [ ]* 5.4 添加滚动偏移一致性 JS 测试
    - 测试 scrollLeft/scrollTop 设置和读取一致性
    - **Property 4: Scroll Offset Consistency**
    - **Validates: Requirements 2.1, 2.3**

- [x] 6. 实现滚动监听器

  - [x] 6.1 实现 AddScrollListener() 方法
    - 注册监听器回调
    - 返回监听器 ID
    - _Requirements: 2.5_

  - [x] 6.2 实现 RemoveScrollListener() 方法
    - 移除监听器
    - _Requirements: 2.5_

  - [x] 6.3 实现 NotifyScrollListeners() 私有方法
    - 通知所有监听器

    - _Requirements: 2.5_

  - [ ]* 6.4 添加滚动事件通知 JS 测试
    - 测试滚动事件触发和监听
    - **Property 7: Scroll Offset Notification**
    - **Validates: Requirements 2.5**

- [x] 7. Checkpoint - 确保所有测试通过
  - Ensure all tests pass, ask the user if questions arise.

## Phase 3: 坐标转换系统

- [x] 8. 实现坐标转换

  - [x] 8.1 实现 ConvertPoint() 方法
    - Document ↔ Viewport 转换
    - Viewport ↔ Layer 转换
    - _Requirements: 3.1, 3.5_

  - [x] 8.2 实现 ConvertRect() 方法
    - 委托给 ConvertPoint()
    - _Requirements: 3.1_

  - [x] 8.3 实现 CalculateLayerBoundsInDocument() 方法
    - 累加祖先位置
    - 处理滚动偏移
    - _Requirements: 5.4_

  - [x] 8.4 实现 CalculateFixedLayerBounds() 方法
    - 使用视口坐标

    - 不累加祖先位置
    - _Requirements: 5.5_

  - [ ]* 8.5 添加坐标转换 JS 测试
    - 测试 getBoundingClientRect 在滚动前后的一致性
    - **Property 8: Coordinate System Round-Trip Consistency**
    - **Validates: Requirements 3.1, 3.2, 3.3, 3.4, 3.5**

- [x] 9. Checkpoint - 确保所有测试通过
  - Ensure all tests pass, ask the user if questions arise.

## Phase 4: LayerTreeBuilder 增量更新扩展

- [x] 10. 扩展 LayerTreeBuilder

  - [x] 10.1 添加 FindParentLayerForObject() 方法
    - Fixed 元素返回根层
    - 其他元素查找最近的有层祖先
    - _Requirements: 4.1, 4.2_

  - [x] 10.2 添加 AddLayerForObject() 方法
    - 查找父层
    - 创建层并附加
    - 延迟计算边界
    - _Requirements: 1.1, 5.1_

  - [x] 10.3 添加 RemoveLayerForObject() 方法
    - 转移子层到父层
    - 移除层
    - 清理映射
    - _Requirements: 1.2_

  - [x] 10.4 添加 UpdateLayerBoundsDeferred() 方法

    - 在父层已知后计算边界
    - 处理 fixed 元素特殊情况
    - _Requirements: 5.1, 5.2, 5.3_


  - [ ]* 10.5 添加 Fixed 层根附加 JS 测试
    - 测试 fixed 元素在嵌套容器中的层结构
    - **Property 9: Fixed Layer Root Attachment**
    - **Validates: Requirements 4.1, 4.2**

  - [ ]* 10.6 添加边界计算 JS 测试
    - 测试层边界在父层附加后的正确性
    - **Property 13: Bounds Calculation After Parent Attachment**
    - **Validates: Requirements 5.1, 5.2, 5.3**

- [x] 11. 实现增量构建

  - [x] 11.1 添加 IncrementalBuild() 方法
    - 遍历待处理更新
    - 调用相应的单层操作
    - 递增版本号
    - _Requirements: 1.6_


  - [x] 11.2 添加 CanIncrementalUpdate() 方法
    - 检查根层是否存在
    - 检查层树是否一致
    - _Requirements: 1.6_

  - [ ]* 11.3 添加批量更新 JS 测试
    - 测试同一帧内多个层操作的原子性
    - **Property 3: Batch Updates Are Atomic**
    - **Validates: Requirements 1.6**

- [x] 12. Checkpoint - 确保所有测试通过
  - Ensure all tests pass, ask the user if questions arise.

## Phase 5: CompositorLayer 扩展

- [x] 13. 扩展 CompositorLayer

  - [x] 13.1 添加 layer_identity_ 成员和访问方法
    - GetLayerIdentity()
    - SetLayerIdentity()
    - _Requirements: 1.1, 1.2_

  - [x] 13.2 添加 GetTreeDepth() 方法
    - 计算层在树中的深度
    - _Requirements: 4.4_

  - [x] 13.3 添加 ReparentTo() 方法
    - 从当前父层移除
    - 添加到新父层
    - _Requirements: 4.5_

  - [x] 13.4 添加 InsertChildByZIndex() 方法
    - 按 z-index 排序插入
    - _Requirements: 4.4_

  - [x] 13.5 添加 GetZIndex() 方法
    - 从关联的 RenderObject 获取 z-index
    - _Requirements: 4.4_

  - [x]* 13.6 添加 z-index 排序 JS 测试

    - 测试多个 fixed 元素的 z-index 排序
    - **Property 11: Fixed Layer Z-Index Ordering**
    - **Validates: Requirements 4.4**

- [x] 14. Checkpoint - 确保所有测试通过
  - Ensure all tests pass, ask the user if questions arise.

## Phase 6: 集成到 RenderPipeline

- [x] 15. 集成 LayerTreeManager 到 RenderPipeline

  - [x] 15.1 在 RenderPipeline 中添加 LayerTreeManager 成员
    - 添加 std::unique_ptr<LayerTreeManager> layer_tree_manager_
    - 在 Initialize() 中创建和初始化
    - _Requirements: 1.1_

  - [x] 15.2 添加特性开关
    - 在 UnifiedPipelineConfig 中添加 enable_incremental_layer_tree
    - 默认为 false（保持向后兼容）
    - _Requirements: 1.1_

  - [x] 15.3 修改 DoLayerTreeBuild() 方法
    - 检查特性开关
    - 如果启用，使用增量更新
    - 否则使用原有的完整重建
    - _Requirements: 1.1, 1.6_


  - [ ]* 15.4 添加层树构建集成 JS 测试
    - 测试增量更新和完整重建的一致性
    - 验证 Toast/Modal 创建不触发完整重建
    - _Requirements: 1.1_

- [x] 16. 迁移滚动处理

  - [x] 16.1 修改 HandleScroll() 方法
    - 委托给 LayerTreeManager::ScrollBy()
    - _Requirements: 2.1_

  - [x] 16.2 修改 ScrollTo() 方法
    - 委托给 LayerTreeManager::SetScrollPosition()
    - _Requirements: 2.1_


  - [ ]* 16.3 添加滚动偏移保持 JS 测试
    - 测试层树重建后滚动偏移保持
    - **Property 5: Scroll Offset Preservation on Rebuild**
    - **Validates: Requirements 2.2**

- [x] 17. Checkpoint - 确保所有测试通过
  - Ensure all tests pass, ask the user if questions arise.

## Phase 7: Fixed 元素优化

- [x] 18. 优化 Fixed 元素处理

  - [x] 18.1 修改 BuildRecursive() 处理 fixed 元素
    - Fixed 元素直接挂在根层下
    - 不参与父层的子层列表
    - _Requirements: 4.1, 4.2_

  - [x] 18.2 修改 CompositeLayerCPU() 处理 fixed 层
    - Fixed 层不受父层滚动影响
    - 使用视口坐标定位

    - _Requirements: 4.3_

  - [ ]* 18.3 添加 Fixed 层视口稳定性 JS 测试
    - 测试滚动时 fixed 元素位置不变

    - **Property 10: Fixed Layer Viewport Stability**
    - **Validates: Requirements 4.3**

  - [ ]* 18.4 添加 Fixed 层移除隔离 JS 测试
    - 测试移除 fixed 元素不影响滚动容器
    - **Property 12: Fixed Layer Removal Isolation**
    - **Validates: Requirements 4.5**

- [x] 19. Checkpoint - 确保所有测试通过
  - Ensure all tests pass, ask the user if questions arise.

## Phase 8: 脏标记和光栅化优化

- [x] 20. 实现脏标记优化

  - [x] 20.1 实现内容变化脏标记
    - 只标记受影响的层
    - _Requirements: 7.1_

  - [x] 20.2 实现 transform/opacity 更新优化
    - 不触发光栅化
    - _Requirements: 7.2, 7.3_


  - [x] 20.3 实现边界变化处理
    - 根据可见性决定是否光栅化
    - _Requirements: 7.4_

  - [ ]* 20.4 添加内容变化脏标记 JS 测试
    - 测试修改元素内容只影响对应层
    - **Property 19: Content Change Dirty Marking**
    - **Validates: Requirements 7.1**


  - [ ]* 20.5 添加 transform/opacity 更新 JS 测试
    - 测试 transform/opacity 变化不触发重绘
    - **Property 20: Transform/Opacity Update Without Rasterization**
    - **Validates: Requirements 7.2, 7.3**

- [x] 21. Checkpoint - 确保所有测试通过
  - Ensure all tests pass, ask the user if questions arise.

## Phase 9: 动画状态保持

- [x] 22. 实现动画状态保持

  - [x] 22.1 在层更新时保持动画状态
    - 不影响无关层的动画
    - _Requirements: 6.1, 6.2_

  - [x] 22.2 实现动画状态转移
    - 层提升时转移动画状态
    - _Requirements: 6.3_

  - [x] 22.3 实现动画清理
    - 动画完成时释放资源

    - _Requirements: 6.4_

  - [ ]* 22.4 添加动画状态保持 JS 测试
    - 测试添加 Toast 时不影响其他元素动画

    - **Property 16: Animation State Preservation**
    - **Validates: Requirements 6.1, 6.2, 6.5**

  - [ ]* 22.5 添加动画状态转移 JS 测试
    - 测试元素层提升时动画继续
    - **Property 17: Animation State Transfer**
    - **Validates: Requirements 6.3**

- [x] 23. Checkpoint - 确保所有测试通过
  - Ensure all tests pass, ask the user if questions arise.

## Phase 10: 调试支持和清理

- [x] 24. 实现调试支持

  - [x] 24.1 实现调试日志
    - 层添加/移除日志
    - 滚动偏移变化日志
    - _Requirements: 8.1, 8.2_

  - [x] 24.2 实现层检查功能

    - 返回层的坐标系、滚动偏移源、父层关系
    - _Requirements: 8.4_

  - [x]* 24.3 添加层检查 JS 测试

    - 测试层信息查询的准确性
    - **Property 23: Layer Inspection Information**
    - **Validates: Requirements 8.4**

- [x] 25. 更新 CMakeLists.txt

  - [x] 25.1 添加新文件到构建系统
    - layer_tree_manager.cpp
    - 测试文件
    - _Requirements: 1.1_

- [x] 26. 更新文档

  - [x] 26.1 更新 core/compositor/README.md
    - 添加 LayerTreeManager 说明
    - 更新架构图
    - _Requirements: 8.1_

- [x] 27. Final Checkpoint - 确保所有测试通过

  - 运行: `build\bin\Release\esm_loader.exe tests\js\test_incremental_layer_tree.js -q 5 >> debuglog.txt`
  - 验证: `findstr "TEST_FAIL" debuglog.txt` 应无输出
  - 验证: `findstr "TEST_PASS" debuglog.txt` 应有多条输出
  - 清理: `del debuglog.txt`
  - Ensure all tests pass, ask the user if questions arise.
