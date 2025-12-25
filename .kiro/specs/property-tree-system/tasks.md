# 实现计划

## 阶段 1: 属性树基础设施

- [x] 1. 属性树节点基类
  - [x] 1.1 创建 property_tree_node.h 模板基类
    - 实现节点 ID、父子关系、脏标记、版本号
    - 实现 GetPathToRoot、FindCommonAncestor 方法
    - _Requirements: 5.4_
  - [ ]* 1.2 编写属性测试：公共祖先正确性
    - **Property 11: 公共祖先正确性**
    - **Validates: Requirements 5.4**

- [x] 2. 变换树节点
  - [x] 2.1 创建 transform_tree_node.h/cpp
    - 实现 SkM44 矩阵存储、变换原点、扁平化标志
    - 实现 GetAccumulatedTransform、GetTransformTo 方法
    - 实现滚动变换支持
    - _Requirements: 1.1, 1.2, 1.4_
  - [ ]* 2.2 编写属性测试：变换累积一致性
    - **Property 1: 变换累积一致性**
    - **Validates: Requirements 1.2**
  - [ ]* 2.3 编写单元测试：3D 变换和透视
    - _Requirements: 1.5_

- [x] 3. 裁剪树节点
  - [x] 3.1 创建 clip_tree_node.h/cpp
    - 实现裁剪矩形、圆角、clip-path 存储
    - 实现 GetClipRectInSpace、GetAccumulatedClipRect 方法
    - _Requirements: 2.1, 2.2, 2.3_
  - [ ]* 3.2 编写属性测试：裁剪区域单调性
    - **Property 4: 裁剪区域单调性**
    - **Validates: Requirements 2.3**

- [x] 4. 效果树节点
  - [x] 4.1 创建 effect_tree_node.h/cpp
    - 实现 opacity、filter、blend-mode、mask 存储
    - 实现 RequiresIsolation、GetAccumulatedOpacity 方法
    - _Requirements: 3.1, 3.4, 3.5_
  - [ ]* 4.2 编写属性测试：效果隔离正确性
    - **Property 7: 效果隔离正确性**
    - **Validates: Requirements 3.4, 3.5**

- [x] 5. 滚动树节点
  - [x] 5.1 创建 scroll_tree_node.h/cpp
    - 实现容器尺寸、内容尺寸、滚动偏移存储
    - 实现 GetMaxScrollOffset、GetScrollDirection 方法
    - _Requirements: 4.1, 4.3, 4.4_
  - [ ]* 5.2 编写属性测试：滚动范围正确性
    - **Property 8: 滚动范围正确性**
    - **Validates: Requirements 4.4**

- [x] 6. 属性树状态
  - [x] 6.1 创建 property_tree_state.h/cpp
    - 实现四棵树节点引用
    - 实现 CanMergeWith、ComputeDifference 方法
    - _Requirements: 5.1, 5.2, 5.3_
  - [ ]* 6.2 编写属性测试：属性树状态合并正确性
    - **Property 10: 属性树状态合并正确性**
    - **Validates: Requirements 5.2**

- [x] 7. 属性树集合
  - [x] 7.1 创建 property_trees.h/cpp
    - 实现 PropertyTree 模板类（节点管理）
    - 实现 TransformTree、ClipTree、EffectTree、ScrollTree
    - 实现 PropertyTrees 集合类
    - _Requirements: 5.1_

- [x] 8. Checkpoint - 确保所有测试通过
  - 确保所有测试通过，如有问题请询问用户

## 阶段 2: 属性树构建

- [x] 9. 属性树构建器
  - [x] 9.1 创建 property_tree_builder.h/cpp
    - 实现 Build 方法（完整构建）
    - 实现 Update 方法（增量更新）
    - 实现各类节点创建判断逻辑
    - _Requirements: 2.5, 4.3_
  - [ ]* 9.2 编写单元测试：构建器基本功能
    - 测试从 RenderObject 树构建属性树
    - _Requirements: 2.5, 4.3_

- [x] 10. RenderObject 扩展
  - [x] 10.1 扩展 RenderObject 类
    - 添加 PropertyTreeState 成员
    - 添加 NeedsTransformNode/ClipNode/EffectNode/ScrollNode 方法
    - 添加 CanDirectlyUpdateTransform/Opacity 方法
    - _Requirements: 1.3, 3.2, 7.1, 7.2_

- [x] 11. Checkpoint - 确保所有测试通过
  - 确保所有测试通过，如有问题请询问用户

## 阶段 3: 几何映射

- [x] 12. 几何映射器
  - [x] 12.1 创建 geometry_mapper.h/cpp
    - 实现 MapPoint、MapRect、MapVisualRect 方法
    - 实现 GetTransformMatrix、GetClipRect 方法
    - 实现变换和裁剪缓存
    - _Requirements: 6.1, 6.2, 6.3, 6.4_
  - [ ]* 12.2 编写属性测试：几何映射往返一致性
    - **Property 12: 几何映射往返一致性**
    - **Validates: Requirements 6.1, 6.2**
  - [ ]* 12.3 编写属性测试：可见区域包含性
    - **Property 13: 可见区域包含性**
    - **Validates: Requirements 6.3**
  - [ ]* 12.4 编写属性测试：缓存一致性
    - **Property 14: 缓存一致性**
    - **Validates: Requirements 6.5**

- [x] 13. Checkpoint - 确保所有测试通过
  - 确保所有测试通过，如有问题请询问用户

## 阶段 4: 绘制系统

- [x] 14. 显示项
  - [x] 14.1 创建 display_item.h/cpp
    - 实现 DisplayItemType 枚举
    - 实现 DisplayItem 类（类型、客户端、Picture、边界）
    - 实现 Id 结构用于缓存匹配
    - _Requirements: 9.1_

- [x] 15. 绘制块
  - [x] 15.1 创建 paint_chunk.h/cpp
    - 实现 PaintChunk 类（属性树状态、绘制指令范围、边界）
    - 实现 CanMergeWith、MergeWith 方法
    - 实现光栅化失效区域管理
    - _Requirements: 5.1, 5.2, 9.1, 9.2_
  - [ ]* 15.2 编写单元测试：绘制块合并
    - _Requirements: 5.2_

- [x] 16. 绘制产物
  - [x] 16.1 创建 paint_artifact.h/cpp
    - 实现 PaintArtifact 类（DisplayItems + PaintChunks）
    - 实现 StartNewChunk、FinishCurrentChunk 方法
    - 实现 ComputeChanges 方法（与上一帧比较）
    - _Requirements: 9.1, 9.2, 9.3_
  - [ ]* 16.2 编写属性测试：出现/消失失效正确性
    - **Property 22: 出现/消失失效正确性**
    - **Validates: Requirements 9.2**
  - [ ]* 16.3 编写属性测试：移动失效正确性
    - **Property 23: 移动失效正确性**
    - **Validates: Requirements 9.3**

- [x] 17. 合成原因
  - [x] 17.1 创建 compositing_reasons.h
    - 实现 CompositingReasons 位标志枚举
    - 实现位运算支持函数
    - 实现 GetCompositingReasonDescriptions 函数
    - _Requirements: 7.1, 7.2_

- [x] 18. Checkpoint - 确保所有测试通过
  - 确保所有测试通过，如有问题请询问用户

## 阶段 5: 层化系统

- [x] 19. 待定层
  - [x] 19.1 创建 pending_layer.h/cpp
    - 实现 PendingLayer 类（绘制块集合、属性树状态、边界）
    - 实现 CompositingType 枚举
    - 实现 CanMergeWith、MergeWith 方法
    - _Requirements: 7.3, 7.4_

- [x] 20. 层化器
  - [x] 20.1 创建 layerizer.h/cpp
    - 实现 Layerizer 类
    - 实现 Layerize 方法（将绘制块分配到层）
    - 实现 DetermineCompositingType、GetCompositingReasons 方法
    - 实现重叠检测和层合并逻辑
    - _Requirements: 7.1, 7.2, 7.3, 7.4, 7.5_
  - [ ]* 20.2 编写属性测试：will-change 层提升
    - **Property 15: will-change 层提升**
    - **Validates: Requirements 7.1**
  - [ ]* 20.3 编写属性测试：动画层提升
    - **Property 16: 动画层提升**
    - **Validates: Requirements 7.2**
  - [ ]* 20.4 编写属性测试：层化幂等性
    - **Property 17: 层化幂等性**
    - **Validates: Requirements 7.3**
  - [ ]* 20.5 编写属性测试：重叠检测正确性
    - **Property 18: 重叠检测正确性**
    - **Validates: Requirements 7.4**

- [x] 21. 光栅化失效器
  - [x] 21.1 创建 raster_invalidator.h/cpp
    - 实现 RasterInvalidator 类
    - 实现 ComputeInvalidation、ComputeChunkInvalidation 方法
    - 实现出现/消失/移动/内容变化的失效处理
    - _Requirements: 9.1, 9.2, 9.3, 9.4, 9.5_
  - [ ]* 21.2 编写属性测试：失效区域最小性
    - **Property 21: 失效区域最小性**
    - **Validates: Requirements 9.1**
  - [ ]* 21.3 编写属性测试：裁剪失效区域正确性
    - **Property 5: 裁剪失效区域正确性**
    - **Validates: Requirements 2.4, 9.4**
  - [ ]* 21.4 编写属性测试：非光栅化属性变化不失效
    - **Property 24: 非光栅化属性变化不失效**
    - **Validates: Requirements 9.5**

- [x] 22. Checkpoint - 确保所有测试通过
  - 确保所有测试通过，如有问题请询问用户

## 阶段 6: 合成器集成

- [x] 23. 合成层重构
  - [x] 23.1 重构 compositor_layer.h/cpp
    - 添加 PropertyTreeState 成员
    - 添加脏区域和纹理脏区域管理
    - 添加绘制块引用
    - 实现增量光栅化支持
    - _Requirements: 8.1, 8.2, 8.3, 9.1_

- [x] 24. 绘制产物合成器
  - [x] 24.1 创建 paint_artifact_compositor.h/cpp
    - 实现 PaintArtifactCompositor 类
    - 实现 Update 方法（完整更新）
    - 实现 TryFastPathUpdate 方法（快速路径）
    - 实现 DirectlyUpdateTransform/Opacity/ScrollOffset 方法
    - _Requirements: 8.1, 8.2, 8.3, 8.4, 8.5_
  - [ ]* 24.2 编写属性测试：变换直接更新不触发光栅化
    - **Property 2: 变换直接更新不触发光栅化**
    - **Validates: Requirements 1.3, 8.1**
  - [ ]* 24.3 编写属性测试：Opacity 直接更新不触发光栅化
    - **Property 6: Opacity 直接更新不触发光栅化**
    - **Validates: Requirements 3.2, 8.2**
  - [ ]* 24.4 编写属性测试：滚动直接更新不触发光栅化
    - **Property 9: 滚动直接更新不触发光栅化**
    - **Validates: Requirements 4.5, 8.3, 10.3**
  - [ ]* 24.5 编写属性测试：直接更新后只触发合成
    - **Property 19: 直接更新后只触发合成**
    - **Validates: Requirements 8.4**
  - [ ]* 24.6 编写属性测试：层结构变化触发完整更新
    - **Property 20: 层结构变化触发完整更新**
    - **Validates: Requirements 8.5**

- [x] 25. 光栅化和合成
  - [x] 25.1 实现 RasterizeDirtyLayers 方法
    - 只光栅化有脏区域的层
    - 支持增量光栅化
    - _Requirements: 9.1, 10.1_
  - [x] 25.2 实现 CompositeToCanvas/CompositeToGPU 方法
    - 使用属性树状态进行合成
    - 支持 CPU 和 GPU 路径
    - _Requirements: 10.1, 10.2_
  - [x] 25.3 实现 UploadDirtyTextures 方法
    - 只上传脏区域到 GPU
    - _Requirements: 10.1, 10.4_

- [x] 26. Checkpoint - 确保所有测试通过
  - 确保所有测试通过，如有问题请询问用户

## 阶段 7: 系统集成

- [x] 27. WindowCompositorAdapter 集成
  - [x] 27.1 扩展 WindowCompositorAdapter
    - 添加 PropertyTrees、PropertyTreeBuilder、PaintArtifactCompositor 成员
    - 添加 SetUsePropertyTreeSystem 开关
    - 实现新旧渲染路径切换
    - _Requirements: 10.1, 10.2_
  - [ ]* 27.2 编写集成测试：渲染路径切换
    - 测试新旧路径切换正确性
    - _Requirements: 10.1_

- [x] 28. RenderPipelineV2 集成
  - [x] 28.1 修改 RenderPipelineV2
    - 集成属性树构建
    - 集成绘制产物生成
    - 集成层化和合成
    - _Requirements: 10.1, 10.2, 10.3_

- [x] 29. 动画系统集成



  - [x] 29.1 修改 AnimationApplicator

    - 检测可直接更新的动画
    - 调用 DirectlyUpdateTransform/Opacity 方法
    - _Requirements: 8.1, 8.2, 10.1_
  - [ ]* 29.2 编写属性测试：滚动偏移正确应用
    - **Property 3: 滚动偏移正确应用**
    - **Validates: Requirements 1.4, 4.2**



- [x] 30. 滚动系统集成

  - [x] 30.1 修改滚动处理

    - 集成滚动树
    - 调用 DirectlyUpdateScrollOffset 方法
    - _Requirements: 4.2, 4.5, 8.3, 10.3_


- [x] 31. Checkpoint - 确保所有测试通过

  - 确保所有测试通过，如有问题请询问用户

## 阶段 8: 性能优化和验证

- [x] 32. 性能测试
  - [x] 32.1 创建性能基准测试
    - 测试 transform 动画 GPU 占用
    - 测试 opacity 动画 GPU 占用
    - 测试滚动性能
    - _Requirements: 10.1, 10.2, 10.3, 10.4, 10.5_
  - [ ]* 32.2 编写性能回归测试
    - 确保 GPU 占用低于 10%
    - 确保静态页面 CPU/GPU 接近 0%
    - _Requirements: 10.1, 10.2_

- [x] 33. 视觉正确性验证
  - [x] 33.1 创建视觉对比测试
    - 与旧渲染路径对比
    - 截图对比测试
    - _Requirements: 10.1_

- [x] 34. 错误处理完善
  - [x] 34.1 实现错误处理
    - 创建 property_tree_error.h
    - 实现各类错误的处理策略
    - 实现 GPU 上下文丢失回退
    - _Requirements: 10.1_



- [x] 35. Final Checkpoint - 确保所有测试通过


  - 确保所有测试通过，如有问题请询问用户
