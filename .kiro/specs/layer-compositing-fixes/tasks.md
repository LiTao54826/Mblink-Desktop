# Implementation Plan: Layer Compositing Bug Fixes

## 阶段 1: 修复强制全量光栅化 (P0)

- [x] 1. 移除 window.cpp 中的 ForceRasterize 调用







  - [x] 1.1 修改 `window.cpp:1523-1526`

    - 删除 `compositor_adapter_->ForceRasterize()` 调用


    - 保留 `needs_repaint_ = true` 以继续触发渲染循环
    - _Requirements: 1.1, 1.3_



  - [x] 1.2 验证动画仍然正常播放


    - 运行 `build\bin\Release\esm_loader.exe examples\preact_demo\test_css_animation.js`


    - 确认动画帧持续更新
    - _Requirements: 1.1_


- [x] 2. 修复 render_pipeline_v2.cpp 中的 MarkFullDirty 滥用


  - [x] 2.1 修改 `BuildLayerTree()` 方法


    - 删除 `if (root_layer_ && CheckRenderObjectNeedsPaint(root)) { root_layer_->MarkFullDirty(); }`
    - 让各层自己追踪脏区域
    - _Requirements: 1.2_
  - [x] 2.2 修改 `UpdateLayerTreeBounds()` 方法

    - 只在层边界实际变化时标记脏区域
    - 不要无条件调用 `MarkFullDirty()`
    - _Requirements: 1.2_
  - [x] 2.3 修改 `CheckRenderObjectNeedsPaint()` 逻辑

    - 不再因为有动画配置就返回 true
    - 只在实际需要重绘时返回 true
    - _Requirements: 1.2_

- [x] 3. 检查点 - 验证增量光栅化生效

  - 运行测试，观察 CPU 使用率是否下降
  - 确认动画仍然正常播放
  - _Requirements: 1.4_

## 阶段 2: 实现 will-change 层提升 (P1)

- [x] 4. 添加 will_change 到 ComputedStyle


  - [x] 4.1 检查 ComputedStyle 是否已有 will_change 字段


    - 如果没有，添加 `std::string will_change;`
    - _Requirements: 2.3_

  - [x] 4.2 在 StyleResolver 中解析 will-change 属性

    - 解析 CSS `will-change` 属性值
    - 支持 "auto", "transform", "opacity", "transform, opacity" 等
    - _Requirements: 2.3_

- [x] 5. 实现 HasWillChangeTransform


  - [x] 5.1 修改 `layer_tree_builder.cpp:HasWillChangeTransform()`


    - 从 ComputedStyle 获取 will_change 值
    - 检查是否包含 "transform"
    - _Requirements: 2.1_
  - [x] 5.2 编写单元测试

    - 测试各种 will-change 值的解析
    - _Requirements: 2.1_

- [x] 6. 实现 HasWillChangeOpacity


  - [x] 6.1 修改 `layer_tree_builder.cpp:HasWillChangeOpacity()`

    - 从 ComputedStyle 获取 will_change 值
    - 检查是否包含 "opacity"
    - _Requirements: 2.2_
  - [x] 6.2 编写单元测试

    - 测试各种 will-change 值的解析
    - _Requirements: 2.2_

- [x] 7. 检查点 - 验证 will-change 层提升

  - 创建测试用例，元素设置 `will-change: transform`
  - 验证元素获得独立层
  - _Requirements: 2.4_

## 阶段 3: 修复动画检测逻辑 (P1)

- [x] 8. 改进 HasTransformAnimation


  - [x] 8.1 修改 `layer_tree_builder.cpp:HasTransformAnimation()`


    - 查询 AnimationController 获取运行中的动画
    - 检查 keyframes 定义是否影响 transform
    - _Requirements: 3.1, 3.4_
  - [x] 8.2 添加 AnimationController 引用到 LayerTreeBuilder

    - 在构造函数或 setter 中传入
    - _Requirements: 3.3_

- [x] 9. 改进 HasOpacityAnimation


  - [x] 9.1 修改 `layer_tree_builder.cpp:HasOpacityAnimation()`

    - 查询 AnimationController 获取运行中的动画
    - 检查 keyframes 定义是否影响 opacity
    - _Requirements: 3.2, 3.4_

- [x] 10. 检查点 - 验证动画检测

  - 运行 CSS 动画测试
  - 验证动画元素被正确检测并提升
  - _Requirements: 3.1, 3.2_

## 阶段 4: 集成 AnimationLayerBridge (P1)

- [x] 11. 在 Window 层暴露 AnimationLayerBridge


  - [x] 11.1 修改 WindowCompositorAdapter


    - 添加 `GetAnimationLayerBridge()` 方法
    - _Requirements: 4.1_
  - [x] 11.2 在 Window 中获取 bridge 引用

    - 在初始化时保存引用
    - _Requirements: 4.1_

- [x] 12. 在动画系统中添加通知


  - [x] 12.1 修改 AnimationController 或相关类

    - 在动画开始时调用 `OnAnimationStart()`
    - 在动画结束时调用 `OnAnimationEnd()`
    - _Requirements: 4.1, 4.2_
  - [x] 12.2 处理 CSS animation 和 transition

    - 两种动画类型都需要通知
    - _Requirements: 4.1, 4.2_

- [x] 13. 检查点 - 验证 AnimationLayerBridge 集成

  - 运行动画测试
  - 验证 OnAnimationStart/End 被正确调用
  - 验证层提升/降级正常工作
  - _Requirements: 4.3, 4.4_

## 阶段 5: 修复 RenderObject 层关联 (P2)

- [x] 14. 添加层关联方法到 RenderObject


  - [x] 14.1 修改 `render_object.h`

    - 添加 `SetCompositorLayer()` 方法
    - 确保 `HasOwnCompositorLayer()` 正确工作
    - _Requirements: 6.1, 6.2_

- [x] 15. 在 CreateLayer 中设置关联


  - [x] 15.1 修改 `layer_tree_builder.cpp:CreateLayer()`

    - 调用 `obj->SetCompositorLayer(layer)`
    - _Requirements: 6.1_

- [x] 16. 修复脏区域标记


  - [x] 16.1 修改脏区域标记逻辑


    - 当元素有独立层时，标记该层而非根层
    - _Requirements: 6.3_


- [x] 17. 检查点 - 验证层关联
  - 验证 `HasOwnCompositorLayer()` 返回正确值
  - 验证脏区域标记到正确的层
  - _Requirements: 6.1, 6.2, 6.3_

## 阶段 6: 修复重影问题 (P0)

- [x] 18. 确保动画元素有独立层


  - [x] 18.1 验证阶段 2-4 的修复


    - 动画元素应该自动获得独立层
    - 独立层移动时不会影响其他层
    - _Requirements: 5.3_


- [x] 19. 检查层边界更新


  - [x] 19.1 验证动画时层边界正确更新

    - transform 动画应该更新层的 transform，不是边界
    - _Requirements: 5.3_

- [x] 20. 最终验证


  - [x] 20.1 运行完整测试


    - `build\bin\Release\esm_loader.exe examples\preact_demo\test_css_animation.js`
    - 验证无重影
    - 验证 CPU/GPU 使用率正常
    - _Requirements: 5.1, 5.2, 5.4_
  - [x] 20.2 运行所有 compositor 测试


    - 确保没有回归
    - _Requirements: 5.1, 5.2_

## 阶段 7: 性能验证

- [x] 21. 性能对比测试



  - [x] 21.1 测量修复前后的 CPU 使用率
    - 记录 transform 动画时的 CPU 占用
    - _Requirements: 1.4_

  - [x] 21.2 测量修复前后的帧率
    - 确保 60fps 稳定
    - _Requirements: 1.4_

  - [x] 21.3 测量光栅化调用次数

    - 验证增量光栅化生效
    - _Requirements: 1.4_

- [x] 22. 最终检查点
  - 所有测试通过
  - 性能指标达标
  - 无视觉问题
