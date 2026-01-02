# Design Document: Incremental Layout Boundary Optimization

## Overview

本设计文档描述了增量布局边界优化系统的实现方案。核心思想是识别"布局边界"（Layout Boundary），将布局变化限制在边界内部，避免触发整棵渲染树的重建。

### 问题背景

当前实现中，DOM 元素的增删操作会触发 `InvalidateRenderTree()`，导致整棵渲染树重建。这对于以下场景造成了不必要的性能开销：

1. **Modal/Toast/Dropdown** - `position: fixed/absolute` 元素的增删
2. **虚拟列表** - 滚动容器内子元素的频繁增删
3. **表格动态加载** - 固定尺寸容器内的内容变化
4. **CSS Containment** - 明确声明了布局隔离的元素

### 解决方案

引入"布局边界"概念，当 DOM 变化发生时：
1. 向上查找最近的布局边界祖先
2. 只标记该边界需要重新布局，而不是整棵树
3. 对于脱离文档流的元素，直接创建/销毁 RenderObject，不影响其他元素

## Architecture

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                              DOM 变化检测                                    │
│                         (WindowDOMObserver)                                 │
└─────────────────────────────────────────────────────────────────────────────┘
                                      │
                                      ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                         布局边界检测器                                       │
│                    (LayoutBoundaryDetector)                                 │
│                                                                              │
│  检测条件：                                                                  │
│  1. position: fixed/absolute (脱离文档流)                                   │
│  2. overflow: scroll/auto + 固定尺寸 (滚动容器)                             │
│  3. width + height 都是固定值 (固定尺寸容器)                                │
│  4. contain: layout/strict/content (CSS Containment)                        │
│  5. flex: 0 0 <size> (Flex 固定项)                                          │
└─────────────────────────────────────────────────────────────────────────────┘
                                      │
                    ┌─────────────────┴─────────────────┐
                    │                                   │
                    ▼                                   ▼
┌───────────────────────────────┐   ┌───────────────────────────────────────┐
│      脱离文档流元素            │   │         布局边界内元素                 │
│  (Out-of-Flow Elements)       │   │    (Layout Boundary Children)         │
│                               │   │                                       │
│  - 直接创建/销毁 RenderObject │   │  - 只标记边界需要重新布局              │
│  - 不触发 InvalidateRenderTree│   │  - 不影响边界外的元素                  │
│  - 只标记脏区域重绘           │   │  - 更新滚动尺寸（如适用）              │
└───────────────────────────────┘   └───────────────────────────────────────┘
```

## Components and Interfaces

### 1. LayoutBoundaryDetector（布局边界检测器）

```cpp
/**
 * @brief 布局边界检测器
 * 
 * 检测元素是否为布局边界，以及查找最近的布局边界祖先。
 */
class LayoutBoundaryDetector {
public:
    /**
     * @brief 布局边界类型
     */
    enum class BoundaryType {
        None,           ///< 不是布局边界
        OutOfFlow,      ///< 脱离文档流 (fixed/absolute)
        ScrollContainer,///< 滚动容器
        FixedSize,      ///< 固定尺寸容器
        CSSContainment, ///< CSS Containment
        FlexFixed,      ///< Flex 固定项
    };
    
    /**
     * @brief 检测元素是否为布局边界
     * @param element 要检测的元素
     * @return 布局边界类型
     */
    static BoundaryType DetectBoundaryType(Element* element);
    
    /**
     * @brief 检测 RenderObject 是否为布局边界
     * @param render_object 要检测的渲染对象
     * @return 布局边界类型
     */
    static BoundaryType DetectBoundaryType(RenderObject* render_object);
    
    /**
     * @brief 检查元素是否脱离文档流
     */
    static bool IsOutOfFlow(const ComputedStyle& style);
    
    /**
     * @brief 检查元素是否为滚动容器
     */
    static bool IsScrollContainer(const ComputedStyle& style);
    
    /**
     * @brief 检查元素是否有固定尺寸
     */
    static bool HasFixedSize(const ComputedStyle& style);
    
    /**
     * @brief 检查元素是否有 CSS Containment
     */
    static bool HasLayoutContainment(const ComputedStyle& style);
    
    /**
     * @brief 检查元素是否为 Flex 固定项
     */
    static bool IsFlexFixedItem(RenderObject* render_object);
    
    /**
     * @brief 查找最近的布局边界祖先
     * @param node 起始节点
     * @return 最近的布局边界祖先，如果没有返回 nullptr
     */
    static Element* FindNearestLayoutBoundary(Node* node);
    
    /**
     * @brief 查找最近的布局边界 RenderObject
     * @param render_object 起始渲染对象
     * @return 最近的布局边界 RenderObject，如果没有返回 nullptr
     */
    static RenderObject* FindNearestLayoutBoundary(RenderObject* render_object);
};
```

### 2. RenderObject 扩展

```cpp
class RenderObject {
public:
    // ... 现有代码 ...
    
    /**
     * @brief 检查是否为布局边界
     * 
     * 布局边界内的变化不会影响外部布局。
     * 参考 Blink ObjectIsRelayoutBoundary
     */
    bool IsLayoutBoundary() const;
    
    /**
     * @brief 获取布局边界类型
     */
    LayoutBoundaryDetector::BoundaryType GetLayoutBoundaryType() const;
    
    /**
     * @brief 缓存布局边界状态
     * 
     * 在样式变化时更新，避免每次都重新计算
     */
    void UpdateLayoutBoundaryCache();
    
private:
    /// 缓存的布局边界类型
    LayoutBoundaryDetector::BoundaryType cached_boundary_type_ = 
        LayoutBoundaryDetector::BoundaryType::None;
    
    /// 布局边界缓存是否有效
    bool boundary_cache_valid_ = false;
};
```

### 3. WindowDOMObserver 修改

```cpp
class WindowDOMObserver : public DOMObserver {
public:
    // ... 现有代码 ...
    
    void OnNodeAdded(Node* node, Node* parent) override {
        // 检查是否为脱离文档流的元素
        if (auto element = dynamic_cast<Element*>(node)) {
            if (LayoutBoundaryDetector::IsOutOfFlow(element->GetComputedStyle())) {
                // 脱离文档流：直接创建 RenderObject，不触发全量重建
                HandleOutOfFlowElementAdded(element, parent);
                return;
            }
        }
        
        // 查找最近的布局边界
        Element* boundary = LayoutBoundaryDetector::FindNearestLayoutBoundary(parent);
        
        if (boundary) {
            // 有布局边界：只标记边界需要重新布局
            HandleBoundedElementAdded(node, parent, boundary);
        } else {
            // 无布局边界：回退到全量重建
            window_->InvalidateRenderTree();
        }
        
        window_->SetNeedsRepaint();
    }
    
    void OnNodeRemoved(Node* node, Node* parent) override {
        // 类似的逻辑...
    }
    
private:
    /**
     * @brief 处理脱离文档流元素的添加
     */
    void HandleOutOfFlowElementAdded(Element* element, Node* parent);
    
    /**
     * @brief 处理脱离文档流元素的移除
     */
    void HandleOutOfFlowElementRemoved(Element* element, Node* parent);
    
    /**
     * @brief 处理布局边界内元素的添加
     */
    void HandleBoundedElementAdded(Node* node, Node* parent, Element* boundary);
    
    /**
     * @brief 处理布局边界内元素的移除
     */
    void HandleBoundedElementRemoved(Node* node, Node* parent, Element* boundary);
};
```

### 4. IncrementalLayoutManager（增量布局管理器）

```cpp
/**
 * @brief 增量布局管理器
 * 
 * 管理增量布局更新，避免全量重建。
 */
class IncrementalLayoutManager {
public:
    explicit IncrementalLayoutManager(Window* window);
    
    /**
     * @brief 处理脱离文档流元素的添加
     * 
     * 直接创建 RenderObject 并添加到渲染树，
     * 不触发 InvalidateRenderTree。
     */
    void AddOutOfFlowElement(Element* element, Node* parent);
    
    /**
     * @brief 处理脱离文档流元素的移除
     */
    void RemoveOutOfFlowElement(Element* element);
    
    /**
     * @brief 标记布局边界需要重新布局
     * 
     * 只标记边界及其子树需要重新布局，
     * 不影响边界外的元素。
     */
    void MarkBoundaryNeedsLayout(Element* boundary);
    
    /**
     * @brief 更新滚动容器的滚动尺寸
     */
    void UpdateScrollContainerSize(Element* scroll_container);
    
    /**
     * @brief 执行增量布局
     * 
     * 只重新计算标记为脏的子树。
     */
    void PerformIncrementalLayout();
    
private:
    Window* window_;
    
    /// 需要重新布局的边界列表
    std::unordered_set<Element*> dirty_boundaries_;
    
    /// 待添加的脱离文档流元素
    std::vector<std::pair<Element*, Node*>> pending_out_of_flow_additions_;
    
    /// 待移除的脱离文档流元素
    std::vector<Element*> pending_out_of_flow_removals_;
};
```

### 5. ComputedStyle 扩展

```cpp
struct ComputedStyle {
    // ... 现有字段 ...
    
    /**
     * @brief CSS contain 属性值
     * 
     * 可能的值：
     * - "none" (默认)
     * - "layout"
     * - "paint"
     * - "size"
     * - "style"
     * - "content" (= layout + paint + style)
     * - "strict" (= layout + paint + size + style)
     */
    std::string contain = "none";
    
    /**
     * @brief 检查是否有布局包含
     */
    bool HasLayoutContainment() const {
        return contain == "layout" || 
               contain == "content" || 
               contain == "strict" ||
               contain.find("layout") != std::string::npos;
    }
    
    /**
     * @brief 检查是否有尺寸包含
     */
    bool HasSizeContainment() const {
        return contain == "size" || 
               contain == "strict" ||
               contain.find("size") != std::string::npos;
    }
};
```

## Data Models

### 布局边界判断逻辑

```cpp
bool LayoutBoundaryDetector::IsLayoutBoundary(RenderObject* ro) {
    if (!ro) return false;
    
    const auto& style = ro->GetComputedStyle();
    
    // 1. 脱离文档流 - 最强的布局边界
    if (IsOutOfFlow(style)) {
        return true;
    }
    
    // 2. CSS Containment
    if (HasLayoutContainment(style)) {
        return true;
    }
    
    // 3. 滚动容器 + 固定尺寸
    if (IsScrollContainer(style) && HasFixedSize(style)) {
        return true;
    }
    
    // 4. 固定尺寸容器（非 auto）
    if (HasFixedSize(style) && !style.HasAutoSize()) {
        return true;
    }
    
    // 5. Flex 固定项
    if (IsFlexFixedItem(ro)) {
        return true;
    }
    
    return false;
}

bool LayoutBoundaryDetector::IsOutOfFlow(const ComputedStyle& style) {
    return style.position == "fixed" || style.position == "absolute";
}

bool LayoutBoundaryDetector::IsScrollContainer(const ComputedStyle& style) {
    return style.overflow_x == "scroll" || style.overflow_x == "auto" ||
           style.overflow_y == "scroll" || style.overflow_y == "auto";
}

bool LayoutBoundaryDetector::HasFixedSize(const ComputedStyle& style) {
    // 检查 width 和 height 是否都是固定值（px, vw, vh 等）
    bool width_fixed = style.width.has_value() && 
                       (style.width->unit == CSSUnit::PX ||
                        style.width->unit == CSSUnit::VW ||
                        style.width->unit == CSSUnit::VH);
    
    bool height_fixed = style.height.has_value() && 
                        (style.height->unit == CSSUnit::PX ||
                         style.height->unit == CSSUnit::VW ||
                         style.height->unit == CSSUnit::VH);
    
    return width_fixed && height_fixed;
}
```

## Correctness Properties

*A property is a characteristic or behavior that should hold true across all valid executions of a system-essentially, a formal statement about what the system should do. Properties serve as the bridge between human-readable specifications and machine-verifiable correctness guarantees.*

### Property 1: Out-of-Flow Element Isolation

*For any* `position: fixed` or `position: absolute` element, when it is added to or removed from the DOM, the system SHALL NOT call `InvalidateRenderTree()` and SHALL NOT trigger layout recalculation for any element outside its subtree.

**Validates: Requirements 1.1, 1.2, 1.3**

### Property 2: Scroll Container Boundary

*For any* scroll container (element with `overflow: scroll/auto` and fixed dimensions), when a child element is added or removed, the system SHALL only mark the scroll container for layout recalculation, and sibling elements of the scroll container SHALL NOT be marked for layout.

**Validates: Requirements 2.1, 2.2, 2.4**

### Property 3: Scroll Dimensions Update

*For any* scroll container, when children are added or removed, the scroll container's `scrollHeight` and `scrollWidth` SHALL be updated to reflect the new content size.

**Validates: Requirements 2.3**

### Property 4: Fixed-Size Container Boundary

*For any* container with explicit fixed `width` and `height` values (in px, vw, vh units), when a child is added or removed, ancestor elements SHALL NOT be marked for layout recalculation.

**Validates: Requirements 3.1, 3.2, 3.3**

### Property 5: CSS Containment Boundary

*For any* element with `contain: layout`, `contain: strict`, or `contain: content`, when a child is added or removed, ancestor elements SHALL NOT be marked for layout recalculation.

**Validates: Requirements 4.1, 4.2, 4.3**

### Property 6: Layout Boundary Detection

*For any* RenderObject, the `IsLayoutBoundary()` method SHALL return `true` if and only if the element satisfies at least one of the layout boundary conditions (out-of-flow, scroll container with fixed size, fixed-size container, CSS containment, or flex fixed item).

**Validates: Requirements 5.1**

### Property 7: Nearest Boundary Traversal

*For any* DOM change, the system SHALL traverse up the ancestor chain and find the nearest layout boundary, and layout invalidation SHALL stop at that boundary.

**Validates: Requirements 5.2, 5.3**

### Property 8: Fallback to Full Rebuild

*For any* DOM change where no layout boundary ancestor is found, the system SHALL fall back to calling `InvalidateRenderTree()` for a full render tree rebuild.

**Validates: Requirements 5.4**

### Property 9: Nested Boundary Resolution

*For any* nested layout boundaries, the system SHALL use the innermost applicable boundary for layout invalidation.

**Validates: Requirements 6.3**

### Property 10: Auto-Size Exclusion

*For any* container with `height: auto` or `width: auto`, the container SHALL NOT be treated as a layout boundary, even if it has other boundary-qualifying properties.

**Validates: Requirements 6.2**

## Error Handling

### 1. 边界检测失败

当无法确定元素是否为布局边界时：
- 回退到全量重建（保守策略）
- 记录警告日志用于调试

### 2. 增量更新失败

当增量更新过程中发生错误时：
- 回退到全量重建
- 清除所有脏标记
- 记录错误日志

### 3. 样式解析失败

当 `contain` 属性解析失败时：
- 使用默认值 "none"
- 不将元素视为布局边界

## Testing Strategy

### 单元测试

使用 Google Test 框架进行单元测试。

1. **LayoutBoundaryDetector 测试**
   - 测试各种样式组合的边界检测
   - 测试祖先查找逻辑

2. **IncrementalLayoutManager 测试**
   - 测试脱离文档流元素的增删
   - 测试布局边界内元素的增删

3. **ComputedStyle 测试**
   - 测试 `contain` 属性解析
   - 测试 `HasLayoutContainment()` 方法

### 属性测试

使用 rapidcheck 进行属性测试，每个属性测试运行 100 次迭代。

**属性测试注释格式**：
```cpp
/**
 * Property Test: Out-of-Flow Element Isolation
 * 
 * Feature: incremental-layout-boundary, Property 1: Out-of-Flow Element Isolation
 * Validates: Requirements 1.1, 1.2, 1.3
 */
RC_GTEST_PROP(LayoutBoundary, OutOfFlowIsolation, ()) {
    // 测试实现
}
```

### 集成测试

1. **Modal 测试** (`test_modal_incremental.js`)
   - 打开/关闭 Modal 不触发全量重建
   - 验证其他元素布局不受影响

2. **虚拟列表测试** (`test_virtual_list_incremental.js`)
   - 滚动时增删元素不触发全量重建
   - 验证滚动性能

3. **Toast 测试** (`test_toast_incremental.js`)
   - 显示/隐藏 Toast 不触发全量重建
   - 验证动画流畅性

### 性能测试

1. **基准测试**
   - 测量优化前后的布局时间
   - 测量 Modal 打开/关闭的帧率

2. **压力测试**
   - 大量元素的虚拟列表滚动
   - 频繁的 Toast 显示/隐藏
