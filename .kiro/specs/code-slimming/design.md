# Design Document: Code Slimming

## Overview

本设计文档描述了 LightUI 项目代码瘦身计划的技术方案。主要目标是清理重构过程中产生的冗余代码，包括：

1. **Hit Testing 系统**：删除旧的 `HitTesting` 类，保留新的 `HitTestController`
2. **Layer Tree 系统**：明确 `LayerTreeBuilder` 和 `LayerTreeManager` 的职责边界
3. **头文件拆分**：将超大头文件拆分到 300 行以下
4. **调试文件清理**：将调试日志文件加入 .gitignore

## Architecture

### 当前架构问题

```
┌─────────────────────────────────────────────────────────────┐
│                    Hit Testing 系统                          │
├─────────────────────────────────────────────────────────────┤
│  hit_testing.h/cpp (旧)     hit_test_controller.h/cpp (新)  │
│  ├── HitTesting             ├── HitTestController           │
│  ├── HitTestResult          ├── HitTestResultEx             │
│  └── 基础实现                └── 完整实现 + DevTools         │
│                                                             │
│  问题：两个系统同时编译，API 不兼容                           │
└─────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│                    Layer Tree 系统                           │
├─────────────────────────────────────────────────────────────┤
│  LayerTreeBuilder           LayerTreeManager                │
│  ├── Build()                ├── RequestAddLayer()           │
│  ├── AddLayerForObject()    ├── ApplyPendingUpdates()       │
│  ├── tree_version_ ←────────┼── tree_version_ (重复!)       │
│  └── IncrementalBuild()     └── 滚动状态管理                 │
│                                                             │
│  问题：职责重复，版本管理重复                                 │
└─────────────────────────────────────────────────────────────┘
```

### 目标架构

```
┌─────────────────────────────────────────────────────────────┐
│                    Hit Testing 系统 (清理后)                 │
├─────────────────────────────────────────────────────────────┤
│  hit_test_controller.h/cpp (唯一实现)                        │
│  ├── HitTestController                                      │
│  ├── HitTestResultEx                                        │
│  ├── HitTestRequest                                         │
│  └── DevTools 调试支持                                       │
└─────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│                    Layer Tree 系统 (重构后)                  │
├─────────────────────────────────────────────────────────────┤
│  LayerTreeBuilder (纯构建)   LayerTreeManager (纯管理)       │
│  ├── Build()                ├── RequestAddLayer()           │
│  ├── CreateLayer()          ├── ApplyPendingUpdates()       │
│  └── 无状态                  ├── tree_version_ (唯一)        │
│                             └── 滚动状态管理                 │
│                                                             │
│  调用关系: Manager → Builder (单向依赖)                      │
└─────────────────────────────────────────────────────────────┘
```

## Components and Interfaces

### 1. Hit Testing 系统清理

#### 删除文件
- `core/event/input/hit_testing.h`
- `core/event/input/hit_testing.cpp`

#### 保留文件
- `core/event/input/hit_test_controller.h`
- `core/event/input/hit_test_controller.cpp`

#### API 迁移映射

| 旧 API (HitTesting) | 新 API (HitTestController) |
|---------------------|---------------------------|
| `HitTest(Document, x, y)` | `HitTest(RenderObject, x, y, request)` |
| `HitTestWithLayers(...)` | `HitTest(...)` (内置支持) |
| `HitTestResult` | `HitTestResultEx` |
| `IsPointInBounds(...)` | 内部实现，不暴露 |

#### 需要更新的调用点

```cpp
// 旧代码
HitTesting hit_testing;
auto result = hit_testing.HitTest(document, x, y);

// 新代码
HitTestController controller;
auto result = controller.HitTest(root_render, x, y);
```

### 2. Layer Tree 系统重构

#### LayerTreeBuilder 职责（纯构建）

```cpp
class LayerTreeBuilder {
public:
    // 构建接口
    std::shared_ptr<CompositorLayer> Build(RenderObject* root);
    std::shared_ptr<CompositorLayer> CreateLayer(RenderObject* obj, LayerPromotionReason reason);
    
    // 查询接口
    LayerPromotionReason ShouldPromote(RenderObject* obj) const;
    std::shared_ptr<CompositorLayer> GetLayerForRenderObject(RenderObject* obj) const;
    
    // 配置接口
    void SetLayerPromotionEnabled(bool enabled);
    void SetDpiScale(float scale);
    
    // 删除: tree_version_ 相关方法
    // 删除: IncrementTreeVersion()
    // 删除: GetTreeVersion()
};
```

#### LayerTreeManager 职责（纯管理）

```cpp
class LayerTreeManager {
public:
    // 增量更新接口
    void RequestAddLayer(RenderObject* obj, LayerPromotionReason reason);
    void RequestRemoveLayer(RenderObject* obj);
    bool ApplyPendingUpdates();
    
    // 版本管理（唯一）
    uint64_t GetTreeVersion() const;
    void IncrementTreeVersion();
    
    // 滚动状态管理
    bool RegisterScrollContainer(RenderObject* container);
    const ScrollState* GetScrollState(RenderObject* container) const;
    
    // 坐标转换
    SkPoint ConvertPoint(...) const;
};
```

### 3. 头文件拆分方案

#### layer_tree_manager.h 拆分

当前：~600 行（超过 300 行限制）

拆分为：
1. `layer_tree_manager.h` (~200 行) - 主类定义
2. `layer_tree_types.h` (~100 行) - 类型定义（ScrollState, PendingLayerUpdate 等）
3. `scroll_state_manager.h` (~150 行) - 滚动状态管理（可选，如果需要进一步拆分）

```cpp
// layer_tree_types.h
#pragma once

namespace lightui {

enum class LayerUpdateType { Add, Remove, UpdateBounds, Reparent, UpdateZIndex };

struct PendingLayerUpdate {
    LayerUpdateType type;
    RenderObject* target = nullptr;
    LayerPromotionReason reason = LayerPromotionReason::None;
    int z_index = 0;
    std::string debug_info;
};

struct ScrollState {
    float scroll_x = 0.0f;
    float scroll_y = 0.0f;
    float max_scroll_x = 0.0f;
    float max_scroll_y = 0.0f;
    // ...
};

enum class CoordinateSpace { Document, Viewport, Layer };

}  // namespace lightui
```

```cpp
// layer_tree_manager.h
#pragma once

#include "layer_tree_types.h"
#include "compositor_layer.h"

namespace lightui {

class LayerTreeManager {
    // 主类定义，引用 layer_tree_types.h 中的类型
};

}  // namespace lightui
```

#### layer_tree_builder.h 优化

当前：~350 行（接近 300 行限制）

优化方案：
- 移除 `tree_version_` 相关代码（约 20 行）
- 简化注释（约 30 行）
- 目标：~300 行

## Data Models

### HitTestResultEx（保留）

```cpp
struct HitTestResultEx {
    std::shared_ptr<Element> element;
    std::shared_ptr<RenderObject> render_object;
    float local_x = 0;
    float local_y = 0;
    float viewport_x = 0;
    float viewport_y = 0;
    int z_index = 0;
    bool in_stacking_context = false;
    std::string miss_reason;
    
    bool IsValid() const { return element != nullptr; }
};
```

### ScrollState（移动到 layer_tree_types.h）

```cpp
struct ScrollState {
    float scroll_x = 0.0f;
    float scroll_y = 0.0f;
    float max_scroll_x = 0.0f;
    float max_scroll_y = 0.0f;
    float content_width = 0.0f;
    float content_height = 0.0f;
    float viewport_width = 0.0f;
    float viewport_height = 0.0f;
    uint64_t version = 0;
};
```

## Correctness Properties

*A property is a characteristic or behavior that should hold true across all valid executions of a system—essentially, a formal statement about what the system should do. Properties serve as the bridge between human-readable specifications and machine-verifiable correctness guarantees.*

由于本任务是代码清理/重构任务，主要验证点是构建成功和功能不退化，而非通用属性。以下是关键验证点：

### Property 1: Build Success After Cleanup

*For any* valid source code state after cleanup, the build system shall compile successfully without errors.

**Validates: Requirements 1.3, 3.2, 3.5, 6.2**

### Property 2: No Deprecated API References

*For any* source file in the codebase after cleanup, there shall be no references to the deprecated `HitTesting` class.

**Validates: Requirements 1.2, 1.4, 1.5**

### Property 3: Header File Size Compliance

*For any* header file in the refactored modules, the line count shall not exceed 300 lines.

**Validates: Requirements 3.1, 3.3**

### Property 4: Single Source of Truth for Version

*For any* layer tree operation, the `tree_version_` shall only be managed by `LayerTreeManager`, not `LayerTreeBuilder`.

**Validates: Requirements 2.2, 2.4**

## Error Handling

### 构建错误处理

1. **缺失依赖**：如果删除旧文件后有编译错误，需要更新调用点
2. **API 不兼容**：提供迁移指南，确保所有调用点使用新 API
3. **头文件循环依赖**：拆分时注意前向声明，避免循环 include

### 回滚策略

如果清理导致严重问题：
1. 使用 `git revert` 回滚提交
2. 分析问题原因
3. 制定更细粒度的清理计划

## Testing Strategy

### 验证方法

由于这是重构任务，主要通过以下方式验证：

1. **编译验证**
   ```cmd
   cmake --build build --config Release --target esm_loader
   ```

2. **现有测试验证**
   ```cmd
   build\bin\Release\esm_loader.exe tests\js\test_hit_testing_layers.js -q 5
   build\bin\Release\esm_loader.exe tests\js\test_incremental_layer_tree.js -q 5
   ```

3. **文件大小验证**
   - 检查 `layer_tree_manager.h` 行数 ≤ 300
   - 检查 `layer_tree_builder.h` 行数 ≤ 300

4. **API 引用验证**
   ```cmd
   findstr /s /i "class HitTesting" core\*.h core\*.cpp
   ```
   预期结果：无匹配

5. **二进制大小验证**
   - 记录清理前 `esm_loader.exe` 大小
   - 记录清理后大小
   - 验证大小减少或持平

### 测试优先级

1. **必须通过**：编译成功
2. **必须通过**：现有 Hit Testing 测试
3. **必须通过**：现有 Layer Tree 测试
4. **应该通过**：二进制大小减少
