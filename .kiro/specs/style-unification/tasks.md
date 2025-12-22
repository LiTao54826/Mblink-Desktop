# Implementation Plan

## Phase 1: Flexbox 统一

- [x] 1. 修改 Flexbox 接口定义



  - [x] 1.1 修改 `flex_layout.h` 中的 `LayoutFlexboxContainer` 接口，使用 `const Style&` 替代专门结构

    - 将 `GetFlexboxContainerStyle()` 改为返回 `const Style&`
    - 将 `GetFlexboxChildStyle()` 改为返回 `const Style&`
    - _Requirements: 1.1_



- [x] 2. 修改 Flex 布局算法使用新接口






  - [x] 2.1 修改 `flex_layout.cpp` 中的 `GenerateAnonymousFlexItems()` 函数

    - 从 `Style` 读取 flex_grow, flex_shrink, align_self 等属性
    - 处理 `optional` 类型属性使用 `.value_or()`
    - _Requirements: 1.2_

  - [x] 2.2 修改 `DetermineFlexBaseSize()` 函数使用新接口

    - _Requirements: 1.2_

  - [x] 2.3 修改 `ComputeFlexboxLayout()` 和 `ComputePreliminary()` 函数

    - _Requirements: 1.2_

- [x] 3. 修改 NativeLayoutEngine Flexbox 适配










  - [x] 3.1 创建 `FlexboxContainerAdapter` 适配器类包装 NativeLayoutEngine

    - 实现 `LayoutFlexboxContainer` 接口
    - 返回 `node->style` 而非专门的 style 结构
    - _Requirements: 1.1, 1.2_

  - [x] 3.2 修改 `ComputeFlexLayout()` 使用适配器
    - _Requirements: 1.2_


- [x] 4. Checkpoint - 确保 Flexbox 测试通过








  - 运行 `test_flex_center.js` 验证居中对齐
  - 运行 `test_radio.js` 验证动态样式变化
  - _Requirements: 6.1, 6.2_

## Phase 2: Grid 统一


- [x] 5. 修改 Grid 接口定义





  - [x] 5.1 修改 `grid/grid.h` 中的 `LayoutGridContainer` 接口

    - 使用 `const Style&` 替代 `GridContainerStyle`
    - 保留 Grid 特有数据的获取方法
    - _Requirements: 2.1_











- [x] 6. 修改 Grid 布局算法






  - [x] 6.1 修改 `grid/grid.cpp` 使用新接口

    - 从 `Style` 读取通用属性

    - _Requirements: 2.2_

- [x] 7. 修改 NativeLayoutEngine Grid 适配


  - [x] 7.1 创建 `GridContainerAdapter` 适配器类

    - _Requirements: 2.1, 2.2_





- [x] 8. Checkpoint - 确保 Grid 测试通过





  - 创建并运行 Grid 布局测试用例
  - _Requirements: 6.1_

## Phase 3: Block 统一


- [x] 9. 修改 Block 接口定义









  - [x] 9.1 修改 `types/traits.h` 中的 `LayoutBlockContainer` 接口


    - 使用 `const Style&` 替代 `BlockContainerStyle`
    - _Requirements: 3.1_


- [x] 10. 修改 Block 布局算法





  - [x] 10.1 修改 `block_layout.cpp` 使用新接口

    - _Requirements: 3.2_


- [x] 11. 修改 NativeLayoutEngine Block 适配










  - [x] 11.1 修改 `GetBlockContainerStyle()` 和 `GetBlockChildStyle()` 返回 `node->style`

    - _Requirements: 3.1, 3.2_



- [x] 12. Checkpoint - 确保 Block 测试通过







  - 测试 Block + Flex 混合布局
  - _Requirements: 6.1, 6.3_

## Phase 4: 清理与优化

- [x] 13. 简化 LayoutNode 结构






  - [x] 13.1 从 `native_layout_engine.h` 中移除冗余 style 字段

    - 移除 `block_container_style`, `block_item_style`
    - 移除 `flexbox_container_style`, `flexbox_item_style`
    - 移除 `grid_container_style`, `grid_item_style`
    - _Requirements: 4.1_

- [x] 14. 简化 UpdateStyle 函数






  - [x] 14.1 移除 `native_layout_engine.cpp` 中所有冗余的 style 同步代码

    - 只保留 `node->style = new_style`
    - _Requirements: 5.1_

- [x] 15. 简化 BuildSubtree 函数






  - [x] 15.1 移除 `CreateNode()` 中所有冗余的 style 初始化代码

    - 只保留 `node.style = ConvertStyle(computed)`
    - _Requirements: 5.2_

- [x] 16. Final Checkpoint - 确保所有测试通过





  - 运行所有组件测试
  - 验证 `UpdateStyle` 代码量减少 80%+
  - _Requirements: 6.1, 6.2, 6.3_
