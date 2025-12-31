# Design Document

## Overview

本设计文档描述了 LightUI 滚动条系统的统一化重构方案。目标是消除 body 元素与普通元素之间滚动条处理的差异，建立一个统一的滚动条检测、布局和绘制流程。

参考 Blink 引擎的 `ScrollableArea` 架构，我们将：
1. 统一所有元素的滚动条检测逻辑
2. 确保滚动条在元素内部显示（Chrome 行为）
3. 移除重复代码和调试语句
4. 提供完整的 JavaScript API

## Architecture

### 当前架构问题

```
┌─────────────────────────────────────────────────────────────┐
│                    ComputeLayoutInternal                     │
│  ┌─────────────────────────────────────────────────────┐    │
│  │  Body 特殊处理 (重复逻辑)                            │    │
│  │  - 第一次布局: scrollbar_width = 0                   │    │
│  │  - 检测内容高度                                      │    │
│  │  - 设置 scrollbar_width                             │    │
│  │  - 重新布局                                          │    │
│  └─────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                    ComputeNodeLayout                         │
│  ┌─────────────────────────────────────────────────────┐    │
│  │  普通元素处理 (跳过 root_node_)                      │    │
│  │  - 检测 overflow:auto                               │    │
│  │  - 设置 scrollbar_width                             │    │
│  │  - 重新布局                                          │    │
│  └─────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────┘
```

### 目标架构

```
┌─────────────────────────────────────────────────────────────┐
│                    ComputeLayoutInternal                     │
│  - 简化为仅处理 root margin 和位置                          │
│  - 不再包含滚动条检测逻辑                                    │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                    ComputeNodeLayout                         │
│  ┌─────────────────────────────────────────────────────┐    │
│  │  统一滚动条检测 (包括 root_node_)                    │    │
│  │  - 检测 overflow:auto/scroll                        │    │
│  │  - 使用 available_space.height 判断容器高度          │    │
│  │  - 设置 scrollbar_width                             │    │
│  │  - 重新布局                                          │    │
│  └─────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                    ComputeBlockLayout                        │
│  ┌─────────────────────────────────────────────────────┐    │
│  │  ComputeScrollbarGutter                              │    │
│  │  - 根据 scrollbar_width 计算 scrollbar_gutter       │    │
│  │  - 减少 content_box_inset                           │    │
│  └─────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────┘
```

## Components and Interfaces

### 1. NativeLayoutEngine (修改)

**文件**: `core/layout/native_layout_engine.cpp`

**修改内容**:
- `ComputeLayoutInternal`: 移除 body 特殊滚动条处理
- `ComputeNodeLayout`: 移除 `node_id != root_node_` 条件，统一处理所有元素

**关键接口**:
```cpp
// 统一的滚动条检测逻辑
void DetectAndApplyScrollbar(NodeId node_id, LayoutOutput& output, const LayoutInput& inputs);
```

### 2. BlockLayout (验证)

**文件**: `core/layout/block_layout.cpp`

**验证内容**:
- `ComputeScrollbarGutter`: 确保正确计算滚动条占用空间
- `ComputeBlockLayoutInner`: 确保 `content_box_inset` 正确应用

### 3. RenderObject (扩展)

**文件**: `core/render/objects/render_object.cpp`

**现有方法**:
- `GetMaxScrollX()`: 计算最大水平滚动范围
- `GetMaxScrollY()`: 计算最大垂直滚动范围
- `HitTestScrollbar()`: 滚动条点击检测

**新增方法**:
```cpp
float GetScrollWidth() const;   // 返回内容总宽度
float GetScrollHeight() const;  // 返回内容总高度
```

### 4. Element API (新增)

**文件**: `core/api/element_api.cpp` (扩展现有)

**新增 JS 绑定**:
```javascript
element.scrollWidth   // getter
element.scrollHeight  // getter
element.scrollTop     // getter/setter
element.scrollLeft    // getter/setter
```

## Data Models

### ScrollbarState

```cpp
struct ScrollbarState {
    bool needs_vertical;      // 是否需要垂直滚动条
    bool needs_horizontal;    // 是否需要水平滚动条
    float scrollbar_width;    // 滚动条宽度 (12px)
    float scroll_top;         // 当前垂直滚动偏移
    float scroll_left;        // 当前水平滚动偏移
    float max_scroll_x;       // 最大水平滚动范围
    float max_scroll_y;       // 最大垂直滚动范围
};
```

### LayoutNode.style 扩展

```cpp
struct Style {
    // ... 现有字段 ...
    float scrollbar_width;    // 滚动条宽度，用于 scrollbar_gutter 计算
};
```

## Correctness Properties

*A property is a characteristic or behavior that should hold true across all valid executions of a system-essentially, a formal statement about what the system should do. Properties serve as the bridge between human-readable specifications and machine-verifiable correctness guarantees.*

### Property 1: Vertical scrollbar triggers content width reduction
*For any* element with overflow:auto, if content_height > container_height, then the content layout width should be reduced by exactly scrollbar_width (12px).
**Validates: Requirements 1.1, 1.5**

### Property 2: Horizontal scrollbar appears only when needed
*For any* element with overflow:auto, horizontal scrollbar should appear only when content_width > (container_width - vertical_scrollbar_width_if_present).
**Validates: Requirements 1.2**

### Property 3: Body and div have identical scrollbar behavior
*For any* body element and div element with the same overflow setting and content, the scrollbar detection result should be identical.
**Validates: Requirements 1.4, 3.2**

### Property 4: Element outer dimensions unchanged by scrollbar
*For any* element, when a scrollbar appears, the element's outer width and height should remain unchanged.
**Validates: Requirements 2.1, 2.3**

### Property 5: scrollWidth/scrollHeight return content dimensions
*For any* element with overflow content, scrollWidth should equal the total content width, and scrollHeight should equal the total content height.
**Validates: Requirements 4.1, 4.2**

### Property 6: scrollTop/scrollLeft clamped to valid bounds
*For any* scroll position set via scrollTop or scrollLeft, the actual scroll position should be clamped to [0, maxScroll].
**Validates: Requirements 4.5, 4.6**

## Error Handling

1. **无效滚动位置**: 当设置的 scrollTop/scrollLeft 超出范围时，自动 clamp 到有效范围
2. **零尺寸容器**: 当容器尺寸为 0 时，不显示滚动条
3. **负内容尺寸**: 当内容尺寸计算为负数时，视为 0

## Testing Strategy

### 单元测试

1. **滚动条检测测试**
   - 测试 overflow:auto 在不同内容尺寸下的行为
   - 测试 overflow:scroll 始终显示滚动条
   - 测试 body 和 div 的一致性

2. **布局计算测试**
   - 测试 scrollbar_gutter 正确减少内容宽度
   - 测试元素外部尺寸不变

3. **JS API 测试**
   - 测试 scrollWidth/scrollHeight 返回正确值
   - 测试 scrollTop/scrollLeft getter/setter

### 属性测试

使用 JavaScript 测试框架进行属性测试：

```javascript
/**
 * Property Test: Vertical scrollbar triggers content width reduction
 * 
 * Feature: unified-scrollbar-system, Property 1: Vertical scrollbar triggers content width reduction
 * Validates: Requirements 1.1, 1.5
 */
function testVerticalScrollbarReducesWidth() {
    // 生成随机内容高度和容器高度
    // 验证当 content_height > container_height 时，内容宽度减少 12px
}
```

### 集成测试

1. 运行 `examples/component_demo/app.js` 验证滚动条显示正确
2. 验证无调试日志输出
3. 验证 textarea 滚动条行为不受影响
