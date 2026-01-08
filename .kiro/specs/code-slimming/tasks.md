# Implementation Plan: Code Slimming

## Overview

本计划按优先级清理代码冗余，从高优先级的 Hit Testing 系统开始，逐步处理 Layer Tree 系统和头文件拆分。每个阶段完成后验证编译和测试。

## Tasks

- [x] 1. 记录基线数据
  - 记录当前 esm_loader.exe 文件大小
  - 记录当前编译时间
  - _Requirements: 6.1_

- [x] 2. 清理调试文件
  - [x] 2.1 更新 .gitignore 添加调试文件
    - 添加 `debuglog.txt` 到 .gitignore
    - 添加 `build_log.txt` 到 .gitignore
    - _Requirements: 4.1, 4.2_
  - [x] 2.2 从 git 跟踪中移除调试文件
    - 执行 `git rm --cached debuglog.txt build_log.txt` (如果存在)
    - _Requirements: 4.3, 4.4_

- [x] 3. 清理 Hit Testing 系统冗余
  - [x] 3.1 更新 core/event/dispatch/mouse_event_dispatcher.cpp
    - 将 `#include "hit_testing.h"` 改为 `#include "hit_test_controller.h"`
    - 将 `HitTesting` 类使用改为 `HitTestController`
    - 将 `HitTestResult` 改为 `HitTestResultEx`
    - _Requirements: 1.4_
  - [x] 3.2 更新 core/event/dispatch/wheel_event_dispatcher.cpp
    - 将 `#include "hit_testing.h"` 改为 `#include "hit_test_controller.h"`
    - 将 `HitTesting` 类使用改为 `HitTestController`
    - _Requirements: 1.4_
  - [x] 3.3 更新 core/event/loop/event_loop.cpp
    - 将 `#include "../input/hit_testing.h"` 改为 `#include "../input/hit_test_controller.h"`
    - 将 `HitTesting` 类使用改为 `HitTestController`
    - _Requirements: 1.4_
  - [x] 3.4 更新 core/quickjs/document_bindings_impl.cpp
    - 将 `#include "hit_testing.h"` 改为 `#include "hit_test_controller.h"`
    - 将 `HitTesting` 类使用改为 `HitTestController`
    - _Requirements: 1.4_
  - [x] 3.5 更新 core/editing/drag_manager.cpp
    - 将 `#include "hit_testing.h"` 改为 `#include "hit_test_controller.h"`
    - 将 `HitTesting` 类使用改为 `HitTestController`
    - _Requirements: 1.4_
  - [x] 3.6 更新 core/render/layer/paint_layer.cpp
    - 将 `#include "hit_testing.h"` 改为 `#include "hit_test_controller.h"`
    - 将 `HitTesting` 类使用改为 `HitTestController`
    - _Requirements: 1.4_
  - [x] 3.7 删除旧的 Hit Testing 文件
    - 删除 `core/event/input/hit_testing.h`
    - 删除 `core/event/input/hit_testing.cpp`
    - _Requirements: 1.2_
  - [x] 3.8 更新 core/event/CMakeLists.txt
    - 移除 `input/hit_testing.cpp` 编译项
    - _Requirements: 1.1_
  - [x] 3.9 更新测试文件
    - 更新 `tests/property/render/test_pointer_events_properties.cpp`
    - 更新 `tests/property/render/test_paint_layer_properties.cpp`
    - _Requirements: 1.4_

- [x] 4. Checkpoint - Hit Testing 清理验证
  - 编译验证: `cmake --build build --config Release --target esm_loader`
  - 运行测试验证功能不退化
  - 确认无 `class HitTesting` 引用
  - _Requirements: 1.3, 6.2_

- [x] 5. 清理 Layer Tree 系统职责重复
  - [x] 5.1 创建 layer_tree_types.h
    - 从 layer_tree_manager.h 提取类型定义
    - 包含 LayerUpdateType, PendingLayerUpdate, ScrollState, CoordinateSpace
    - _Requirements: 3.1_
  - [x] 5.2 重构 layer_tree_manager.h
    - 引用 layer_tree_types.h
    - 移除内联类型定义
    - 确保行数 ≤ 300
    - _Requirements: 3.1, 3.2_
  - [x] 5.3 评估 LayerTreeBuilder 的 tree_version_
    - 分析后决定保留：LayerTreeBuilder 的版本号跟踪层结构变化，与 LayerTreeManager 的版本号用途不同
    - LayerTreeBuilder 内部使用 IncrementTreeVersion() 跟踪 Add/Remove/IncrementalBuild 操作
    - _Requirements: 2.2, 2.4_
  - [x] 5.4 优化 layer_tree_builder.h
    - 当前 273 行，符合 ≤300 行规范
    - _Requirements: 3.3_
  - [x] 5.5 更新 CMakeLists.txt
    - layer_tree_types.h 是纯头文件，无需修改 CMakeLists.txt
    - _Requirements: 3.4_

- [x] 6. Checkpoint - Layer Tree 重构验证
  - 编译验证: 通过
  - 二进制大小: 27,963,904 字节（与 Hit Testing 清理后一致）
  - 头文件行数: layer_tree_builder.h 273 行，符合规范
  - _Requirements: 3.5, 6.2_

- [x] 7. 更新文档
  - [x] 7.1 更新 core/event/input/README.md
    - 移除 HitTesting 类的描述
    - 添加 HitTestController 的使用说明
    - _Requirements: 5.1_
  - [x] 7.2 更新 core/compositor/README.md
    - 添加 layer_tree_types.h 的描述
    - 更新 LayerTreeBuilder 和 LayerTreeManager 的职责描述
    - _Requirements: 5.2_
  - [ ] 7.3 清理分析文档
    - 删除或归档 CODE_REDUNDANCY_ANALYSIS.md
    - _Requirements: 5.3_

- [ ] 8. 最终验证
  - [ ] 8.1 完整编译验证
    - 执行完整编译
    - 确认无警告
    - _Requirements: 6.2_
  - [ ] 8.2 测试验证
    - 运行所有相关测试
    - 确认无回归
    - _Requirements: 6.3_
  - [ ] 8.3 记录瘦身结果
    - 记录清理后 esm_loader.exe 文件大小
    - 计算大小变化
    - 记录编译时间变化
    - _Requirements: 6.1, 6.4_

## Notes

- 每个 Checkpoint 必须通过才能继续下一阶段
- 如果编译失败，立即修复后再继续
- 保持提交粒度小，便于回滚
- 优先保证功能不退化，其次追求代码精简
