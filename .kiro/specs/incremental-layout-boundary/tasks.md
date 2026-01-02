# Implementation Plan

## 强制要求

> **自动化测试规范** (参考 #[[file:.kiro/steering/build-and-test.md]])
> 
> 1. **每个任务完成后必须编译验证**：`cmake --build build --config Release --target esm_loader`
> 2. **每个 Checkpoint 必须运行自动化测试**：
>    ```cmd
>    build\bin\Release\esm_loader.exe tests\js\test_xxx.js -q 5 >> debuglog.txt
>    findstr "TEST_PASS TEST_FAIL ERROR" debuglog.txt
>    del debuglog.txt
>    ```
> 3. **测试失败时自动修复**：除非遇到需要人工判断的情况，否则自动排查并修复问题
> 4. **测试通过后才能继续下一个任务**
> 5. **真实 UI 测试**：所有集成测试必须通过 JS 脚本配合日志输出进行自动化验证，确保真实 UI 效果正确
> 6. **禁止跳过测试**：不允许跳过任何测试步骤，必须确保每个功能点都有对应的自动化测试覆盖
> 7. **日志标记规范**：测试脚本必须使用 `[TEST_START]`、`[TEST_PASS]`、`[TEST_FAIL]`、`[TEST_END]`、`[ERROR]` 标记



---



- [ ] 1. 扩展 ComputedStyle 支持 CSS Containment


  - [x] 1.1 添加 contain 属性到 ComputedStyle


    - 添加 `std::string contain = "none"` 字段
    - 添加 `HasLayoutContainment()` 方法
    - 添加 `HasSizeContainment()` 方法


    - 文件: `core/render/css/computed_style.h`


    - _Requirements: 4.4_

  - [ ] 1.2 解析 contain CSS 属性
    - 在 StyleResolver 中添加 contain 属性解析
    - 支持 none, layout, paint, size, style, content, strict
    - 文件: `core/render/css/style_resolver.cpp`
    - _Requirements: 4.4_


- [ ] 2. 实现 LayoutBoundaryDetector

  - [ ] 2.1 创建 LayoutBoundaryDetector 类
    - 定义 BoundaryType 枚举
    - 实现 DetectBoundaryType() 方法
    - 文件: `core/layout/layout_boundary_detector.h`, `layout_boundary_detector.cpp`
    - _Requirements: 5.1_


  - [ ]* 2.2 编写 LayoutBoundaryDetector 属性测试
    - **Property 6: Layout Boundary Detection**
    - **Validates: Requirements 5.1**

  - [ ] 2.3 实现边界检测辅助方法
    - IsOutOfFlow() - 检测 fixed/absolute
    - IsScrollContainer() - 检测 overflow: scroll/auto
    - HasFixedSize() - 检测固定尺寸


    - HasLayoutContainment() - 检测 CSS Containment


    - IsFlexFixedItem() - 检测 Flex 固定项


    - 文件: `core/layout/layout_boundary_detector.cpp`
    - _Requirements: 1.1, 1.2, 2.1, 3.3, 4.1_

  - [ ] 2.4 实现 FindNearestLayoutBoundary 方法
    - 向上遍历祖先链查找最近的布局边界

    - 支持 Node* 和 RenderObject* 两个版本
    - 文件: `core/layout/layout_boundary_detector.cpp`
    - _Requirements: 5.2_



  - [x]* 2.5 编写祖先查找属性测试

    - **Property 7: Nearest Boundary Traversal**

    - **Validates: Requirements 5.2, 5.3**

- [x] 3. Checkpoint - 确保编译通过

  - Ensure all tests pass, ask the user if questions arise.

- [ ] 4. 扩展 RenderObject 布局边界支持

  - [ ] 4.1 添加 IsLayoutBoundary 方法到 RenderObject
    - 添加 `IsLayoutBoundary()` 方法
    - 添加 `GetLayoutBoundaryType()` 方法
    - 添加缓存字段 `cached_boundary_type_`
    - 文件: `core/render/objects/render_object.h`, `render_object.cpp`
    - _Requirements: 5.1_


  - [ ] 4.2 实现布局边界缓存更新
    - 在样式变化时更新缓存
    - 添加 `UpdateLayoutBoundaryCache()` 方法
    - 文件: `core/render/objects/render_object.cpp`
    - _Requirements: 5.1_

- [ ] 5. 实现 IncrementalLayoutManager

  - [ ] 5.1 创建 IncrementalLayoutManager 类
    - 定义类结构和成员变量

    - 添加 dirty_boundaries_ 集合
    - 文件: `core/layout/incremental_layout_manager.h`, `incremental_layout_manager.cpp`
    - _Requirements: 5.3_

  - [ ] 5.2 实现脱离文档流元素处理
    - AddOutOfFlowElement() - 直接创建 RenderObject
    - RemoveOutOfFlowElement() - 直接移除 RenderObject


    - 不触发 InvalidateRenderTree


    - 文件: `core/layout/incremental_layout_manager.cpp`


    - _Requirements: 1.1, 1.2, 1.3_

  - [ ]* 5.3 编写脱离文档流元素属性测试
    - **Property 1: Out-of-Flow Element Isolation**


    - **Validates: Requirements 1.1, 1.2, 1.3**

  - [ ] 5.4 实现布局边界标记
    - MarkBoundaryNeedsLayout() - 标记边界需要重新布局
    - 只影响边界内的子树

    - 文件: `core/layout/incremental_layout_manager.cpp`
    - _Requirements: 2.1, 2.2, 3.1, 3.2, 4.1, 4.2, 4.3_

  - [ ]* 5.5 编写布局边界属性测试
    - **Property 2: Scroll Container Boundary**
    - **Property 4: Fixed-Size Container Boundary**
    - **Property 5: CSS Containment Boundary**
    - **Validates: Requirements 2.1, 2.2, 2.4, 3.1, 3.2, 4.1, 4.2, 4.3**


  - [ ] 5.6 实现滚动容器尺寸更新
    - UpdateScrollContainerSize() - 更新 scrollHeight/scrollWidth

    - 文件: `core/layout/incremental_layout_manager.cpp`
    - _Requirements: 2.3_

  - [ ]* 5.7 编写滚动尺寸更新属性测试
    - **Property 3: Scroll Dimensions Update**
    - **Validates: Requirements 2.3**

- [x] 6. Checkpoint - 确保编译通过

  - Ensure all tests pass, ask the user if questions arise.

- [ ] 7. 修改 WindowDOMObserver 集成增量布局

  - [ ] 7.1 添加 IncrementalLayoutManager 到 Window
    - 在 Window 类中添加 IncrementalLayoutManager 成员
    - 初始化和生命周期管理
    - 文件: `core/window/window.h`, `window.cpp`

    - _Requirements: 5.3_

  - [x] 7.2 修改 OnNodeAdded 使用增量布局


    - 检测脱离文档流元素，调用 AddOutOfFlowElement
    - 查找布局边界，调用 MarkBoundaryNeedsLayout
    - 无边界时回退到 InvalidateRenderTree


    - 文件: `core/window/window_dom_observer.cpp`
    - _Requirements: 1.1, 1.2, 2.1, 3.1, 4.1, 5.2, 5.3, 5.4_

  - [x] 7.3 修改 OnNodeRemoved 使用增量布局

    - 检测脱离文档流元素，调用 RemoveOutOfFlowElement
    - 查找布局边界，调用 MarkBoundaryNeedsLayout
    - 无边界时回退到 InvalidateRenderTree
    - 文件: `core/window/window_dom_observer.cpp`
    - _Requirements: 1.3, 2.2, 3.2, 4.2, 4.3, 5.2, 5.3, 5.4_



  - [ ]* 7.4 编写回退逻辑属性测试
    - **Property 8: Fallback to Full Rebuild**
    - **Validates: Requirements 5.4**

- [ ] 8. 处理边界情况

  - [ ] 8.1 处理嵌套布局边界
    - 使用最内层的布局边界
    - 文件: `core/layout/layout_boundary_detector.cpp`
    - _Requirements: 6.3_

  - [ ]* 8.2 编写嵌套边界属性测试
    - **Property 9: Nested Boundary Resolution**
    - **Validates: Requirements 6.3**

  - [ ] 8.3 处理 auto 尺寸排除
    - height: auto 或 width: auto 的容器不作为边界
    - 文件: `core/layout/layout_boundary_detector.cpp`
    - _Requirements: 6.2_

  - [ ]* 8.4 编写 auto 尺寸排除属性测试
    - **Property 10: Auto-Size Exclusion**
    - **Validates: Requirements 6.2**

  - [ ] 8.5 处理动态样式变化
    - 样式变化导致边界状态改变时触发适当的布局
    - 文件: `core/window/window_dom_observer.cpp`
    - _Requirements: 6.4_

- [ ] 9. Checkpoint - 确保编译通过
  - Ensure all tests pass, ask the user if questions arise.

- [ ] 10. 真实 UI 集成测试（自动化验证）

  - [ ] 10.1 创建 Modal 增量布局测试
    - 测试 Modal 打开/关闭不触发全量重建
    - 验证其他元素布局不受影响
    - **必须通过 JS 脚本 + 日志输出自动化验证**
    - 使用 `console.log('[TEST_PASS]')` / `console.log('[TEST_FAIL]')` 标记测试结果
    - 文件: `tests/js/test_modal_incremental.js`
    - _Requirements: 1.1, 1.3_

  - [ ] 10.2 创建虚拟列表增量布局测试
    - 测试滚动容器内元素增删
    - 验证只有滚动容器被标记为脏
    - **必须通过 JS 脚本 + 日志输出自动化验证**
    - 验证 scrollHeight/scrollWidth 正确更新
    - 文件: `tests/js/test_virtual_list_incremental.js`
    - _Requirements: 2.1, 2.2, 2.3, 2.4_

  - [ ] 10.3 创建 Toast 增量布局测试
    - 测试 Toast 显示/隐藏不触发全量重建
    - **必须通过 JS 脚本 + 日志输出自动化验证**
    - 文件: `tests/js/test_toast_incremental.js`
    - _Requirements: 1.1, 1.3_

  - [ ] 10.4 创建固定尺寸容器测试
    - 测试固定宽高容器内元素增删不影响祖先布局
    - **必须通过 JS 脚本 + 日志输出自动化验证**
    - 文件: `tests/js/test_fixed_size_container.js`
    - _Requirements: 3.1, 3.2, 3.3_

  - [ ]* 10.5 创建 CSS Containment 测试
    - 测试 contain: layout/strict/content
    - **必须通过 JS 脚本 + 日志输出自动化验证**
    - 文件: `tests/js/test_css_containment.js`
    - _Requirements: 4.1, 4.2, 4.3_

- [ ] 11. Final Checkpoint - 运行所有自动化测试
  - 运行所有 test_*.js 测试脚本
  - 使用 `findstr "TEST_FAIL"` 检查是否有失败
  - 确保所有测试通过后才能完成
  - Ensure all tests pass, ask the user if questions arise.
