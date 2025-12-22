# 实现计划

- [x] 1. 实现 LayerManager 核心功能


  - [x] 1.1 创建 layer_manager.cpp 实现文件



    - 实现 LayerManager 单例模式
    - 实现 BeginFrame() 清除所有 Layer
    - 实现 GetLayerLevel() 根据 z-index 返回层级
    - 实现 ShouldCollect() 判断是否收集元素
    - 实现 Collect() 分发元素到对应 Layer
    - _Requirements: 1.1, 1.2_
  - [x] 1.2 编写属性测试 - BeginFrame 清除


    - **Property 1: BeginFrame 清除所有 Layer**
    - **Validates: Requirements 1.1**

  - [x] 1.3 编写属性测试 - 元素收集

    - **Property 2: 元素收集到正确的 Layer**
    - **Validates: Requirements 1.2**

- [x] 2. 实现 LayerManager 绘制功能

  - [x] 2.1 实现 PaintLayers() 方法


    - 按 Base -> Overlay -> Modal 顺序绘制
    - 设置 painting_layers_ 标志防止递归
    - _Requirements: 1.3_
  - [x] 2.2 编写属性测试 - 绘制顺序


    - **Property 3: Layer 绘制顺序**
    - **Validates: Requirements 1.3**
  - [x] 2.3 编写属性测试 - Layer 内排序


    - **Property 4: Layer 内 z-index 排序**
    - **Validates: Requirements 1.4, 2.4, 5.2**

- [x] 3. 实现 LayerManager Hit Testing

  - [x] 3.1 实现 LayerManager::HitTest() 方法


    - 从 Modal -> Overlay -> Base 顺序测试
    - 调用各 Layer 的 HitTest 方法
    - _Requirements: 2.1, 2.2, 2.3_
  - [x] 3.2 编写属性测试 - Layer 优先级


    - **Property 5: Layer 优先级 Hit Testing**
    - **Validates: Requirements 2.1, 2.2, 5.1**
  - [x] 3.3 编写属性测试 - Hit Testing 穿透


    - **Property 6: Hit Testing 穿透**
    - **Validates: Requirements 2.3**
  - [x] 3.4 编写属性测试 - pointer-events


    - **Property 7: pointer-events: none 跳过**
    - **Validates: Requirements 2.5**

- [x] 4. 实现 LayerManager 滚动处理

  - [x] 4.1 实现 LayerManager::HandleWheel() 方法


    - 从 Modal -> Overlay 顺序处理
    - 返回 true 表示事件被处理
    - _Requirements: 3.1, 3.2, 3.3_
  - [x] 4.2 编写属性测试 - 滚动事件路由


    - **Property 8: 滚动事件路由**
    - **Validates: Requirements 3.1, 3.4**
  - [x] 4.3 编写属性测试 - 滚动事件阻止


    - **Property 9: 滚动事件阻止**
    - **Validates: Requirements 3.2**
  - [x] 4.4 编写属性测试 - 滚动事件穿透


    - **Property 10: 滚动事件穿透**
    - **Validates: Requirements 3.3**

- [x] 5. 实现状态查询和兼容性


  - [x] 5.1 实现 HasOverlays() 和 GetLayer() 方法

    - HasOverlays 检查 Overlay 和 Modal 层
    - GetLayer 返回指定层级的 Layer
    - _Requirements: 5.3_

  - [x] 5.2 编写属性测试 - HasOverlays 状态

    - **Property 11: HasOverlays 状态查询**
    - **Validates: Requirements 5.3**

  - [x] 5.3 编写属性测试 - 兼容性

    - **Property 12: 兼容性 - 元素收集**
    - **Validates: Requirements 6.1, 6.2**


- [x] 6. Checkpoint - 确保所有测试通过

  - Ensure all tests pass, ask the user if questions arise.

- [x] 7. 集成 HitTesting 模块


  - [x] 7.1 修改 hit_testing.h 添加 HitTestWithLayers 声明

    - 新增方法使用 LayerManager 进行 Hit Testing
    - _Requirements: 2.1, 2.2_
  - [x] 7.2 修改 hit_testing.cpp 实现 HitTestWithLayers


    - 先调用 LayerManager::HitTest
    - 如果未命中再调用原有的 HitTestRenderObject
    - _Requirements: 2.1, 2.2, 2.3_

- [x] 8. 集成 EventLoop 模块


  - [x] 8.1 修改 event_loop.cpp 使用 LayerManager Hit Testing

    - 在鼠标事件处理中使用 HitTestWithLayers
    - _Requirements: 4.1_
  - [x] 8.2 修改 event_loop.cpp 使用 LayerManager 滚动处理


    - 在滚轮事件处理中先调用 LayerManager::HandleWheel
    - _Requirements: 3.1, 3.2, 3.3_

  - [x] 8.3 修改 hover chain 更新逻辑

    - 使用 LayerManager 的 Hit Testing 结果
    - _Requirements: 4.1, 4.2, 4.3_

- [x] 9. 更新构建配置

  - [x] 9.1 修改 core/render/CMakeLists.txt


    - 添加 layer_manager.cpp 到源文件列表
    - _Requirements: 6.3_


- [x] 10. Checkpoint - 确保所有测试通过

  - Ensure all tests pass, ask the user if questions arise.

- [x] 11. 清理和迁移


  - [x] 11.1 更新 render_object.cpp 使用 LayerManager

    - 将 OverlayManager 调用替换为 LayerManager
    - _Requirements: 6.1, 6.2_
  - [x] 11.2 更新 window.cpp 使用 LayerManager


    - 将 OverlayManager 调用替换为 LayerManager
    - _Requirements: 6.1, 6.2_
  - [x] 11.3 标记 OverlayManager 为废弃


    - 添加废弃注释，后续版本删除
    - _Requirements: 6.3_

- [x] 12. Final Checkpoint - 确保所有测试通过



  - Ensure all tests pass, ask the user if questions arise.
