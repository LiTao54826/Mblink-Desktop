# Design Document: Unified Layer System

## Overview

本设计文档描述了 LightUI 统一层系统的重构方案。采用**简化设计**，将现有的 `LayerManager` 和 `CompositorLayer` 两个独立系统统一为一个紧凑高效的架构。

**设计原则：简洁、高效、易维护**

- **单一 PaintLayer 类**：整合 z-order 排序、compositing 判断、绘制、hit testing
- **复用 CompositorLayer**：保留现有的 GPU 纹理管理，按需关联到 PaintLayer
- **删除 LayerManager**：其功能由 PaintLayer 的 stacking context 机制替代
- **减少类数量**：不需要单独的 StackingNode、CompositingDecider、Painter 类

**为什么选择简化设计：**
1. LightUI 不是完整浏览器，不需要 Blink 那样复杂的架构
2. 类越少，维护成本越低，bug 越少
3. 减少间接调用，性能更好
4. 所有功能仍然完整实现

```
┌─────────────────────────────────────────────────────────────────┐
│                        Blink 架构（参考）                        │
├─────────────────────────────────────────────────────────────────┤
│  LayoutObject → PaintLayer → PaintLayerStackingNode             │
│                      ↓                                          │
│              CompositingLayerAssigner                           │
│                      ↓                                          │
│              cc::Layer (GPU Layer)                              │
└─────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────┐
│                     LightUI 简化架构                             │
├─────────────────────────────────────────────────────────────────┤
│  RenderObject → PaintLayer (z-order + compositing + paint)      │
│                      ↓                                          │
│              CompositorLayer (GPU，按需创建)                     │
└─────────────────────────────────────────────────────────────────┘
```

## Architecture

### 整体架构图

```
┌─────────────────────────────────────────────────────────────────────┐
│                          渲染管线                                    │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│  ┌─────────────┐         ┌─────────────────────────────────────┐   │
│  │ RenderObject│────────→│            PaintLayer               │   │
│  │   (现有)    │         │  ┌─────────────────────────────────┐│   │
│  └─────────────┘         │  │ • z-order 列表 (pos/neg)        ││   │
│                          │  │ • stacking context 判断         ││   │
│                          │  │ • compositing 判断              ││   │
│                          │  │ • Paint() 绘制                  ││   │
│                          │  │ • HitTest() 点击测试            ││   │
│                          │  └─────────────────────────────────┘│   │
│                          └──────────────┬──────────────────────┘   │
│                                         │ (按需关联)                │
│                                         ↓                          │
│                          ┌─────────────────────────────────────┐   │
│                          │         CompositorLayer             │   │
│                          │  • GPU 纹理管理                     │   │
│                          │  • 脏区域跟踪                       │   │
│                          │  • 动画边界                         │   │
│                          │  (复用现有实现)                     │   │
│                          └─────────────────────────────────────┘   │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

### 职责分离

| 组件 | 职责 | 来源 |
|------|------|------|
| PaintLayer | Stacking Context、z-index 排序、Compositing 判断、绘制、Hit Testing | 新建（整合 Layer + LayerManager 功能） |
| CompositorLayer | GPU 纹理、脏区域、动画边界 | 复用现有 |
| Compositor | GPU 合成 | 复用现有 |

## Components and Interfaces

### 1. PaintLayer（核心类，整合所有功能）

```cpp
// core/render/layer/paint_layer.h
// 统一的绘制层类，整合 z-order、compositing、paint、hit-test

class PaintLayer {
public:
    explicit PaintLayer(RenderObject* render_object);
    ~PaintLayer();

    // =========================================================================
    // 基本属性
    // =========================================================================
    
    RenderObject* GetRenderObject() const { return render_object_; }
    PaintLayer* Parent() const { return parent_; }
    const std::vector<PaintLayer*>& Children() const { return children_; }

    // 树操作
    void AddChild(PaintLayer* child);
    void RemoveChild(PaintLayer* child);
    void InsertBefore(PaintLayer* child, PaintLayer* before);

    // =========================================================================
    // Stacking Context（内嵌，不需要单独的 StackingNode）
    // =========================================================================
    
    /**
     * @brief 检查是否是 stacking context
     * 
     * 创建 stacking context 的条件（CSS 规范）：
     * - position: absolute/relative/fixed 且 z-index != auto
     * - opacity < 1
     * - transform != none
     * - filter != none
     * - will-change: transform/opacity
     */
    bool IsStackingContext() const;
    
    /**
     * @brief 获取 stacking context 祖先
     */
    PaintLayer* StackingContext() const;
    
    /**
     * @brief 获取 z-index
     */
    int ZIndex() const;

    // =========================================================================
    // Z-Order 列表（内嵌，不需要单独的 StackingNode 类）
    // =========================================================================
    
    /**
     * @brief 获取正 z-index 子层列表（z-index >= 0，按升序）
     */
    const std::vector<PaintLayer*>& PosZOrderList() const { return pos_z_order_list_; }
    
    /**
     * @brief 获取负 z-index 子层列表（z-index < 0，按升序）
     */
    const std::vector<PaintLayer*>& NegZOrderList() const { return neg_z_order_list_; }
    
    /**
     * @brief 更新 z-order 列表（只在 stacking context 上有效）
     */
    void UpdateZOrderLists();
    
    /**
     * @brief 标记 z-order 列表需要更新
     */
    void DirtyZOrderLists() { z_order_dirty_ = true; }
    
    /**
     * @brief 检查 z-order 列表是否需要更新
     */
    bool ZOrderListsDirty() const { return z_order_dirty_; }

    // =========================================================================
    // Compositing（内嵌，不需要单独的 CompositingDecider 类）
    // =========================================================================
    
    /**
     * @brief 检查是否需要独立的 CompositorLayer
     * 
     * 提升条件：
     * - will-change: transform/opacity
     * - position: fixed
     * - 活动的 transform/opacity 动画
     * - 可滚动容器
     */
    bool NeedsCompositing() const;
    
    /**
     * @brief 获取提升原因
     */
    LayerPromotionReason GetPromotionReason() const;
    
    /**
     * @brief 确保有 CompositorLayer（按需创建）
     */
    void EnsureCompositorLayer();
    
    /**
     * @brief 获取关联的 CompositorLayer
     */
    CompositorLayer* GetCompositedLayer() const { return compositor_layer_.get(); }
    
    /**
     * @brief 检查是否有独立的 CompositorLayer
     */
    bool HasCompositedLayer() const { return compositor_layer_ != nullptr; }

    // =========================================================================
    // 绘制（内嵌，不需要单独的 PaintLayerPainter 类）
    // =========================================================================
    
    /**
     * @brief 绘制层及其子层
     * 按 CSS stacking context 规则绘制：
     * 1. 背景和边框
     * 2. 负 z-index 子层
     * 3. 正常流内容
     * 4. 正 z-index 子层
     */
    void Paint(SkCanvas* canvas);
    
    /**
     * @brief 绘制层自身内容（不包括子层）
     */
    void PaintContents(SkCanvas* canvas);

    // =========================================================================
    // Hit Testing（复用现有 Layer::HitTest 逻辑）
    // =========================================================================
    
    /**
     * @brief Hit Testing（按 z-order 逆序测试）
     */
    bool HitTest(float x, float y, HitTestResult& result);

    // =========================================================================
    // 滚动支持
    // =========================================================================
    
    /**
     * @brief 处理滚轮事件
     */
    bool HandleWheel(float x, float y, float delta_x, float delta_y);

    // =========================================================================
    // 调试
    // =========================================================================
    
    std::string ToDebugString() const;
    void DumpTree(int indent = 0) const;

private:
    // 关联的 RenderObject
    RenderObject* render_object_;
    
    // 树结构（使用 vector，简单高效）
    PaintLayer* parent_ = nullptr;
    std::vector<PaintLayer*> children_;
    
    // Z-order 列表（内嵌，只有 stacking context 使用）
    std::vector<PaintLayer*> pos_z_order_list_;  // z-index >= 0
    std::vector<PaintLayer*> neg_z_order_list_;  // z-index < 0
    bool z_order_dirty_ = true;
    
    // Compositing（内嵌）
    std::shared_ptr<CompositorLayer> compositor_layer_;
    LayerPromotionReason promotion_reason_ = LayerPromotionReason::None;
    
    // 缓存
    mutable PaintLayer* cached_stacking_context_ = nullptr;
    mutable bool stacking_context_dirty_ = true;
    
    // 私有方法
    void CollectZOrderLayers();
    void SortZOrderLists();
    void PaintNegativeZOrderChildren(SkCanvas* canvas);
    void PaintPositiveZOrderChildren(SkCanvas* canvas);
    bool HitTestChildren(float x, float y, HitTestResult& result);
};
```

### 2. CompositorLayer（复用现有，不修改）

现有的 `CompositorLayer` 类完全复用，不做任何修改。它负责：
- GPU 纹理管理
- 脏区域跟踪
- 动画边界计算
- DPI 缩放支持

PaintLayer 通过 `compositor_layer_` 成员按需关联 CompositorLayer。

### 3. PaintLayer 树的构建和生命周期

```cpp
// 在 RenderObject 中添加
class RenderObject {
    // ...
    
    /**
     * @brief 获取或创建 PaintLayer
     * 只有需要 PaintLayer 的元素才会创建
     */
    PaintLayer* EnsurePaintLayer();
    
    /**
     * @brief 获取 PaintLayer（可能为 nullptr）
     */
    PaintLayer* GetPaintLayer() const { return paint_layer_.get(); }
    
    /**
     * @brief 检查是否需要 PaintLayer
     * 
     * 需要 PaintLayer 的条件：
     * - 是 stacking context（z-index + position、opacity < 1、transform 等）
     * - 需要 compositing（will-change、动画、fixed、滚动）
     * - 是根元素
     */
    bool NeedsPaintLayer() const;
    
private:
    std::unique_ptr<PaintLayer> paint_layer_;
};
```

**构建时机：**
1. 在 `RenderObject::UpdateLayoutStyle()` 或样式变化时检查是否需要 PaintLayer
2. 懒创建：只有真正需要时才创建
3. 在 `Window::Paint()` 开始时，从根 PaintLayer 开始绘制

**与现有 LayerManager 的对比：**

| 现有 LayerManager | 新 PaintLayer |
|------------------|---------------|
| Paint 时动态收集 | 样式变化时构建 |
| 每帧重建列表 | 增量更新 |
| 单例全局状态 | 树结构，无全局状态 |
| 按 z-index 阈值分层 | 按 CSS stacking context 分层 |

### 4. position:fixed 元素的处理

```cpp
// 在 PaintLayer 中
class PaintLayer {
    // ...
    
    /**
     * @brief 检查是否是 fixed 定位
     */
    bool IsFixedPositioned() const;
    
    /**
     * @brief 获取用于 hit testing 的绝对坐标
     * 
     * 对于 fixed 元素：直接使用 layout.x/y（视口坐标）
     * 对于其他元素：累加父元素偏移
     */
    void GetAbsolutePosition(float& abs_x, float& abs_y) const;
};
```

**关键逻辑（复用自 Layer::AddItem）：**
```cpp
if (is_fixed) {
    // position: fixed 元素：layout.x/y 已经是视口绝对坐标
    abs_x = layout.x;
    abs_y = layout.y;
} else {
    // 其他元素：累加父元素偏移
    abs_x = parent_abs_x + layout.x;
    abs_y = parent_abs_y + layout.y;
}
```

### 5. 与 ScrollLayerManager 的关系

`ScrollLayerManager` 是独立的滚动层管理器，负责：
- 滚动容器的注册和管理
- 滚动偏移的应用
- 滚动动画

**不受本次重构影响**，但需要确保：
- PaintLayer 的 HitTest 正确处理滚动偏移
- PaintLayer 的 HandleWheel 与 ScrollLayerManager 协调

## Data Models

### PaintLayer 树结构

```
PaintLayer (root, stacking context)
│
├── pos_z_order_list_: [layer_z1, layer_z2, ...]  // z-index >= 0，按升序
├── neg_z_order_list_: [layer_z-1, layer_z-2, ...] // z-index < 0，按升序
│
├── PaintLayer (child1, z-index: 2)
│   ├── compositor_layer_ → CompositorLayer (GPU)
│   └── children_: [...]
│
├── PaintLayer (child2, z-index: -1)
│   ├── compositor_layer_ → nullptr (软件渲染)
│   └── children_: [...]
│
└── PaintLayer (child3, stacking context, z-index: 1)
    ├── pos_z_order_list_: [...]
    ├── neg_z_order_list_: [...]
    └── children_: [...]
```

### 与现有数据结构的映射

| 现有结构 | 新结构 | 说明 |
|---------|--------|------|
| Layer::items_ | PaintLayer::pos/neg_z_order_list_ | z-order 列表 |
| LayerManager::layers_ | PaintLayer 树 | 层级管理 |
| LayerManager::ShouldCollect() | PaintLayer::NeedsCompositing() | compositing 判断 |
| LayerManager::HitTest() | PaintLayer::HitTest() | hit testing |
| LayerManager::PaintLayers() | PaintLayer::Paint() | 绘制 |
| CompositorLayer | CompositorLayer（不变） | GPU 层 |

### 代码复用映射

| 现有代码 | 复用到 | 说明 |
|---------|--------|------|
| Layer::EnsureSorted() | PaintLayer::SortZOrderLists() | 排序逻辑 |
| Layer::HitTest() | PaintLayer::HitTest() | hit testing 逻辑 |
| Layer::HitTestRenderObject() | PaintLayer::HitTestChildren() | 递归 hit test |
| Layer::Paint() | PaintLayer::Paint() | 绘制逻辑 |
| LayerManager::GetLayerLevel() | PaintLayer::NeedsCompositing() | 层级判断 |
| LayerTreeBuilder::ShouldPromote() | PaintLayer::GetPromotionReason() | 提升原因 |

## Correctness Properties

*A property is a characteristic or behavior that should hold true across all valid executions of a system-essentially, a formal statement about what the system should do. Properties serve as the bridge between human-readable specifications and machine-verifiable correctness guarantees.*

### Property 1: PaintLayer 树 round-trip 一致性

*For any* PaintLayer 树，序列化为字符串后再解析，应该产生结构相同、z-index 排序相同的树。

**Validates: Requirements 1.4, 1.5**

### Property 2: Stacking Context 创建正确性

*For any* RenderObject，当其 CSS 属性满足 stacking context 创建条件时，对应的 PaintLayer 的 `IsStackingContext()` 应返回 true。

**Validates: Requirements 1.1**

### Property 3: Z-order 列表维护正确性

*For any* stacking context 的 PaintLayer，其 `pos_z_order_list_` 应包含所有 z-index >= 0 的直接子层（按升序），`neg_z_order_list_` 应包含所有 z-index < 0 的直接子层（按升序）。

**Validates: Requirements 1.2**

### Property 4: 绘制顺序正确性

*For any* stacking context，绘制顺序应为：背景 → 负 z-index 子层 → 正常流 → 正 z-index 子层。

**Validates: Requirements 1.3**

### Property 5: CompositorLayer 不影响绘制顺序

*For any* PaintLayer 树，无论哪些层有 CompositorLayer，最终的绘制顺序应该与纯软件渲染时相同。

**Validates: Requirements 2.1, 2.2, 2.3**

### Property 6: Fixed 元素 stacking context 归属

*For any* position:fixed 元素，其 PaintLayer 应参与根 stacking context 的 z-order 排序。

**Validates: Requirements 3.1, 3.4**

### Property 7: Hit Testing 逆序遍历

*For any* 点击位置，hit testing 应按绘制顺序的逆序（从上到下视觉顺序）遍历 PaintLayer。

**Validates: Requirements 4.1, 4.2**

### Property 8: 滚动偏移坐标变换

*For any* 可滚动容器内的 hit testing，坐标应正确应用滚动偏移变换。

**Validates: Requirements 4.3**

### Property 9: 同 z-index 文档顺序

*For any* 两个 z-index 相同的重叠元素，hit testing 应返回文档顺序靠后的元素。

**Validates: Requirements 4.4**

### Property 10: Compositing 提升规则

*For any* 满足提升条件（will-change、动画、fixed、滚动）的 PaintLayer，应创建对应的 CompositorLayer。

**Validates: Requirements 6.1, 6.2, 6.3, 6.4**

### Property 11: 增量更新范围

*For any* 样式变化，只有受影响的 PaintLayer 及其祖先应被更新。

**Validates: Requirements 7.1, 7.2, 7.3**

### Property 12: 层检查信息完整性

*For any* PaintLayer，检查 API 应返回其 stacking context 状态、z-index、compositing 状态和关联的 RenderObject。

**Validates: Requirements 8.3**

## Error Handling

### 1. 空指针保护

```cpp
// PaintLayer 方法中的空指针检查
PaintLayer* PaintLayer::StackingContext() const {
    if (!parent_) return nullptr;  // 根层没有 stacking context
    // ...
}
```

### 2. 循环引用检测

```cpp
// 在 UpdateZOrderLists 中检测循环
void PaintLayerStackingNode::CollectLayers(PaintLayer* layer) {
    std::unordered_set<PaintLayer*> visited;
    // 检测循环引用
    if (visited.count(layer)) {
        LOG_ERROR("Circular reference detected in PaintLayer tree");
        return;
    }
    visited.insert(layer);
    // ...
}
```

### 3. 无效状态恢复

```cpp
// CompositorLayer 创建失败时的回退
void CompositingDecider::UpdateCompositingState(PaintLayer* layer) {
    if (ShouldComposite(layer)) {
        auto compositor_layer = CreateCompositorLayer();
        if (!compositor_layer) {
            // 回退到软件渲染
            LOG_WARNING("Failed to create CompositorLayer, falling back to software rendering");
            return;
        }
        layer->SetCompositedLayer(compositor_layer);
    }
}
```

## Testing Strategy

### 双重测试方法

本项目采用单元测试和属性测试相结合的方法：

1. **单元测试**: 验证具体示例和边界情况
2. **属性测试**: 验证应在所有输入上成立的通用属性

### 属性测试框架

使用 **RapidCheck** (C++ 属性测试库) 或自定义的简单属性测试框架。

### 测试文件结构

```
tests/
├── js/
│   ├── test_paint_layer.js           # PaintLayer 功能测试
│   ├── test_stacking_context.js      # Stacking context 测试
│   ├── test_z_order.js               # Z-order 排序测试
│   └── test_hit_testing_layers.js    # Hit testing 测试
└── cpp/
    └── paint_layer_property_test.cpp # 属性测试
```

### 属性测试示例

```cpp
// tests/cpp/paint_layer_property_test.cpp

/**
 * Property Test: Z-order 列表排序
 * 
 * Feature: unified-layer-system, Property 3: Z-order 列表维护正确性
 * Validates: Requirements 1.2
 */
void TestZOrderListSorting() {
    // 生成随机 z-index 值
    std::vector<int> z_indices = GenerateRandomZIndices(100);
    
    // 创建 PaintLayer 树
    auto root = CreatePaintLayerTree(z_indices);
    
    // 更新 z-order 列表
    root->UpdateZOrderLists();
    
    // 验证属性：pos_z_order_list_ 按升序排列
    auto& pos_list = root->StackingNode()->PosZOrderList();
    for (size_t i = 1; i < pos_list.size(); ++i) {
        ASSERT(pos_list[i-1]->ZIndex() <= pos_list[i]->ZIndex());
    }
    
    // 验证属性：neg_z_order_list_ 按升序排列
    auto& neg_list = root->StackingNode()->NegZOrderList();
    for (size_t i = 1; i < neg_list.size(); ++i) {
        ASSERT(neg_list[i-1]->ZIndex() <= neg_list[i]->ZIndex());
    }
}

/**
 * Property Test: Round-trip 一致性
 * 
 * Feature: unified-layer-system, Property 1: PaintLayer 树 round-trip 一致性
 * Validates: Requirements 1.4, 1.5
 */
void TestRoundTripConsistency() {
    // 生成随机 PaintLayer 树
    auto original = GenerateRandomPaintLayerTree();
    
    // 序列化
    std::string serialized = original->ToDebugString();
    
    // 解析
    auto parsed = ParsePaintLayerTree(serialized);
    
    // 验证结构相同
    ASSERT(ComparePaintLayerTrees(original, parsed));
}
```

### JavaScript 测试示例

```javascript
// tests/js/test_paint_layer.js

/**
 * Property Test: Stacking Context 创建
 * 
 * Feature: unified-layer-system, Property 2: Stacking Context 创建正确性
 * Validates: Requirements 1.1
 */
function testStackingContextCreation() {
    // 测试 z-index + position 创建 stacking context
    const div = document.createElement('div');
    div.style.position = 'relative';
    div.style.zIndex = '1';
    document.body.appendChild(div);
    
    // 验证 PaintLayer 是 stacking context
    const layer = getPaintLayerForElement(div);
    assertEqual(layer.isStackingContext(), true, 
        "position:relative + z-index should create stacking context");
    
    // 测试 opacity < 1 创建 stacking context
    const div2 = document.createElement('div');
    div2.style.opacity = '0.5';
    document.body.appendChild(div2);
    
    const layer2 = getPaintLayerForElement(div2);
    assertEqual(layer2.isStackingContext(), true,
        "opacity < 1 should create stacking context");
}
```

### 测试覆盖要求

- 每个正确性属性必须有对应的属性测试
- 属性测试应运行至少 100 次迭代
- 测试注释必须引用设计文档中的属性编号

## Migration Plan

### Phase 1: 创建 PaintLayer（保持旧系统运行）

1. 创建 `paint_layer.h/cpp`
2. 实现基础功能：树结构、IsStackingContext、ZIndex
3. **LayerManager 继续正常工作**

### Phase 2: 实现核心功能

1. 实现 z-order 列表管理
2. 实现 Paint() 方法（复用 Layer::Paint 逻辑）
3. 实现 HitTest() 方法（复用 Layer::HitTest 逻辑）
4. 实现 NeedsCompositing()（复用 LayerTreeBuilder::ShouldPromote 逻辑）
5. **新旧系统并行运行，可对比验证**

### Phase 3: 切换到 PaintLayer

1. 修改 Window::Paint() 使用 PaintLayer
2. 修改 HitTesting 使用 PaintLayer
3. 验证功能等价
4. **删除 LayerManager 和 Layer**

### Phase 4: 清理和优化

1. 删除旧文件
2. 更新文档
3. 性能优化

## File Changes Summary

| 文件 | 操作 | 说明 |
|------|------|------|
| `core/render/layer/paint_layer.h` | 新增 | PaintLayer 类（整合所有功能） |
| `core/render/layer/paint_layer.cpp` | 新增 | PaintLayer 实现 |
| `core/render/objects/render_object.h` | 修改 | 添加 paint_layer_ 成员和相关方法 |
| `core/render/objects/render_object.cpp` | 修改 | 实现 EnsurePaintLayer、NeedsPaintLayer |
| `core/render/objects/render_block.cpp` | 修改 | 移除 LayerManager 调用，使用 PaintLayer |
| `core/render/objects/render_inline.cpp` | 修改 | 移除 LayerManager 调用，使用 PaintLayer |
| `core/window/window.cpp` | 修改 | 使用 PaintLayer 绘制和 hit test |
| `core/event/input/hit_testing.cpp` | 修改 | 使用 PaintLayer hit test |
| `core/event/dispatch/mouse_event_dispatcher.cpp` | 修改 | 使用 PaintLayer hit test |
| `core/render/layer/layer_manager.h` | 删除 | 功能已整合到 PaintLayer |
| `core/render/layer/layer_manager.cpp` | 删除 | 功能已整合到 PaintLayer |
| `core/render/layer/layer.h` | 删除 | 功能已整合到 PaintLayer |
| `core/render/layer/layer.cpp` | 删除 | 功能已整合到 PaintLayer |
| `core/render/layer/CMakeLists.txt` | 修改 | 更新文件列表 |
| `core/render/layer/README.md` | 修改 | 更新文档 |
| `tests/property/render/test_layer_system_properties.cpp` | 修改 | 更新测试使用 PaintLayer |
| `tests/js/test_layer_manager.js` | 删除/重写 | 更新为 PaintLayer 测试 |
