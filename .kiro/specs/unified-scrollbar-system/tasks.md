# Implementation Plan

- [x] 1. 统一滚动条检测逻辑
  - [x] 1.1 修改 ComputeLayoutInternal 移除 body 特殊处理
    - 移除 body 滚动条检测的特殊代码块
    - 保留 root margin 和位置处理
    - 移除调试 printf/fflush 语句
    - _Requirements: 1.4, 3.2, 5.1, 5.2_
  - [x] 1.2 修改 ComputeNodeLayout 支持 root 节点
    - 移除 `node_id != root_node_` 条件
    - 使用 `available_space.height` 作为容器高度判断
    - 确保 scrollbar_width 正确传递给 ComputeBlockLayout
    - _Requirements: 1.1, 1.4, 3.1_
  - [ ]* 1.3 编写属性测试：Body 和 div 滚动条行为一致
    - **Property 3: Body and div have identical scrollbar behavior**
    - **Validates: Requirements 1.4, 3.2**

- [x] 2. 验证 block_layout 滚动条计算
  - [x] 2.1 验证 ComputeScrollbarGutter 逻辑正确性
    - 确认 scrollbar_width > 0 时正确计算 gutter
    - 确认 gutter 正确应用到 content_box_inset
    - _Requirements: 2.2, 3.3_
  - [ ]* 2.2 编写属性测试：垂直滚动条触发内容宽度减少
    - **Property 1: Vertical scrollbar triggers content width reduction**
    - **Validates: Requirements 1.1, 1.5**
  - [ ]* 2.3 编写属性测试：水平滚动条仅在需要时出现
    - **Property 2: Horizontal scrollbar appears only when needed**
    - **Validates: Requirements 1.2**

- [x] 3. Checkpoint - 确保所有测试通过
  - Ensure all tests pass, ask the user if questions arise.

- [x] 4. 添加 JavaScript API 绑定
  - [x] 4.1 实现 scrollWidth/scrollHeight getter
    - 在 RenderObject 中添加 GetScrollWidth/GetScrollHeight 方法
    - 在 Element API 中绑定到 JS
    - _Requirements: 4.1, 4.2_
  - [x] 4.2 实现 scrollTop/scrollLeft getter/setter
    - 确保 setter 将值 clamp 到有效范围
    - 在 Element API 中绑定到 JS
    - _Requirements: 4.3, 4.4, 4.5, 4.6_
  - [ ]* 4.3 编写属性测试：scrollWidth/scrollHeight 返回内容尺寸
    - **Property 5: scrollWidth/scrollHeight return content dimensions**
    - **Validates: Requirements 4.1, 4.2**
  - [ ]* 4.4 编写属性测试：scrollTop/scrollLeft 被 clamp 到有效范围
    - **Property 6: scrollTop/scrollLeft clamped to valid bounds**
    - **Validates: Requirements 4.5, 4.6**

- [x] 5. 验证 textarea 滚动条不受影响
  - [x] 5.1 验证 textarea 滚动条行为保持不变
    - 运行现有 textarea 测试
    - 确认 HTMLTextAreaElement 的滚动条逻辑未被修改
    - _Requirements: 6.1, 6.2_

- [x] 6. 集成测试
  - [x] 6.1 运行 component_demo 验证滚动条功能
    - 验证 body overflow:auto 垂直滚动条正常显示
    - 验证无水平滚动条（内容宽度 < 视口宽度 - 滚动条宽度）
    - 验证无调试日志输出
    - _Requirements: 1.1, 1.2, 2.1, 5.1_
  - [ ]* 6.2 编写属性测试：元素外部尺寸不因滚动条改变
    - **Property 4: Element outer dimensions unchanged by scrollbar**
    - **Validates: Requirements 2.1, 2.3**

- [x] 7. Final Checkpoint - 确保所有测试通过



  - Ensure all tests pass, ask the user if questions arise.
