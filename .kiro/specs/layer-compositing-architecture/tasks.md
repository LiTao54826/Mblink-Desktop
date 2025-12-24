# Implementation Plan

## 阶段 1: 核心基础设施

- [x] 1. 创建 CompositorLayer 类



  - [x] 1.1 实现基本层结构（边界、变换、透明度）

    - 创建 `core/compositor/compositor_layer.h` 和 `.cpp`
    - 实现 id、bounds、transform matrix、opacity 属性
    - 实现父子关系管理
    - _Requirements: 2.1, 2.2, 2.3_
  - [x] 1.2 实现 CPU 位图管理


    - 添加 SkBitmap 成员和延迟初始化
    - 实现 GetCanvas() 用于光栅化
    - 处理边界变化时的位图调整
    - _Requirements: 1.2_
  - [x] 1.3 实现脏区域跟踪


    - 添加脏区域列表
    - 实现 MarkDirty()、ClearDirtyRegions()、HasDirtyRegions()
    - 实现脏区域合并算法
    - _Requirements: 3.1, 3.4_
  - [x] 1.4 编写脏区域合并的属性测试


    - **Property 3: Dirty region marking and merging**
    - **Validates: Requirements 3.1, 3.4**
  - [x] 1.5 实现滚动偏移管理


    - 添加 scroll_offset_ 成员
    - 实现 GetScrollOffset()、SetScrollOffset()
    - _Requirements: 6.1_

- [x] 2. 创建 GPU 纹理管理



  - [x] 2.1 实现纹理分配和释放

    - 添加 GLuint texture_id_ 成员
    - 实现 CreateTexture()、DestroyTexture()
    - 处理纹理调整大小
    - _Requirements: 4.2, 4.3, 4.4_
  - [x] 2.2 实现部分纹理上传


    - 单独跟踪纹理脏区域
    - 使用 glTexSubImage2D 实现 UploadDirtyRegions()
    - _Requirements: 4.1_

  - [x] 2.3 编写纹理管理的单元测试

    - 测试纹理创建、调整大小、销毁
    - 测试部分上传正确性
    - _Requirements: 4.1, 4.2, 4.3, 4.4_

- [x] 3. 检查点 - 确保所有测试通过



  - Ensure all tests pass, ask the user if questions arise.

## 阶段 2: 层树构建

- [x] 4. 创建 LayerTreeBuilder 类



  - [x] 4.1 实现层提升逻辑

    - 创建 `core/compositor/layer_tree_builder.h` 和 `.cpp`
    - 实现 ShouldPromote() 检查 will-change、position、动画
    - 定义 PromotionReason 枚举
    - _Requirements: 2.1, 2.2, 2.3, 2.4_
  - [x] 4.2 编写层提升的属性测试


    - **Property 2: Layer promotion based on CSS properties**
    - **Validates: Requirements 2.1, 2.2, 2.3**
  - [x] 4.3 实现 Build() 从渲染树构建层树

    - 递归遍历渲染树
    - 为提升的元素创建层
    - 建立父子关系
    - _Requirements: 2.1, 2.2, 2.3, 2.4_
  - [x] 4.4 实现 Update() 用于增量层树更新

    - 处理元素添加/删除
    - 处理提升原因变化
    - _Requirements: 2.5_
  - [x] 4.5 编写层清理的属性测试


    - **Property 10: Layer cleanup on condition change**
    - **Validates: Requirements 2.5, 7.4**

- [x] 5. 将 LayerInfo 集成到 RenderObject



  - [x] 5.1 向 RenderObject 添加 LayerInfo 结构

    - 添加 compositor_layer weak_ptr
    - 添加 promotion_reason 字段
    - _Requirements: 2.1_

  - [x] 5.2 更新 RenderObject 以跟踪层关联
    - 通过 LayerTreeBuilder 管理 RenderObject 到 CompositorLayer 的映射
    - 使用 render_object_to_layer_ 映射表实现关联
    - _Requirements: 2.1_

- [x] 6. 检查点 - 确保所有测试通过



  - Ensure all tests pass, ask the user if questions arise.

## 阶段 3: 光栅化

- [x] 7. 创建 Rasterizer 类
  - [x] 7.1 实现完整层光栅化
    - 创建 `core/compositor/rasterizer.h` 和 `.cpp`
    - 使用现有 Paint() 逻辑实现 RasterizeLayer()
    - 绘制前清除位图
    - _Requirements: 1.2_
  - [x] 7.2 编写光栅化正确性的属性测试
    - **Property 1: Layer content is rasterized to CPU bitmap**
    - **Validates: Requirements 1.2, 3.2**
  - [x] 7.3 实现增量光栅化
    - 实现 RasterizeDirtyRegions()
    - 使用 canvas clip 限制绘制到脏区域
    - 绘制前只清除脏区域
    - _Requirements: 3.2_
  - [x] 7.4 编写增量光栅化的属性测试
    - **Property 4: Incremental rasterization preserves unchanged pixels**
    - **Validates: Requirements 3.2, 3.3**
  - [x] 7.5 实现滚动感知光栅化
    - 跟踪之前光栅化的内容
    - 滚动时只光栅化新可见区域
    - _Requirements: 3.5, 6.2_
  - [x] 7.6 编写滚动优化的属性测试
    - **Property 5: Scroll optimization - no re-rasterization**
    - **Validates: Requirements 3.5, 6.1, 6.2, 6.3**

- [x] 8. 检查点 - 确保所有测试通过
  - 54 个 compositor 相关测试全部通过 ✅
  - Ensure all tests pass, ask the user if questions arise.

## 阶段 4: GPU 合成

- [x] 9. 创建 Compositor 类
  - [x] 9.1 实现 GPU 初始化
    - 创建 `core/compositor/compositor.h` 和 `.cpp`
    - 创建纹理四边形渲染的着色器程序
    - 设置 VAO/VBO 用于四边形几何
    - _Requirements: 1.4_
  - [x] 9.2 实现层合成
    - 实现 Composite() 遍历层树
    - 使用变换和透明度渲染每层的纹理
    - 正确处理 z-order
    - _Requirements: 5.1, 5.4_
  - [x] 9.3 编写 z-order 合成的属性测试
    - **Property 6: Z-order compositing correctness**
    - **Validates: Requirements 5.1, 5.4**
  - [x] 9.4 实现 CPU 回退合成
    - GPU 不可用时实现软件混合
    - 使用 SkCanvas 进行 CPU 合成
    - _Requirements: 1.5, 8.2_
  - [x] 9.5 实现帧跳过优化
    - 跟踪自上一帧以来是否有层变化
    - 如果没有变化则跳过合成
    - _Requirements: 5.5_
  - [x] 9.6 编写帧跳过的属性测试
    - **Property 8: Frame skip when nothing changed**
    - **Validates: Requirements 3.3, 5.5**

- [x] 10. 检查点 - 确保所有测试通过
  - 48 个 compositor 相关测试全部通过 ✅
  - Ensure all tests pass, ask the user if questions arise.

## 阶段 5: 动画集成

- [x] 11. 将动画与层系统集成
  - [x] 11.1 创建 AnimationLayerBridge 类
    - 连接动画系统和合成层系统
    - 检测动画开始/结束，触发层提升/降级
    - _Requirements: 5.2, 5.3, 7.1, 7.2_
  - [x] 11.2 编写动画优化的属性测试
    - **Property 7: Transform/opacity animation without re-rasterization**
    - **Validates: Requirements 5.2, 5.3, 7.1, 7.2**
    - 10 个动画层属性测试全部通过 ✅
  - [x] 11.3 实现动画触发的层提升
    - 动画开始时将元素提升到自己的层
    - 动画结束时降级（如果没有其他原因）
    - _Requirements: 2.3, 7.4_
  - [x] 11.4 实现动画批处理
    - BeginAnimationUpdates/EndAnimationUpdates 批量处理
    - 所有动画处理后单次合成器更新
    - _Requirements: 7.3_

- [x] 12. 检查点 - 确保所有测试通过
  - 10 个动画层属性测试全部通过 ✅

## 阶段 6: 滚动集成

- [x] 13. 将滚动与层系统集成
  - [x] 13.1 创建滚动内容层
    - 创建 `core/compositor/scroll_layer_manager.h` 和 `.cpp`
    - 检测可滚动容器
    - 为滚动内容创建单独的层
    - _Requirements: 2.4_
  - [x] 13.2 实现无需重新光栅化的滚动
    - 滚动事件时更新层滚动偏移
    - 合成器在合成时应用滚动偏移
    - _Requirements: 6.1_
  - [x] 13.3 实现固定元素处理
    - 将固定元素保持在单独的层
    - 固定层在滚动时不移动
    - _Requirements: 6.5_
  - [x] 13.4 编写滚动 + 固定元素的属性测试
    - 20 个滚动层属性测试全部通过 ✅
    - 测试固定元素在滚动时保持静止
    - _Requirements: 6.5_

- [x] 14. 检查点 - 确保所有测试通过
  - 76 个 compositor 相关测试全部通过 ✅

## 阶段 7: 管线集成

- [x] 15. 创建 RenderPipelineV2
  - [x] 15.1 实现管线初始化
    - 创建 `core/compositor/render_pipeline_v2.h` 和 `.cpp`
    - 初始化 LayerTreeBuilder、Rasterizer、Compositor
    - _Requirements: 1.1_
  - [x] 15.2 实现 Render() 方法
    - 构建/更新层树
    - 光栅化脏层
    - 合成到屏幕
    - _Requirements: 1.1, 1.2, 1.3, 1.4_
  - [x] 15.3 实现 HandleScroll() 方法
    - 更新滚动偏移而不重新光栅化
    - 触发合成器更新
    - _Requirements: 6.1, 6.4_
  - [x] 15.4 实现 UpdateAnimations() 方法
    - 更新层变换/透明度
    - 触发合成器更新
    - _Requirements: 7.1, 7.2_
  - [x] 15.5 编写渲染管线属性测试
    - 24 个渲染管线属性测试全部通过 ✅

- [x] 16. 与 Window 类集成
  - [x] 16.1 向 Window 添加 RenderPipelineV2
    - 创建 `WindowCompositorAdapter` 适配器类
    - 在窗口初始化时创建适配器
    - 通过 `SetUseLayerCompositing(true)` 启用分层合成
    - _Requirements: 8.1_
  - [x] 16.2 更新滚动事件处理
    - 适配器提供 `HandleScroll()` 方法路由到管线
    - _Requirements: 6.1_
  - [x] 16.3 更新动画处理
    - 适配器提供动画相关方法路由到管线
    - _Requirements: 7.1, 7.2_

- [x] 17. 检查点 - 确保所有测试通过
  - 108 个 compositor 相关测试全部通过 ✅
  - Window 集成编译成功 ✅

## 阶段 8: 视觉一致性验证

- [x] 18. 实现视觉一致性测试
  - [x] 18.1 编写视觉一致性的属性测试
    - **Property 9: Visual output consistency**
    - **Validates: Requirements 8.1, 8.2**
    - 16 个视觉一致性属性测试全部通过 ✅
  - [x] 18.2 创建参考渲染对比工具
    - 用新旧管线渲染相同内容
    - 逐像素对比
    - _Requirements: 8.1_
  - [x] 18.3 修复发现的任何视觉差异
    - 调试并修复渲染差异
    - _Requirements: 8.1_

- [x] 19. 实现调试功能
  - [x] 19.1 添加层边界可视化
    - 在每层周围绘制彩色边框
    - 通过 SetShowLayerBorders() 切换
    - _Requirements: 8.4_
  - [x] 19.2 添加性能分析
    - 跟踪每层光栅化时间
    - 跟踪合成时间
    - 通过 CompositorFrameInfo 报告
    - _Requirements: 8.5, 7.5_

- [x] 20. 最终检查点 - 确保所有测试通过
  - 108 个 compositor 相关测试全部通过 ✅
