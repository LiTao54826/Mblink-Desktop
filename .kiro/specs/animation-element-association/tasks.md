# Implementation Plan

## 强制要求

> **自动化测试规范** (参考 #[[file:.kiro/steering/build-and-test.md]])
> 
> 1. **每个任务完成后必须编译验证**：`cmake --build build --config Release --target esm_loader`
> 2. **每个 Checkpoint 必须运行自动化测试**：
>    ```cmd
>    build\bin\Release\esm_loader.exe tests\js\test_modal_animation.js -q 5 >> debuglog.txt
>    findstr "TEST_PASS TEST_FAIL ERROR" debuglog.txt
>    del debuglog.txt
>    ```
> 3. **测试失败时自动修复**：除非遇到需要人工判断的情况，否则自动排查并修复问题
> 4. **记录排除方向**：修复 bug 时，每排除一个方向就记录，避免反复排查已排除的问题
> 5. **测试通过后才能继续下一个任务**

> **真实 UI 测试验收规范**
> 
> 1. **使用 JS 测试脚本进行真实 UI 测试**：测试脚本必须创建真实的 DOM 元素和动画
> 2. **配合 C++/JS 日志验证**：
>    - JS 端使用 `console.log("[TEST_PASS]")` / `console.log("[TEST_FAIL]")` 输出测试结果
>    - C++ 端可通过 `LOG_DEBUG` 输出关键状态信息（如动画数量、Element 关联状态）
> 3. **测试场景必须覆盖**：
>    - 动画启动后添加新 DOM 元素（如 Modal）
>    - 渲染树重建后动画是否继续运行
>    - 动画关联的 Element 被移除后是否正确清理
> 4. **测试脚本模板**：
>    ```javascript
>    // 创建带动画的元素
>    const spinner = document.createElement('div');
>    spinner.style.animation = 'spin 1s infinite';
>    document.body.appendChild(spinner);
>    
>    // 触发 DOM 变化（如添加 Modal）
>    setTimeout(() => {
>        const modal = document.createElement('div');
>        modal.className = 'modal';
>        document.body.appendChild(modal);
>        
>        // 验证动画是否继续运行
>        // 通过检查 C++ 日志或 JS API 获取动画状态
>    }, 500);
>    ```
> 5. **自动化验收**：测试脚本自动判断通过/失败，无需人工干预

---

- [x] 1. 修改 RunningAnimation 和 RunningTransition 结构体


  - [x] 1.1 修改 RunningAnimation 结构体使用 Element 引用


    - 将 `RenderObject* object` 改为 `std::weak_ptr<Element> element`
    - 添加 `GetRenderObject()` 方法
    - 添加 `IsValid()` 方法
    - 文件: `core/render/animation/animation_controller.h`
    - _Requirements: 2.1_

  - [ ]* 1.2 编写 RunningAnimation 属性测试
    - **Property 5: Graceful Handling of Null RenderObject**
    - **Validates: Requirements 2.4, 3.4, 4.3**

  - [x] 1.3 修改 RunningTransition 结构体使用 Element 引用


    - 将 `RenderObject* object` 改为 `std::weak_ptr<Element> element`
    - 添加 `GetRenderObject()` 方法
    - 添加 `IsValid()` 方法
    - 文件: `core/render/animation/animation_timeline.h`
    - _Requirements: 3.1_

- [x] 2. 重构 AnimationController


  - [x] 2.1 添加 Element 版本的 StartAnimation 方法

    - 新增 `StartAnimation(std::shared_ptr<Element>, const CSSAnimation&)`
    - 修改原有 `StartAnimation(RenderObject*, ...)` 内部提取 Element
    - 文件: `core/render/animation/animation_controller.h`, `animation_controller.cpp`
    - _Requirements: 2.3_

  - [x] 2.2 修改 AnimationController::Update 使用 Element 获取 RenderObject

    - 遍历动画时通过 `anim.GetRenderObject()` 获取当前 RenderObject
    - 如果 RenderObject 为 null，跳过应用但保留状态
    - 清理无效动画（Element 已销毁）
    - 文件: `core/render/animation/animation_controller.cpp`
    - _Requirements: 2.2, 2.4_

  - [ ]* 2.3 编写 AnimationController 属性测试
    - **Property 1: Animation State Preservation Across Render Tree Rebuilds**
    - **Validates: Requirements 1.1, 1.2**

  - [x] 2.4 修改 FindAnimation 和其他查找方法

    - 使用 Element 指针作为查找键
    - 更新 `StopAnimation`, `PauseAnimation`, `ResumeAnimation` 等方法
    - 文件: `core/render/animation/animation_controller.cpp`
    - _Requirements: 2.2_

  - [ ]* 2.5 编写 AnimationController 属性测试
    - **Property 4: RenderObject Lookup from Element**
    - **Validates: Requirements 2.2, 3.2, 4.1, 5.3**

- [x] 3. Checkpoint - 确保所有测试通过


  - Ensure all tests pass, ask the user if questions arise.

- [x] 4. 重构 AnimationTimeline


  - [x] 4.1 添加 Element 版本的 StartTransition 方法

    - 新增 `StartTransition(std::shared_ptr<Element>, ...)`
    - 修改原有方法内部提取 Element
    - 文件: `core/render/animation/animation_timeline.h`, `animation_timeline.cpp`
    - _Requirements: 3.3_

  - [x] 4.2 修改 AnimationTimeline::Update 使用 Element 获取 RenderObject

    - 遍历过渡时通过 `trans.GetRenderObject()` 获取当前 RenderObject
    - 如果 RenderObject 为 null，跳过应用但保留状态
    - 清理无效过渡
    - 文件: `core/render/animation/animation_timeline.cpp`
    - _Requirements: 3.2, 3.4_

  - [x] 4.3 修改 GetCurrentValue 和其他查找方法

    - 使用 Element 指针作为查找键
    - 更新 `StopTransition`, `StopAllTransitions` 等方法
    - 文件: `core/render/animation/animation_timeline.cpp`
    - _Requirements: 3.2_

- [x] 5. 重构 AnimationApplicator


  - [x] 5.1 添加 Element 版本的方法


    - 修改 `started_animations_` 使用 `Element*` 作为键
    - 添加 `ExtractElement()` 方法从 RenderObject 获取 Element
    - 修改内部实现通过 Element 获取 RenderObject
    - 文件: `core/render/animation/animation_applicator.h`, `animation_applicator.cpp`
    - _Requirements: 4.1, 4.2, 4.3_

  - [ ]* 5.2 编写 AnimationApplicator 属性测试
    - **Property 2: Animation Re-association After Rebuild**
    - **Validates: Requirements 1.3, 2.3, 3.3**

- [x] 6. Checkpoint - 确保所有测试通过

  - Ensure all tests pass, ask the user if questions arise.

- [x] 7. 重构 AnimationOptimizer



  - [x] 7.1 AnimationDirtyTracker 保持使用 RenderObject* 键


    - 注意：AnimationDirtyTracker 仍使用 `RenderObject*` 作为键
    - 这是因为脏标记是每帧重置的，不需要跨渲染树重建保持
    - 文件: `core/render/animation/animation_optimizer.h`, `animation_optimizer.cpp`
    - _Requirements: 5.1_


  - [x] 7.2 BatchAnimationUpdater 保持使用 RenderObject* 键

    - 注意：BatchAnimationUpdater 仍使用 `RenderObject*`
    - 批量更新是每帧操作，不需要跨渲染树重建保持
    - 文件: `core/render/animation/animation_optimizer.h`, `animation_optimizer.cpp`

    - _Requirements: 5.1_

- [x] 8. AnimationLayerBridge 保持现有实现



  - [x] 8.1 AnimationLayerBridge 保持使用 RenderObject* 键
    - 注意：AnimationLayerBridge 仍使用 `RenderObject*` 作为键
    - 层桥接器跟踪的是层状态，与 RenderObject 生命周期一致
    - 渲染树重建时层也会重建，所以不需要改为 Element 引用
    - 文件: `core/compositor/animation/animation_layer_bridge.h`, `animation_layer_bridge.cpp`
    - _Requirements: 5.2, 5.3_

  - [ ]* 8.2 编写 AnimationLayerBridge 属性测试
    - **Property 3: Animation Cleanup on Element Removal**
    - **Validates: Requirements 1.4**

- [x] 9. 修改 Window::InvalidateRenderTree


  - [x] 9.1 移除 InvalidateRenderTree 中清除动画的代码


    - 已确认：不再调用 `animation_controller_->ClearRunningAnimations()`
    - 已确认：不再调用 `animation_timeline_->StopAll()`
    - 只保留渲染树和层树的失效逻辑，以及 AnimationApplicator::Clear()
    - 文件: `core/window/window.cpp`
    - _Requirements: 6.1, 6.2, 6.3_

  - [ ]* 9.2 编写 InvalidateRenderTree 属性测试
    - **Property 6: InvalidateRenderTree Does Not Clear Animations**
    - **Validates: Requirements 6.1, 6.2, 6.3**


- [x] 10. Checkpoint - 确保所有测试通过

  - Ensure all tests pass, ask the user if questions arise.

- [x] 11. 集成测试

  - [x] 11.1 更新 Modal 动画测试


    - 修改 `tests/js/test_modal_animation.js`
    - 验证 spinner 动画在 Modal 打开后继续运行
    - 需要添加 @keyframes 规则并验证动画状态
    - _Requirements: 1.1_

  - [ ]* 11.2 编写 DOM 变化集成测试
    - 测试添加/删除兄弟元素时动画不受影响
    - 测试渲染树完整重建时动画状态保持
    - _Requirements: 1.2, 1.3_

- [x] 12. Final Checkpoint - 确保所有测试通过


  - Ensure all tests pass, ask the user if questions arise.
