# 实现计划

## 测试要求

**重要**: 所有测试必须连接真实的渲染层，不使用 mock 对象：
- 使用 `DOMTestBase` 创建真实的 `Document`
- 使用真实的 `StyleManager`、`AnimationController`、`StyleResolver`
- 使用真实的 `RenderObject`（`RenderBlock`、`RenderInline` 等）
- 测试文件放在 `tests/property/render/` 目录下

## 1. StyleManager @keyframes 解析集成

- [x] 1.1 在 StyleManager 中添加 AnimationController 成员
  - 在 `core/lexbor/style_manager.h` 中添加 `AnimationController animation_controller_` 成员
  - 添加 `GetAnimationController()` 和 `const GetAnimationController()` 方法
  - _需求: 1.1_

- [x] 1.2 实现 @keyframes 规则提取
  - 在 `core/lexbor/style_manager.cpp` 中实现 `FindKeyframesBlocks()` 方法
  - 使用手动解析处理嵌套大括号（关键帧内的属性块）
  - _需求: 1.1, 1.3_

- [x] 1.3 实现 @keyframes 自动注册
  - 在 `ParseCSSString()` 方法中调用 `ExtractAndRegisterKeyframes()`
  - 对每个提取的 @keyframes 块调用 `KeyframesRule::Parse()`
  - 将解析后的规则注册到 `animation_controller_`
  - _需求: 1.1, 1.2, 1.4_

- [x] 1.4 编写 @keyframes 解析属性测试
  - 创建 `tests/property/render/test_animation_keyframes_properties.cpp`
  - 继承 `DOMTestBase`，使用真实 `StyleManager`
  - **属性 1: @keyframes 注册一致性**
  - **验证: 需求 1.1, 1.3, 1.4**

- [x] 1.5 编写 @keyframes 级联覆盖属性测试
  - 在同一测试文件中添加
  - 使用真实 `StyleManager` 解析多个同名 @keyframes
  - **属性 2: @keyframes 级联覆盖**
  - **验证: 需求 1.2**

## 2. ComputedStyle 动画字段扩展

- [x] 2.1 在 ComputedStyle 中添加动画字段
  - 在 `core/render/render_object.h` 的 `ComputedStyle` 结构体中添加:
    - `std::vector<CSSAnimation> animations`
    - `std::string animation_play_state = "running"`
  - _需求: 2.1_

- [x] 2.2 在 StyleResolver 中实现动画属性解析
  - 在 `core/render/style_resolver.cpp` 中添加 `ParseAnimationProperty()` 方法
  - 处理 `animation` 简写属性
  - 处理各个 `animation-*` 分解属性
  - _需求: 2.1, 2.2, 2.3_

- [x] 2.3 支持多动画解析
  - 处理逗号分隔的多个动画配置
  - 确保所有动画都存储在 `ComputedStyle.animations` 中
  - _需求: 2.4_

- [x] 2.4 编写动画简写解析属性测试
  - 创建 `tests/property/render/test_animation_parsing_properties.cpp`
  - 继承 `DOMTestBase`，使用真实 `Document` 和 `RenderObject`
  - **属性 3: 动画简写解析完整性**
  - **验证: 需求 2.1, 2.2**

- [x] 2.5 编写多动画保留属性测试
  - 在同一测试文件中添加
  - 测试逗号分隔的多个动画配置
  - **属性 4: 多动画保留**
  - **验证: 需求 2.4**

## 3. 检查点 - 确保所有测试通过
- [x] 3. 确保所有测试通过，如有问题请询问用户。

## 4. AnimationApplicator 实现

- [x] 4.1 创建 AnimationApplicator 类
  - 创建 `core/render/animation_applicator.h` 和 `animation_applicator.cpp`
  - 实现构造函数，接收 `AnimationController&` 引用
  - _需求: 3.3_

- [x] 4.2 实现 StartAnimationsForObject 方法
  - 读取 RenderObject 的 ComputedStyle.animations
  - 为每个动画调用 `AnimationController::StartAnimation()`
  - 跟踪已启动的动画名称
  - _需求: 3.2_

- [x] 4.3 实现 ApplyAnimationValues 方法
  - 获取当前动画属性值 (`GetCurrentProperties`)
  - 将属性值应用到 RenderObject 的 ComputedStyle
  - 标记 RenderObject 需要重绘
  - _需求: 3.3, 3.4_

- [x] 4.4 实现 ApplyPropertyToStyle 辅助方法
  - 根据属性名将值应用到 ComputedStyle 的对应字段
  - 支持常见可动画属性（opacity, transform, color, width, height 等）
  - _需求: 3.3_

- [x] 4.5 编写属性插值正确性属性测试
  - 创建 `tests/property/render/test_animation_interpolation_properties.cpp`
  - 使用真实 `PropertyInterpolation` 类
  - 测试数值、颜色属性的插值
  - **属性 6: 属性插值正确性**
  - **验证: 需求 3.2, 3.3**

## 5. 渲染循环集成

- [x] 5.1 在渲染循环中添加动画更新调用
  - 在主渲染循环中获取当前高精度时间戳
  - 调用 `AnimationController::Update(current_time)`
  - _需求: 3.1_

- [x] 5.2 实现渲染树动画应用遍历
  - 递归遍历渲染树
  - 对每个 RenderObject 调用 `AnimationApplicator::ApplyAnimationValues()`
  - _需求: 3.3_

- [x] 5.3 处理动画启动时机
  - 在样式解析后检测新动画
  - 自动启动 ComputedStyle 中定义的动画
  - _需求: 3.2_

- [x]* 5.4 编写动画进度计算属性测试
  - 创建 `tests/property/render/test_animation_progress_properties.cpp`
  - 继承 `DOMTestBase`，使用真实 `AnimationController`
  - 测试不同时间点的动画进度计算
  - **属性 5: 动画进度计算**
  - **验证: 需求 3.2**

## 6. 检查点 - 确保所有测试通过
- [x] 6. 确保所有测试通过，如有问题请询问用户。

## 7. Transform 插值增强

- [x] 7.1 实现 DecomposedTransform 结构体
  - 在 `core/render/property_interpolation.h` 中定义结构体
  - 包含 translate_x, translate_y, rotate, scale_x, scale_y, skew_x, skew_y 字段
  - _需求: 4.1_

- [x] 7.2 实现 DecomposeTransform 方法
  - 解析 transform 字符串中的各个函数
  - 提取 translate, rotate, scale, skew 值
  - 处理不同的单位（px, deg, rad, %）
  - _需求: 4.1_

- [x] 7.3 实现 InterpolateDecomposed 方法
  - 对每个组件进行线性插值
  - 处理角度插值（选择最短路径）
  - _需求: 4.2_

- [x] 7.4 实现 ComposeTransform 方法
  - 将分解的组件重新组合为 CSS transform 字符串
  - 按正确顺序输出 transform 函数
  - _需求: 4.3_

- [x] 7.5 更新 InterpolateTransform 方法
  - 替换现有的阶跃函数实现（当前只是 `factor < 0.5f ? from : to`）
  - 使用新的分解-插值-重组流程
  - 处理分解失败的回退情况
  - _需求: 4.1, 4.2, 4.3, 4.4_

- [x]* 7.6 编写 Transform 往返属性测试
  - 创建 `tests/property/render/test_transform_interpolation_properties.cpp`
  - 使用真实 `PropertyInterpolation::DecomposeTransform` 和 `ComposeTransform`
  - 测试分解-重组的往返一致性
  - **属性 7: Transform 分解往返**
  - **验证: 需求 4.1, 4.3**

- [x]* 7.7 编写 Transform 插值线性属性测试
  - 在同一测试文件中添加
  - 测试各组件的线性插值正确性
  - **属性 8: Transform 插值线性**
  - **验证: 需求 4.2**

## 8. 动画事件系统

- [x] 8.1 在 RenderObject 中添加获取 Element 的方法
  - 添加 `GetElement()` 方法，从关联的 Node 获取 Element
  - 处理 Node 不是 Element 的情况
  - _需求: 5.1, 5.2, 5.3_

- [x] 8.2 完善 FireAnimationEvent 实现
  - 在 `core/render/animation_controller.cpp` 中完成实现
  - 创建 AnimationEvent 对象
  - 在关联的 Element 上分发事件
  - _需求: 5.1, 5.2, 5.3, 5.4_

- [x] 8.3 确保事件在正确时机触发
  - animationstart: 延迟结束后
  - animationend: 所有迭代完成后
  - animationiteration: 每次迭代完成时
  - _需求: 5.1, 5.2, 5.3_

- [x]* 8.4 编写动画事件时机属性测试
  - 创建 `tests/property/render/test_animation_events_properties.cpp`
  - 继承 `DOMTestBase`，使用真实 `Element` 和事件系统
  - 通过 `AddEventListener` 捕获真实事件
  - **属性 9: 动画事件时机**
  - **验证: 需求 5.1, 5.2**

- [x]* 8.5 编写动画事件属性属性测试
  - 在同一测试文件中添加
  - 验证 `AnimationEvent` 的 `animationName`、`elapsedTime` 等属性
  - **属性 10: 动画事件属性**
  - **验证: 需求 5.4**

## 9. 检查点 - 确保所有测试通过
- [x] 9. 确保所有测试通过，如有问题请询问用户。

## 10. Fill-Mode 支持

- [x] 10.1 实现 forwards 填充模式
  - 动画结束后保留最后一帧的样式
  - 在 AnimationController 中处理 FINISHED 状态
  - _需求: 6.1_

- [x] 10.2 实现 backwards 填充模式
  - 延迟期间应用第一帧的样式
  - 在 DELAYED 状态时计算并应用样式
  - _需求: 6.2_

- [x] 10.3 实现 both 填充模式
  - 组合 forwards 和 backwards 行为
  - _需求: 6.3_

- [x] 10.4 实现 none 填充模式
  - 确保动画时间外不应用关键帧样式
  - _需求: 6.4_

- [x]* 10.5 编写 forwards 填充模式属性测试
  - 创建 `tests/property/render/test_animation_fillmode_properties.cpp`
  - 继承 `DOMTestBase`，使用真实渲染树
  - 验证动画结束后 `ComputedStyle` 保留最后一帧样式
  - **属性 11: Fill-mode forwards 行为**
  - **验证: 需求 6.1, 6.3**

- [x]* 10.6 编写 backwards 填充模式属性测试
  - 在同一测试文件中添加
  - 验证延迟期间 `ComputedStyle` 应用第一帧样式
  - **属性 12: Fill-mode backwards 行为**
  - **验证: 需求 6.2, 6.3**

## 11. Play-State 控制

- [x] 11.1 实现 animation-play-state 解析
  - 在 StyleResolver 中解析 animation-play-state 属性（已在 ParseAnimationProperty 中实现）
  - 支持 "running" 和 "paused" 值
  - _需求: 7.1, 7.2_

- [x] 11.2 实现暂停状态处理
  - 在 AnimationApplicator 中检测 play-state 变化
  - 调用 AnimationController 的 PauseAnimation/ResumeAnimation
  - _需求: 7.1, 7.2_

- [x] 11.3 支持通过 JavaScript 控制
  - 确保 style.animationPlayState 设置立即生效
  - _需求: 7.3_

- [x]* 11.4 编写暂停状态保留属性测试
  - 创建 `tests/property/render/test_animation_playstate_properties.cpp`
  - 继承 `DOMTestBase`，使用真实 `AnimationController`
  - 验证暂停后动画进度不变
  - **属性 13: 暂停状态保留**
  - **验证: 需求 7.1**

- [x]* 11.5 编写恢复位置连续性属性测试
  - 在同一测试文件中添加
  - 验证恢复后从暂停位置继续
  - **属性 14: 恢复位置连续性**
  - **验证: 需求 7.2**

## 12. 最终检查点 - 确保所有测试通过
- [x] 12. 确保所有测试通过，如有问题请询问用户。
