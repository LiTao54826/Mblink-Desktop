# 增量更新系统重构设计文档 V2

> 基于 Chromium Blink 引擎设计思想的改进版本

## 1. 设计目标

### 1.1 核心目标

1. **延迟同步**：DOM 变化不立即更新渲染树，而是标记脏节点，在渲染前统一处理
2. **精确失效**：只重新计算真正需要更新的节点，最小化布局和绘制开销
3. **原子性操作**：批量 DOM 操作作为一个原子单元处理，避免中间状态
4. **生命周期管理**：严格的阶段划分，防止非法状态转换

### 1.2 参考 Blink 的关键设计

| Blink 概念 | 我们的实现 | 说明 |
|-----------|-----------|------|
| DocumentLifecycle | RenderLifecycle | 渲染管线状态机 |
| SetNeedsStyleRecalc | MarkStyleDirty | 样式失效标记 |
| SetNeedsLayout | MarkNeedsLayout | 布局失效标记 |
| StyleTraversalRoot | DirtyTreeRoot | 脏节点公共祖先追踪 |
| LayoutInvalidationReason | InvalidationReason | 失效原因追踪（调试用）|


---

## 2. 架构概览

### 2.1 新架构图

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                              DOM 树                                          │
│  (Node, Element, Text)                                                      │
└─────────────────────────────────────────────────────────────────────────────┘
                                      │
                                      │ DOM 操作触发
                                      ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                         InvalidationController                               │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐              │
│  │  StyleInvalid   │  │  LayoutInvalid  │  │  PaintInvalid   │              │
│  │  - dirty_nodes  │  │  - dirty_nodes  │  │  - dirty_nodes  │              │
│  │  - dirty_root   │  │  - dirty_root   │  │  - dirty_rects  │              │
│  └─────────────────┘  └─────────────────┘  └─────────────────┘              │
└─────────────────────────────────────────────────────────────────────────────┘
                                      │
                                      │ 渲染帧开始
                                      ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                          RenderLifecycle                                     │
│                                                                              │
│   Idle → StyleRecalc → RenderTreeSync → LayoutClean → PrePaint → Paint      │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
                                      │
                                      ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                         统一渲染/布局树                                       │
│  RenderObject 包含：                                                         │
│  - 渲染信息（ComputedStyle, PaintCache）                                     │
│  - 布局信息（LayoutStyle, LayoutCache, LayoutOutput）                        │
│  - 脏标记（needs_style_recalc_, needs_layout_, needs_paint_）               │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 2.2 与当前架构的对比

| 方面 | 当前实现 | 新设计 |
|------|---------|--------|
| DOM 变化响应 | 立即触发 InvalidateRenderTree | 标记脏节点，延迟处理 |
| 脏标记位置 | RenderObject (needs_layout_) | Node + RenderObject 双层 |
| 同步时机 | 每次 DOM 操作后 | 渲染帧开始时统一处理 |
| 失效传播 | 简单向上传播 | 智能传播（考虑布局边界）|
| 调试支持 | 无 | InvalidationReason 追踪 |


---

## 3. 核心组件设计

### 3.1 渲染生命周期 (RenderLifecycle)

参考 Blink 的 `DocumentLifecycle`，定义严格的状态机：

```cpp
/**
 * @file render_lifecycle.h
 * @brief 渲染生命周期状态机
 * 
 * 参考 Blink DocumentLifecycle 设计
 */

#pragma once

#include <string>

namespace lightui {

class RenderLifecycle {
public:
    enum class State {
        kIdle,                    // 空闲状态
        
        // DOM 修改阶段
        kInDOMModification,       // DOM 正在修改（收集脏节点）
        
        // 样式阶段
        kInStyleRecalc,           // 样式重算中
        kStyleClean,              // 样式已清理
        
        // 渲染树同步阶段
        kInRenderTreeSync,        // 渲染树同步中
        kRenderTreeClean,         // 渲染树已同步
        
        // 布局阶段
        kInLayout,                // 布局计算中
        kLayoutClean,             // 布局已完成
        
        // 绘制阶段
        kInPrePaint,              // 预绘制（计算脏区域）
        kPrePaintClean,           // 预绘制完成
        kInPaint,                 // 绘制中
        kPaintClean,              // 绘制完成
        
        // 停止状态
        kStopping,
        kStopped,
    };

    // 状态转换
    void AdvanceTo(State new_state);
    void EnsureStateAtMost(State state);
    
    // 状态查询
    State GetState() const { return state_; }
    bool IsActive() const { return state_ > State::kIdle && state_ < State::kStopping; }
    
    // 状态检查（用于 DCHECK）
    bool StateAllowsTreeMutations() const;
    bool StateAllowsLayoutTreeMutations() const;
    bool StateAllowsStyleRecalc() const;
    
    // 调试
    std::string ToString() const;

private:
    bool CanAdvanceTo(State new_state) const;
    bool CanRewindTo(State new_state) const;
    
    State state_ = State::kIdle;
    int disallow_transition_count_ = 0;
};

/**
 * @brief 生命周期作用域守卫
 * 
 * RAII 风格的状态转换，确保异常安全
 */
class LifecycleScope {
public:
    LifecycleScope(RenderLifecycle& lifecycle, RenderLifecycle::State target_state);
    ~LifecycleScope();
    
private:
    RenderLifecycle& lifecycle_;
    RenderLifecycle::State final_state_;
};

/**
 * @brief 禁止状态转换的作用域
 * 
 * 在某些关键操作期间禁止状态转换
 */
class DisallowTransitionScope {
public:
    explicit DisallowTransitionScope(RenderLifecycle& lifecycle);
    ~DisallowTransitionScope();
    
private:
    RenderLifecycle& lifecycle_;
};

} // namespace lightui
```


### 3.2 失效原因追踪 (InvalidationReason)

参考 Blink 的 `LayoutInvalidationReason`，用于调试和性能分析：

```cpp
/**
 * @file invalidation_reason.h
 * @brief 失效原因追踪
 * 
 * 参考 Blink layout_invalidation_reason.h
 */

#pragma once

namespace lightui {

/**
 * @brief 样式失效原因
 */
namespace StyleInvalidationReason {
    constexpr const char* kUnknown = "Unknown";
    constexpr const char* kDOMStructureChanged = "DOM structure changed";
    constexpr const char* kAttributeChanged = "Attribute changed";
    constexpr const char* kClassChanged = "Class changed";
    constexpr const char* kStyleAttributeChanged = "Style attribute changed";
    constexpr const char* kPseudoClassChanged = "Pseudo class changed";
    constexpr const char* kInheritedStyleChanged = "Inherited style changed";
    constexpr const char* kFontChanged = "Font changed";
}

/**
 * @brief 布局失效原因
 */
namespace LayoutInvalidationReason {
    constexpr const char* kUnknown = "Unknown";
    constexpr const char* kSizeChanged = "Size changed";
    constexpr const char* kStyleChanged = "Style changed";
    constexpr const char* kDOMChanged = "DOM changed";
    constexpr const char* kTextChanged = "Text changed";
    constexpr const char* kChildChanged = "Child changed";
    constexpr const char* kAddedToLayout = "Added to layout";
    constexpr const char* kRemovedFromLayout = "Removed from layout";
    constexpr const char* kAncestorMoved = "Ancestor moved";
    constexpr const char* kScrollbarChanged = "Scrollbar changed";
    constexpr const char* kViewportChanged = "Viewport changed";
}

/**
 * @brief 绘制失效原因
 */
namespace PaintInvalidationReason {
    constexpr const char* kUnknown = "Unknown";
    constexpr const char* kStyleChanged = "Style changed";
    constexpr const char* kLayoutChanged = "Layout changed";
    constexpr const char* kScrolled = "Scrolled";
    constexpr const char* kSelection = "Selection changed";
    constexpr const char* kFocus = "Focus changed";
    constexpr const char* kHover = "Hover changed";
}

using InvalidationReasonForTracing = const char*;

} // namespace lightui
```


### 3.3 失效控制器 (InvalidationController)

核心组件，管理所有类型的失效标记：

```cpp
/**
 * @file invalidation_controller.h
 * @brief 失效控制器 - 管理样式、布局、绘制失效
 * 
 * 参考 Blink StyleEngine + LayoutObject 的失效机制
 */

#pragma once

#include <memory>
#include <unordered_set>
#include <vector>
#include "invalidation_reason.h"

namespace lightui {

class Node;
class Element;
class RenderObject;
class Document;

/**
 * @brief 脏树根追踪器
 * 
 * 参考 Blink StyleTraversalRoot
 * 追踪脏节点的公共祖先，避免全树遍历
 */
class DirtyTreeRoot {
public:
    void Update(Node* common_ancestor, Node* dirty_node);
    void Clear();
    
    Node* GetRoot() const { return root_; }
    bool HasDirtyNodes() const { return root_ != nullptr; }
    
    // 当节点从树中移除时调用
    void OnNodeRemoved(Node* node);
    
private:
    Node* root_ = nullptr;
    bool is_single_root_ = true;  // 是否只有单个脏节点
};

/**
 * @brief 失效控制器
 */
class InvalidationController {
public:
    explicit InvalidationController(Document* document);
    
    // =========================================================================
    // 样式失效
    // =========================================================================
    
    /**
     * @brief 标记节点需要样式重算
     * 
     * 类似 Blink Node::SetNeedsStyleRecalc
     */
    void MarkStyleDirty(Node* node, InvalidationReasonForTracing reason);
    
    /**
     * @brief 标记节点的子树需要样式重算
     */
    void MarkSubtreeStyleDirty(Node* node, InvalidationReasonForTracing reason);
    
    /**
     * @brief 检查是否有待处理的样式重算
     */
    bool HasPendingStyleRecalc() const { return style_dirty_root_.HasDirtyNodes(); }
    
    /**
     * @brief 获取样式重算的根节点
     */
    Node* GetStyleRecalcRoot() const { return style_dirty_root_.GetRoot(); }
    
    // =========================================================================
    // 布局失效
    // =========================================================================
    
    /**
     * @brief 标记渲染对象需要布局
     * 
     * 类似 Blink LayoutObject::SetNeedsLayout
     */
    void MarkLayoutDirty(RenderObject* obj, InvalidationReasonForTracing reason);
    
    /**
     * @brief 标记渲染对象的子节点需要布局
     */
    void MarkChildNeedsLayout(RenderObject* obj);
    
    /**
     * @brief 检查是否有待处理的布局
     */
    bool HasPendingLayout() const { return !layout_dirty_objects_.empty(); }
    
    /**
     * @brief 获取需要布局的对象列表
     */
    const std::unordered_set<RenderObject*>& GetLayoutDirtyObjects() const {
        return layout_dirty_objects_;
    }
    
    // =========================================================================
    // 绘制失效
    // =========================================================================
    
    /**
     * @brief 标记渲染对象需要重绘
     */
    void MarkPaintDirty(RenderObject* obj, InvalidationReasonForTracing reason);
    
    /**
     * @brief 添加脏矩形
     */
    void AddDirtyRect(const SkRect& rect);
    
    /**
     * @brief 检查是否有待处理的绘制
     */
    bool HasPendingPaint() const { return !paint_dirty_objects_.empty(); }
    
    /**
     * @brief 获取合并后的脏区域
     */
    SkRect GetDirtyBounds() const;
    
    // =========================================================================
    // 结构变化追踪
    // =========================================================================
    
    /**
     * @brief 记录节点添加
     */
    void RecordNodeAdded(Node* node, Node* parent);
    
    /**
     * @brief 记录节点移除
     */
    void RecordNodeRemoved(Node* node, Node* parent);
    
    /**
     * @brief 记录节点替换（原子操作）
     */
    void RecordNodeReplaced(Node* old_node, Node* new_node, Node* parent);
    
    /**
     * @brief 检查是否有结构变化
     */
    bool HasStructuralChanges() const { return !structural_changes_.empty(); }
    
    // =========================================================================
    // 清理
    // =========================================================================
    
    /**
     * @brief 清除样式失效标记
     */
    void ClearStyleInvalidation();
    
    /**
     * @brief 清除布局失效标记
     */
    void ClearLayoutInvalidation();
    
    /**
     * @brief 清除绘制失效标记
     */
    void ClearPaintInvalidation();
    
    /**
     * @brief 清除所有失效标记
     */
    void ClearAll();
    
private:
    Document* document_;
    
    // 样式失效追踪
    DirtyTreeRoot style_dirty_root_;
    std::unordered_set<Node*> style_dirty_nodes_;
    
    // 布局失效追踪
    std::unordered_set<RenderObject*> layout_dirty_objects_;
    RenderObject* layout_dirty_root_ = nullptr;
    
    // 绘制失效追踪
    std::unordered_set<RenderObject*> paint_dirty_objects_;
    std::vector<SkRect> dirty_rects_;
    
    // 结构变化记录
    struct StructuralChange {
        enum class Type { Added, Removed, Replaced };
        Type type;
        Node* node;
        Node* parent;
        Node* new_node;  // 仅用于 Replaced
    };
    std::vector<StructuralChange> structural_changes_;
};

} // namespace lightui
```


### 3.4 Node 层脏标记扩展

在 Node 类中添加样式失效标记（参考 Blink Node）：

```cpp
// 在 node.h 中添加

class Node {
public:
    // ... 现有代码 ...
    
    // =========================================================================
    // 样式失效标记（参考 Blink Node）
    // =========================================================================
    
    /**
     * @brief 样式变化类型
     */
    enum class StyleChangeType {
        kNoChange,           // 无变化
        kLocalStyleChange,   // 本地样式变化
        kSubtreeStyleChange, // 子树样式变化
        kReattachStyleChange // 需要重新附加渲染对象
    };
    
    /**
     * @brief 检查是否需要样式重算
     */
    bool NeedsStyleRecalc() const {
        return style_change_type_ != StyleChangeType::kNoChange;
    }
    
    /**
     * @brief 检查子节点是否需要样式重算
     */
    bool ChildNeedsStyleRecalc() const { return child_needs_style_recalc_; }
    
    /**
     * @brief 设置需要样式重算
     */
    void SetNeedsStyleRecalc(StyleChangeType type, InvalidationReasonForTracing reason);
    
    /**
     * @brief 清除样式重算标记
     */
    void ClearNeedsStyleRecalc();
    
    /**
     * @brief 标记祖先链需要子节点样式重算
     */
    void MarkAncestorsWithChildNeedsStyleRecalc();
    
    /**
     * @brief 获取样式重算的父节点
     * 
     * 对于 Shadow DOM，返回 flat tree 中的父节点
     */
    Element* GetStyleRecalcParent() const;
    
protected:
    StyleChangeType style_change_type_ = StyleChangeType::kNoChange;
    bool child_needs_style_recalc_ = false;
    bool needs_style_invalidation_ = false;
    bool child_needs_style_invalidation_ = false;
};
```


### 3.5 RenderObject 层脏标记扩展

扩展 RenderObject 的脏标记系统（参考 Blink LayoutObject）：

```cpp
// 在 render_object.h 中扩展

class RenderObject {
public:
    // ... 现有代码 ...
    
    // =========================================================================
    // 布局失效标记（参考 Blink LayoutObject）
    // =========================================================================
    
    /**
     * @brief 标记需要布局
     * 
     * @param reason 失效原因（用于调试）
     * @param mark_parents 是否标记父节点链
     */
    void SetNeedsLayout(InvalidationReasonForTracing reason,
                        MarkingBehavior mark_parents = kMarkContainerChain);
    
    /**
     * @brief 标记需要布局并完全重绘
     */
    void SetNeedsLayoutAndFullPaintInvalidation(InvalidationReasonForTracing reason);
    
    /**
     * @brief 标记子节点需要布局
     */
    void SetChildNeedsLayout(MarkingBehavior mark_parents = kMarkContainerChain);
    
    /**
     * @brief 清除布局标记
     */
    void ClearNeedsLayout();
    void ClearNeedsLayoutWithoutPaintInvalidation();
    
    /**
     * @brief 检查是否需要完整布局
     */
    bool SelfNeedsFullLayout() const { return self_needs_full_layout_; }
    
    /**
     * @brief 检查子节点是否需要布局
     */
    bool ChildNeedsFullLayout() const { return child_needs_full_layout_; }
    
    /**
     * @brief 检查是否需要任何布局
     */
    bool NeedsLayout() const {
        return self_needs_full_layout_ || child_needs_full_layout_ ||
               needs_simplified_layout_;
    }
    
    /**
     * @brief 标记需要简化布局（仅位置变化）
     */
    void SetNeedsSimplifiedLayout();
    
    // =========================================================================
    // 绘制失效标记
    // =========================================================================
    
    /**
     * @brief 标记需要完全重绘
     */
    void SetShouldDoFullPaintInvalidation(PaintInvalidationReasonForTracing reason);
    
    /**
     * @brief 标记需要检查绘制失效
     */
    void SetShouldCheckForPaintInvalidation();
    
    /**
     * @brief 清除绘制失效标记
     */
    void ClearPaintInvalidationFlags();
    
    // =========================================================================
    // 布局边界检测
    // =========================================================================
    
    /**
     * @brief 检查是否是布局边界
     * 
     * 布局边界内的变化不会影响外部布局
     * 参考 Blink ObjectIsRelayoutBoundary
     */
    bool IsLayoutBoundary() const;
    
    /**
     * @brief 检查是否可以跳过布局
     * 
     * 如果约束空间相同且没有脏标记，可以复用缓存结果
     */
    bool CanSkipLayout(const LayoutInput& inputs) const;
    
protected:
    // 布局脏标记
    bool self_needs_full_layout_ = true;
    bool child_needs_full_layout_ = false;
    bool needs_simplified_layout_ = false;
    bool needs_positioned_movement_layout_ = false;
    
    // 绘制脏标记
    bool should_do_full_paint_invalidation_ = false;
    bool should_check_for_paint_invalidation_ = false;
    bool subtree_should_check_for_paint_invalidation_ = false;
    
    // 内在尺寸脏标记
    bool intrinsic_logical_widths_dirty_ = true;
};

/**
 * @brief 标记行为
 */
enum MarkingBehavior {
    kMarkOnlyThis,        // 只标记当前节点
    kMarkContainerChain,  // 标记容器链
};
```



---

## 4. 渲染管线实现

### 4.1 RenderPipeline（渲染管线）

整合所有组件的核心渲染管线：

```cpp
/**
 * @file render_pipeline.h
 * @brief 渲染管线 - 整合生命周期、失效控制、布局和绘制
 */

#pragma once

#include "render_lifecycle.h"
#include "invalidation_controller.h"
#include "core/layout/layout_engine.h"
#include "core/render/render_object.h"

namespace lightui {

class Document;
class RenderTreeBuilder;

/**
 * @brief 渲染管线
 * 
 * 负责协调整个渲染流程：
 * 1. 收集 DOM 变化（脏标记）
 * 2. 样式重算
 * 3. 渲染树同步
 * 4. 布局计算
 * 5. 绘制
 */
class RenderPipeline {
public:
    explicit RenderPipeline(Document* document);
    
    /**
     * @brief 处理一帧
     * 
     * 这是渲染的主入口点，在每帧开始时调用。
     * 按照生命周期阶段依次处理所有待处理的变化。
     */
    void ProcessFrame(float viewport_width, float viewport_height);
    
    /**
     * @brief 检查是否需要渲染
     */
    bool NeedsRender() const;
    
    /**
     * @brief 获取失效控制器
     */
    InvalidationController& GetInvalidationController() { return invalidation_; }
    
    /**
     * @brief 获取生命周期
     */
    RenderLifecycle& GetLifecycle() { return lifecycle_; }
    
    /**
     * @brief 获取渲染树根节点
     */
    std::shared_ptr<RenderObject> GetRenderTree() const { return render_tree_; }
    
    /**
     * @brief 强制全量重建
     */
    void InvalidateAll();
    
private:
    // =========================================================================
    // 渲染阶段
    // =========================================================================
    
    /**
     * @brief 阶段 1：样式重算
     * 
     * 遍历脏节点，重新计算样式。
     * 参考 Blink StyleEngine::RecalcStyle
     */
    void RecalcStyle();
    
    /**
     * @brief 阶段 2：渲染树同步
     * 
     * 处理结构变化，同步渲染树与 DOM 树。
     * - 处理节点添加/删除/替换
     * - 创建/销毁 RenderObject
     */
    void SyncRenderTree();
    
    /**
     * @brief 阶段 3：布局计算
     * 
     * 计算所有脏节点的布局。
     * 支持增量布局（只计算脏子树）。
     */
    void PerformLayout(float viewport_width, float viewport_height);
    
    /**
     * @brief 阶段 4：预绘制
     * 
     * 计算脏区域，准备绘制。
     */
    void PrePaint();
    
    // =========================================================================
    // 辅助方法
    // =========================================================================
    
    /**
     * @brief 判断是否需要全量重建渲染树
     */
    bool NeedsFullRebuild() const;
    
    /**
     * @brief 全量重建渲染树
     */
    void RebuildRenderTree();
    
    /**
     * @brief 增量更新渲染树
     */
    void IncrementalUpdateRenderTree();
    
    /**
     * @brief 处理单个结构变化
     */
    void ProcessStructuralChange(const InvalidationController::StructuralChange& change);
    
private:
    Document* document_;
    RenderLifecycle lifecycle_;
    InvalidationController invalidation_;
    
    std::shared_ptr<RenderObject> render_tree_;
    std::unique_ptr<RenderTreeBuilder> render_tree_builder_;
    std::shared_ptr<LayoutEngine> layout_engine_;
    
    // 状态标记
    bool needs_full_rebuild_ = true;
    float last_viewport_width_ = 0;
    float last_viewport_height_ = 0;
};

} // namespace lightui
```

### 4.2 RenderPipeline 实现

```cpp
/**
 * @file render_pipeline.cpp
 */

#include "render_pipeline.h"
#include "core/dom/document.h"
#include "core/dom/element.h"
#include "core/render/render_tree_builder.h"

namespace lightui {

RenderPipeline::RenderPipeline(Document* document)
    : document_(document)
    , invalidation_(document)
    , render_tree_builder_(std::make_unique<RenderTreeBuilder>())
    , layout_engine_(std::make_shared<LayoutEngine>()) {
    render_tree_builder_->SetDocument(document);
}

void RenderPipeline::ProcessFrame(float viewport_width, float viewport_height) {
    // 检查视口大小变化
    if (viewport_width != last_viewport_width_ || 
        viewport_height != last_viewport_height_) {
        needs_full_rebuild_ = true;
        last_viewport_width_ = viewport_width;
        last_viewport_height_ = viewport_height;
    }
    
    // 快速路径：无需渲染
    if (!NeedsRender() && !needs_full_rebuild_) {
        return;
    }
    
    // =========================================================================
    // 阶段 1：样式重算
    // =========================================================================
    if (invalidation_.HasPendingStyleRecalc()) {
        LifecycleScope scope(lifecycle_, RenderLifecycle::State::kStyleClean);
        lifecycle_.AdvanceTo(RenderLifecycle::State::kInStyleRecalc);
        RecalcStyle();
    }
    
    // =========================================================================
    // 阶段 2：渲染树同步
    // =========================================================================
    {
        LifecycleScope scope(lifecycle_, RenderLifecycle::State::kRenderTreeClean);
        lifecycle_.AdvanceTo(RenderLifecycle::State::kInRenderTreeSync);
        
        if (needs_full_rebuild_ || NeedsFullRebuild()) {
            RebuildRenderTree();
            needs_full_rebuild_ = false;
        } else if (invalidation_.HasStructuralChanges()) {
            IncrementalUpdateRenderTree();
        }
    }
    
    // =========================================================================
    // 阶段 3：布局计算
    // =========================================================================
    if (render_tree_ && invalidation_.HasPendingLayout()) {
        LifecycleScope scope(lifecycle_, RenderLifecycle::State::kLayoutClean);
        lifecycle_.AdvanceTo(RenderLifecycle::State::kInLayout);
        PerformLayout(viewport_width, viewport_height);
    }
    
    // =========================================================================
    // 阶段 4：预绘制
    // =========================================================================
    if (invalidation_.HasPendingPaint()) {
        LifecycleScope scope(lifecycle_, RenderLifecycle::State::kPrePaintClean);
        lifecycle_.AdvanceTo(RenderLifecycle::State::kInPrePaint);
        PrePaint();
    }
    
    // 清理失效标记
    invalidation_.ClearAll();
    lifecycle_.AdvanceTo(RenderLifecycle::State::kIdle);
}

bool RenderPipeline::NeedsRender() const {
    return invalidation_.HasPendingStyleRecalc() ||
           invalidation_.HasStructuralChanges() ||
           invalidation_.HasPendingLayout() ||
           invalidation_.HasPendingPaint();
}

void RenderPipeline::RecalcStyle() {
    Node* root = invalidation_.GetStyleRecalcRoot();
    if (!root) {
        return;
    }
    
    // 遍历脏节点，重新计算样式
    std::function<void(Node*)> recalc = [&](Node* node) {
        if (!node) return;
        
        if (node->NeedsStyleRecalc()) {
            // 重新计算样式
            if (auto element = dynamic_cast<Element*>(node)) {
                if (auto ro = element->GetRenderObject()) {
                    ComputedStyle new_style = render_tree_builder_->GetStyleResolver()
                        .ResolveStyle(element->shared_from_this());
                    ro->SetComputedStyle(new_style);
                    
                    // 样式变化可能影响布局
                    invalidation_.MarkLayoutDirty(ro.get(), 
                        LayoutInvalidationReason::kStyleChanged);
                }
            }
            node->ClearNeedsStyleRecalc();
        }
        
        // 递归处理子节点
        if (node->ChildNeedsStyleRecalc()) {
            for (const auto& child : node->GetChildNodes()) {
                recalc(child.get());
            }
        }
    };
    
    recalc(root);
    invalidation_.ClearStyleInvalidation();
}

void RenderPipeline::SyncRenderTree() {
    // 由 ProcessFrame 中的条件分支处理
}

bool RenderPipeline::NeedsFullRebuild() const {
    // 启发式规则：大量结构变化时全量重建更高效
    const auto& changes = invalidation_.GetStructuralChanges();
    
    // 规则 1：变化数量超过阈值
    if (changes.size() > 10) {
        return true;
    }
    
    // 规则 2：有替换操作且涉及复杂子树
    for (const auto& change : changes) {
        if (change.type == InvalidationController::StructuralChange::Type::Replaced) {
            if (change.node && change.node->GetChildNodes().size() > 5) {
                return true;
            }
        }
    }
    
    return false;
}

void RenderPipeline::RebuildRenderTree() {
    auto body = document_->GetBody();
    if (!body) {
        return;
    }
    
    // 保存滚动位置
    std::unordered_map<Node*, std::pair<float, float>> scroll_positions;
    if (render_tree_) {
        // SaveScrollPositions(render_tree_.get(), scroll_positions);
    }
    
    // 全量重建
    render_tree_ = render_tree_builder_->BuildRenderTree(body, nullptr);
    
    // 恢复滚动位置
    if (!scroll_positions.empty() && render_tree_) {
        // RestoreScrollPositions(render_tree_.get(), scroll_positions);
    }
    
    // 标记需要完整布局
    if (render_tree_) {
        invalidation_.MarkLayoutDirty(render_tree_.get(), 
            LayoutInvalidationReason::kDOMChanged);
    }
}

void RenderPipeline::IncrementalUpdateRenderTree() {
    const auto& changes = invalidation_.GetStructuralChanges();
    
    for (const auto& change : changes) {
        ProcessStructuralChange(change);
    }
}

void RenderPipeline::ProcessStructuralChange(
    const InvalidationController::StructuralChange& change) {
    
    switch (change.type) {
        case InvalidationController::StructuralChange::Type::Added: {
            // 创建新的 RenderObject
            if (auto parent_ro = change.parent->GetRenderObject()) {
                auto new_ro = render_tree_builder_->CreateRenderObjectForNode(change.node);
                if (new_ro) {
                    // 找到正确的插入位置
                    size_t insert_pos = FindInsertPosition(parent_ro.get(), change.node);
                    
                    auto& children = parent_ro->GetChildrenMutable();
                    if (insert_pos >= children.size()) {
                        parent_ro->AppendChild(new_ro);
                    } else {
                        children.insert(children.begin() + insert_pos, new_ro);
                        new_ro->SetParent(parent_ro);
                    }
                    
                    change.node->SetRenderObject(new_ro);
                    
                    // 递归创建子节点的 RenderObject
                    CreateRenderSubtree(change.node, new_ro.get());
                    
                    // 标记需要布局
                    invalidation_.MarkLayoutDirty(parent_ro.get(),
                        LayoutInvalidationReason::kChildChanged);
                }
            }
            break;
        }
        
        case InvalidationController::StructuralChange::Type::Removed: {
            if (auto ro = change.node->GetRenderObject()) {
                if (auto parent_ro = ro->GetParent()) {
                    parent_ro->RemoveChild(ro);
                    invalidation_.MarkLayoutDirty(parent_ro.get(),
                        LayoutInvalidationReason::kChildChanged);
                }
                
                // 清除绑定
                ClearRenderObjectBindings(change.node);
            }
            break;
        }
        
        case InvalidationController::StructuralChange::Type::Replaced: {
            // 原子替换：先移除旧的，再添加新的
            // 但使用相同的位置索引
            if (auto old_ro = change.node->GetRenderObject()) {
                auto parent_ro = old_ro->GetParent();
                if (parent_ro) {
                    // 找到旧节点的位置
                    size_t pos = 0;
                    const auto& children = parent_ro->GetChildren();
                    for (size_t i = 0; i < children.size(); ++i) {
                        if (children[i].get() == old_ro.get()) {
                            pos = i;
                            break;
                        }
                    }
                    
                    // 移除旧节点
                    parent_ro->RemoveChild(old_ro);
                    ClearRenderObjectBindings(change.node);
                    
                    // 在相同位置插入新节点
                    auto new_ro = render_tree_builder_->CreateRenderObjectForNode(change.new_node);
                    if (new_ro) {
                        auto& mutable_children = parent_ro->GetChildrenMutable();
                        if (pos >= mutable_children.size()) {
                            parent_ro->AppendChild(new_ro);
                        } else {
                            mutable_children.insert(mutable_children.begin() + pos, new_ro);
                            new_ro->SetParent(parent_ro);
                        }
                        
                        change.new_node->SetRenderObject(new_ro);
                        CreateRenderSubtree(change.new_node, new_ro.get());
                    }
                    
                    invalidation_.MarkLayoutDirty(parent_ro.get(),
                        LayoutInvalidationReason::kChildChanged);
                }
            }
            break;
        }
    }
}

void RenderPipeline::PerformLayout(float viewport_width, float viewport_height) {
    if (!render_tree_ || !layout_engine_) {
        return;
    }
    
    // 检查是否可以增量布局
    bool can_incremental = layout_engine_->CanIncrementalLayout();
    
    if (can_incremental) {
        // 增量布局：只计算脏子树
        layout_engine_->ComputeIncrementalLayout(viewport_width, viewport_height);
    } else {
        // 全量布局
        layout_engine_->BuildLayoutTree(render_tree_);
        layout_engine_->ComputeLayout(viewport_width, viewport_height);
    }
    
    // 获取布局结果
    layout_engine_->GetLayoutInfo(render_tree_);
    
    // 布局完成后，标记需要重绘
    // 这里可以优化：只标记布局变化的节点
    invalidation_.MarkPaintDirty(render_tree_.get(),
        PaintInvalidationReason::kLayoutChanged);
    
    invalidation_.ClearLayoutInvalidation();
}

void RenderPipeline::PrePaint() {
    // 计算脏区域
    SkRect dirty_bounds = invalidation_.GetDirtyBounds();
    
    // 如果脏区域为空，使用整个视口
    if (dirty_bounds.isEmpty() && render_tree_) {
        dirty_bounds = render_tree_->GetBoundingRect();
    }
    
    // 存储脏区域供绘制使用
    // （实际绘制在 Window::Render 中进行）
}

void RenderPipeline::InvalidateAll() {
    needs_full_rebuild_ = true;
}

} // namespace lightui
```

---

## 5. 与现有代码的集成

### 5.1 修改 WindowDOMObserver

将现有的立即更新改为标记脏节点：

```cpp
class WindowDOMObserver : public DOMObserver {
public:
    explicit WindowDOMObserver(Window* window) : window_(window) {}

    void OnNodeAdded(Node* node, Node* parent) override {
        if (window_ && !IsInBatch(node)) {
            // 新方式：记录结构变化，延迟处理
            auto& pipeline = window_->GetRenderPipeline();
            pipeline.GetInvalidationController().RecordNodeAdded(node, parent);
            window_->SetNeedsRepaint();
        }
    }

    void OnNodeRemoved(Node* node, Node* parent) override {
        if (window_ && !IsInBatch(node)) {
            auto& pipeline = window_->GetRenderPipeline();
            pipeline.GetInvalidationController().RecordNodeRemoved(node, parent);
            window_->SetNeedsRepaint();
        }
    }

    void OnStyleChanged(Element* element, const std::string& property,
                       const std::string& old_value, const std::string& new_value) override {
        if (window_ && !IsInBatch(element)) {
            auto& pipeline = window_->GetRenderPipeline();
            
            // display 属性变化需要特殊处理
            if (property == "display") {
                bool was_none = (old_value == "none" || old_value.empty());
                bool is_none = (new_value == "none");
                if (was_none != is_none) {
                    // 可见性变化，需要重建渲染树
                    pipeline.InvalidateAll();
                    window_->SetNeedsRepaint();
                    return;
                }
            }
            
            // 标记样式脏
            pipeline.GetInvalidationController().MarkStyleDirty(
                element, StyleInvalidationReason::kStyleAttributeChanged);
            window_->SetNeedsRepaint();
        }
    }

    void OnTextChanged(Node* node, const std::string& old_text,
                      const std::string& new_text) override {
        if (window_ && !IsInBatch(node)) {
            auto& pipeline = window_->GetRenderPipeline();
            
            // 文本变化需要布局
            if (auto ro = node->GetRenderObject()) {
                pipeline.GetInvalidationController().MarkLayoutDirty(
                    ro.get(), LayoutInvalidationReason::kTextChanged);
            }
            window_->SetNeedsRepaint();
        }
    }
    
    // ... 其他方法类似修改 ...
};
```

### 5.2 修改 Window::Render

```cpp
void Window::Render() {
    if (!document_ || !surface_) {
        return;
    }
    
    // 获取视口大小
    int width, height;
    GetSize(&width, &height);
    float dpi_scale = GetDisplayScale();
    float viewport_width = width / dpi_scale;
    float viewport_height = height / dpi_scale;
    
    // 处理动画
    UpdateAnimations(GetTimestamp());
    
    // 核心：使用渲染管线处理一帧
    render_pipeline_.ProcessFrame(viewport_width, viewport_height);
    
    // 检查是否需要绘制
    if (!needs_repaint_ && !render_pipeline_.NeedsRender()) {
        return;  // 快速路径：无需绘制
    }
    
    // 获取渲染树
    auto render_tree = render_pipeline_.GetRenderTree();
    if (!render_tree) {
        return;
    }
    
    // 绘制
    SkCanvas* canvas = surface_->getCanvas();
    canvas->save();
    canvas->scale(dpi_scale, dpi_scale);
    
    // 获取脏区域
    SkRect dirty_bounds = render_pipeline_.GetInvalidationController().GetDirtyBounds();
    
    if (dirty_bounds.isEmpty() || force_full_repaint_) {
        // 全量绘制
        canvas->clear(GetBackgroundColor());
        render_tree->Paint(canvas);
    } else {
        // 增量绘制：只重绘脏区域
        canvas->save();
        canvas->clipRect(dirty_bounds);
        canvas->clear(GetBackgroundColor());
        render_tree->Paint(canvas);
        canvas->restore();
    }
    
    canvas->restore();
    
    // 清除标记
    needs_repaint_ = false;
    force_full_repaint_ = false;
}
```

---

## 6. 实现计划

### 6.1 阶段一：基础设施（1-2 周）

1. **实现 RenderLifecycle**
   - 状态机和状态转换
   - Scope 守卫类
   - 单元测试

2. **实现 InvalidationController**
   - DirtyTreeRoot 追踪
   - 样式/布局/绘制失效标记
   - 结构变化记录

3. **实现 InvalidationReason**
   - 失效原因常量
   - 调试日志支持

### 6.2 阶段二：渲染管线（2-3 周）

1. **实现 RenderPipeline**
   - 帧处理流程
   - 样式重算
   - 渲染树同步
   - 布局计算

2. **修改 Node 和 RenderObject**
   - 添加脏标记字段
   - 实现标记方法

3. **集成到 Window**
   - 修改 WindowDOMObserver
   - 修改 Window::Render

### 6.3 阶段三：优化和测试（1-2 周）

1. **性能优化**
   - 启发式规则调优
   - 缓存策略
   - 增量布局优化

2. **测试**
   - 单元测试
   - 集成测试
   - Preact demo 测试

---

## 7. 总结

### 7.1 关键改进

| 方面 | 改进前 | 改进后 |
|------|--------|--------|
| DOM 变化响应 | 立即重建渲染树 | 标记脏节点，延迟处理 |
| 同步时机 | 每次 DOM 操作 | 渲染帧开始时统一处理 |
| 失效传播 | 简单向上传播 | 智能传播（考虑布局边界）|
| 调试支持 | 无 | InvalidationReason 追踪 |
| 生命周期 | 无明确阶段 | 严格的状态机管理 |

### 7.2 预期收益

1. **性能提升**：批量处理 DOM 变化，减少重复计算
2. **稳定性提升**：原子性操作，避免中间状态
3. **可维护性提升**：清晰的生命周期阶段
4. **可调试性提升**：失效原因追踪

### 7.3 风险控制

1. **渐进式迁移**：保留旧代码路径，通过 feature flag 切换
2. **充分测试**：单元测试 + 集成测试 + 性能测试
3. **回退机制**：出现问题时可快速回退到旧实现
