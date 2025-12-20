# 增量更新系统重构设计文档

## 1. 问题分析

### 1.1 当前架构概述

```
┌─────────────────────────────────────────────────────────────────────┐
│                           DOM 树                                     │
│  (Node, Element, Text)                                              │
└─────────────────────────────────────────────────────────────────────┘
                                    │
                                    │ DOMObserver 通知
                                    ▼
┌─────────────────────────────────────────────────────────────────────┐
│                      WindowDOMObserver                               │
│  OnNodeAdded / OnNodeRemoved / OnStyleChanged / OnTextChanged       │
└─────────────────────────────────────────────────────────────────────┘
                                    │
                    ┌───────────────┴───────────────┐
                    │                               │
                    ▼                               ▼
┌───────────────────────────────┐   ┌───────────────────────────────┐
│         渲染树                 │   │         布局树                 │
│  (RenderObject 树)            │   │  (NativeLayoutEngine 内部)     │
│  - RenderBlock                │   │  - LayoutNode                 │
│  - RenderInline               │   │  - 匿名块盒                    │
│  - RenderText                 │   │  - IFC 容器                   │
└───────────────────────────────┘   └───────────────────────────────┘
```

### 1.2 核心问题

#### 问题 1：双树同步困难

渲染树和布局树是两棵独立的树，结构不完全一致：

```
渲染树:                          布局树:
  RenderBlock (div)               LayoutNode (div)
    ├─ RenderText ("Hello")         ├─ AnonymousBlockBox (IFC)
    ├─ RenderBlock (p)              │     └─ [inline: "Hello", button]
    └─ RenderInline (button)        └─ LayoutNode (p)
```

- 布局树有**匿名块盒**（Anonymous Block Box）来包裹混合内容中的内联元素
- 布局树有**IFC 容器**标记
- 两棵树的节点数量和结构可能不同

**后果**：
- 渲染树的插入位置索引不能直接用于布局树
- 增量插入时可能插入到错误位置或索引越界

#### 问题 2：ReplaceChild 时序问题

Preact 的 diff 算法大量使用 `replaceChild`：

```javascript
// Preact diff 算法
parentDOM.replaceChild(newDOM, oldDOM);
```

对应的 C++ 实现：

```cpp
// node.cpp - ReplaceChild
void Node::ReplaceChild(new_child, old_child) {
    // 1. NotifyNodeRemoved(old_child)  ← 此时 old_child 还在 DOM 树中
    // 2. *it = new_child               ← DOM 树修改
    // 3. NotifyNodeAdded(new_child)    ← 此时 new_child 已在 DOM 树中
}
```

**问题**：
- 步骤 1 触发 `RemoveRenderObject`，移除旧节点的 RenderObject
- 步骤 3 触发 `InsertRenderObject`，需要找到正确的插入位置
- 但此时布局树可能处于不一致状态（旧节点已移除，新节点还未添加）

#### 问题 3：缺乏原子性

每个 DOM 操作都立即触发渲染树/布局树更新：

```
DOM 操作 1 → 立即更新渲染树 → 立即更新布局树
DOM 操作 2 → 立即更新渲染树 → 立即更新布局树
DOM 操作 3 → 立即更新渲染树 → 立即更新布局树
...
渲染帧
```

**问题**：
- 中间状态可能不一致
- 无法合并多个操作
- 性能浪费（多次更新，只需要最终结果）

#### 问题 4：布局树顺序问题

当前 `AddElement` 总是将新节点添加到布局树末尾：

```cpp
void AddElement(RenderObject* render_obj, RenderObject* parent) {
    // ...
    BuildSubtree(render_obj, parent_id);  // 总是添加到末尾
}
```

**问题**：
- 页面切换时，新页面的元素被添加到旧页面元素之后
- 布局计算时顺序错误，导致空白区域

### 1.3 问题总结

| 问题 | 根因 | 影响 |
|------|------|------|
| 双树同步 | 渲染树和布局树结构不一致 | 索引计算错误、崩溃 |
| ReplaceChild 时序 | 先删后加的中间状态 | 插入位置错误 |
| 缺乏原子性 | 每次操作立即更新 | 中间状态不一致、性能差 |
| 布局树顺序 | 总是添加到末尾 | 空白区域、布局错误 |

---

## 2. 业界方案分析

### 2.1 Chromium Blink 引擎

Blink 使用**延迟同步 + 脏标记**机制：

```
DOM 操作 → 标记脏节点 → ... → 渲染前同步 → 布局 → 绘制
```

关键设计：
1. **StyleInvalidation**：样式失效传播
2. **LayoutInvalidation**：布局失效传播
3. **PaintInvalidation**：绘制失效传播
4. **Lifecycle**：严格的生命周期阶段

```cpp
// Blink 的生命周期阶段
enum class DocumentLifecycle {
    kInactive,
    kVisualUpdatePending,
    kInStyleRecalc,
    kStyleClean,
    kInLayoutSubtreeChange,
    kLayoutSubtreeChangeClean,
    kInPreLayout,
    kInPerformLayout,
    kAfterPerformLayout,
    kLayoutClean,
    kInCompositingInputsUpdate,
    kCompositingInputsClean,
    kInCompositingAssignmentsUpdate,
    kCompositingAssignmentsClean,
    kInPrePaint,
    kPrePaintClean,
    kInPaint,
    kPaintClean,
};
```

### 2.2 WebKit

WebKit 使用类似的**脏标记 + 延迟更新**：

```cpp
// WebKit 的脏标记
enum StyleChange {
    NoStyleChange,
    InlineStyleChange,
    FullStyleChange,
    ReconstructRenderTree,
};
```

关键设计：
1. **setNeedsStyleRecalc()**：标记需要样式重算
2. **setNeedsLayout()**：标记需要布局
3. **setNeedsRepaint()**：标记需要重绘
4. 在 `updateRendering()` 中统一处理

### 2.3 React Fiber

React 使用**双缓冲 + 协调**：

```
Current Tree ←→ Work-in-Progress Tree
```

关键设计：
1. **Reconciliation**：在 WIP 树上进行 diff
2. **Commit**：一次性提交所有变更
3. **原子性**：要么全部应用，要么全部回滚

### 2.4 Flutter

Flutter 使用**三棵树**：

```
Widget Tree → Element Tree → RenderObject Tree
```

关键设计：
1. **Widget**：不可变的配置描述
2. **Element**：Widget 的实例，管理生命周期
3. **RenderObject**：实际的布局和绘制
4. **markNeedsBuild()**：标记需要重建
5. **markNeedsLayout()**：标记需要布局
6. **markNeedsPaint()**：标记需要绘制

---

## 3. 重构方案

### 3.1 方案概述

采用**延迟同步 + 统一树 + 生命周期**的设计：

```
┌─────────────────────────────────────────────────────────────────────┐
│                           DOM 树                                     │
│  (Node, Element, Text)                                              │
└─────────────────────────────────────────────────────────────────────┘
                                    │
                                    │ 标记脏节点（不立即更新）
                                    ▼
┌─────────────────────────────────────────────────────────────────────┐
│                      DirtyNodeTracker                                │
│  - pending_structural_changes_  (结构变化：添加/删除/移动)           │
│  - pending_style_changes_       (样式变化)                          │
│  - pending_text_changes_        (文本变化)                          │
└─────────────────────────────────────────────────────────────────────┘
                                    │
                                    │ 渲染前同步（SyncRenderTree）
                                    ▼
┌─────────────────────────────────────────────────────────────────────┐
│                      统一渲染/布局树                                 │
│  RenderObject 同时包含：                                            │
│  - 渲染信息（样式、绘制）                                           │
│  - 布局信息（尺寸、位置、缓存）                                     │
└─────────────────────────────────────────────────────────────────────┘
                                    │
                                    │ 布局计算
                                    ▼
┌─────────────────────────────────────────────────────────────────────┐
│                      NativeLayoutEngine                              │
│  - 遍历 RenderObject 树进行布局                                     │
│  - 不再维护独立的 LayoutNode 树                                     │
│  - 匿名块盒作为 RenderObject 的内部状态                             │
└─────────────────────────────────────────────────────────────────────┘
```

### 3.2 核心组件设计

#### 3.2.1 DirtyNodeTracker（脏节点追踪器）

```cpp
/**
 * @brief 脏节点追踪器
 * 
 * 收集 DOM 变化，延迟到渲染前统一处理。
 * 这避免了中间状态不一致的问题。
 */
class DirtyNodeTracker {
public:
    // 结构变化类型
    enum class StructuralChangeType {
        Added,      // 节点被添加
        Removed,    // 节点被移除
        Moved,      // 节点被移动
        Replaced,   // 节点被替换（ReplaceChild 的原子操作）
    };
    
    // 结构变化记录
    struct StructuralChange {
        StructuralChangeType type;
        std::weak_ptr<Node> node;           // 变化的节点
        std::weak_ptr<Node> parent;         // 父节点
        std::weak_ptr<Node> old_parent;     // 旧父节点（用于 Moved）
        std::weak_ptr<Node> new_node;       // 新节点（用于 Replaced）
        size_t index;                       // 在父节点中的索引
    };
    
    // 样式变化记录
    struct StyleChange {
        std::weak_ptr<Element> element;
        std::string property;
        std::string old_value;
        std::string new_value;
    };
    
    // 文本变化记录
    struct TextChange {
        std::weak_ptr<Node> node;
        std::string old_text;
        std::string new_text;
    };
    
    // 记录变化（由 DOMObserver 调用）
    void RecordNodeAdded(Node* node, Node* parent, size_t index);
    void RecordNodeRemoved(Node* node, Node* parent, size_t index);
    void RecordNodeReplaced(Node* old_node, Node* new_node, Node* parent, size_t index);
    void RecordStyleChanged(Element* element, const std::string& property,
                           const std::string& old_value, const std::string& new_value);
    void RecordTextChanged(Node* node, const std::string& old_text, const std::string& new_text);
    
    // 检查是否有待处理的变化
    bool HasPendingChanges() const;
    
    // 获取待处理的变化（用于同步）
    const std::vector<StructuralChange>& GetStructuralChanges() const;
    const std::vector<StyleChange>& GetStyleChanges() const;
    const std::vector<TextChange>& GetTextChanges() const;
    
    // 清除所有待处理的变化
    void Clear();
    
    // 优化：合并冗余变化
    void Optimize();
    
private:
    std::vector<StructuralChange> structural_changes_;
    std::vector<StyleChange> style_changes_;
    std::vector<TextChange> text_changes_;
    
    // 用于快速查找的索引
    std::unordered_set<Node*> added_nodes_;
    std::unordered_set<Node*> removed_nodes_;
};
```

#### 3.2.2 RenderTreeSynchronizer（渲染树同步器）

```cpp
/**
 * @brief 渲染树同步器
 * 
 * 在渲染前将 DOM 变化同步到渲染树。
 * 处理所有待处理的变化，确保渲染树与 DOM 树一致。
 */
class RenderTreeSynchronizer {
public:
    /**
     * @brief 同步渲染树
     * 
     * 处理所有待处理的 DOM 变化，更新渲染树。
     * 这是一个原子操作，要么全部成功，要么回滚。
     * 
     * @param tracker 脏节点追踪器
     * @param render_tree 渲染树根节点
     * @return 是否有变化被应用
     */
    bool Synchronize(DirtyNodeTracker& tracker, 
                     std::shared_ptr<RenderObject> render_tree);
    
private:
    // 处理结构变化
    void ProcessStructuralChanges(const std::vector<DirtyNodeTracker::StructuralChange>& changes);
    
    // 处理样式变化
    void ProcessStyleChanges(const std::vector<DirtyNodeTracker::StyleChange>& changes);
    
    // 处理文本变化
    void ProcessTextChanges(const std::vector<DirtyNodeTracker::TextChange>& changes);
    
    // 判断是否需要重建子树
    bool NeedsSubtreeRebuild(const std::vector<DirtyNodeTracker::StructuralChange>& changes);
    
    // 重建子树
    void RebuildSubtree(Node* root);
    
    // 增量更新单个节点
    void UpdateNode(Node* node);
    
    // 查找 DOM 节点在父节点中的索引
    size_t FindNodeIndex(Node* node, Node* parent);
    
    // 查找 RenderObject 在父节点中的索引
    size_t FindRenderObjectIndex(RenderObject* ro, RenderObject* parent);
};
```

#### 3.2.3 统一的 RenderObject

当前 `RenderObject` 已经包含了布局相关的字段，但 `NativeLayoutEngine` 仍然维护独立的 `LayoutNode`。重构后：

```cpp
class RenderObject {
    // ... 现有字段 ...
    
    // 布局相关（已有，保持不变）
    Style layout_style_;
    Cache layout_cache_;
    LayoutOutput layout_output_;
    Layout unrounded_layout_;
    
    // 新增：匿名块盒支持
    struct AnonymousBlockInfo {
        bool is_anonymous = false;              // 是否是匿名块盒
        std::vector<RenderObject*> inline_children;  // 包含的内联子元素
    };
    std::optional<AnonymousBlockInfo> anonymous_block_info_;
    
    // 新增：IFC 布局结果
    struct IFCLayoutResult {
        std::vector<InlineBox> inline_boxes;
        std::vector<LineBox> line_boxes;
        IFCMeasureResult measure_result;
    };
    std::optional<IFCLayoutResult> ifc_result_;
};
```

#### 3.2.4 简化的 NativeLayoutEngine

```cpp
class NativeLayoutEngine {
public:
    /**
     * @brief 计算布局
     * 
     * 直接遍历 RenderObject 树进行布局计算。
     * 不再维护独立的 LayoutNode 树。
     */
    void ComputeLayout(RenderObject* root, float available_width, float available_height);
    
    /**
     * @brief 增量布局
     * 
     * 只重新计算标记为 needs_layout 的节点。
     */
    bool ComputeIncrementalLayout(RenderObject* root, float available_width, float available_height);
    
private:
    // 计算单个节点的布局
    LayoutOutput ComputeNodeLayout(RenderObject* node, const LayoutInput& inputs);
    
    // 处理匿名块盒
    void ProcessAnonymousBlocks(RenderObject* node);
    
    // 创建匿名块盒（作为 RenderObject 的内部状态，不创建新节点）
    void CreateAnonymousBlockInfo(RenderObject* node);
};
```

### 3.3 生命周期设计

```cpp
/**
 * @brief 渲染生命周期
 */
enum class RenderLifecycle {
    Idle,                    // 空闲
    DOMModification,         // DOM 修改中（收集变化）
    StyleRecalc,             // 样式重算
    RenderTreeSync,          // 渲染树同步
    Layout,                  // 布局计算
    Paint,                   // 绘制
    Composite,               // 合成
};

class RenderPipeline {
public:
    void ProcessFrame() {
        // 1. 同步渲染树
        if (dirty_tracker_.HasPendingChanges()) {
            lifecycle_ = RenderLifecycle::RenderTreeSync;
            synchronizer_.Synchronize(dirty_tracker_, render_tree_);
            dirty_tracker_.Clear();
        }
        
        // 2. 样式重算（如果需要）
        if (needs_style_recalc_) {
            lifecycle_ = RenderLifecycle::StyleRecalc;
            RecalcStyles();
        }
        
        // 3. 布局
        if (needs_layout_) {
            lifecycle_ = RenderLifecycle::Layout;
            layout_engine_.ComputeLayout(render_tree_.get(), width_, height_);
        }
        
        // 4. 绘制
        lifecycle_ = RenderLifecycle::Paint;
        Paint();
        
        lifecycle_ = RenderLifecycle::Idle;
    }
    
private:
    RenderLifecycle lifecycle_ = RenderLifecycle::Idle;
    DirtyNodeTracker dirty_tracker_;
    RenderTreeSynchronizer synchronizer_;
    NativeLayoutEngine layout_engine_;
    std::shared_ptr<RenderObject> render_tree_;
};
```

### 3.4 DOM 操作的新流程

#### 3.4.1 AppendChild

```cpp
void Node::AppendChild(std::shared_ptr<Node> child) {
    // 1. 修改 DOM 树
    child_nodes_.push_back(child);
    child->SetParentNode(shared_from_this());
    
    // 2. 记录变化（不立即更新渲染树）
    if (auto doc = GetOwnerDocument()) {
        doc->GetDirtyTracker().RecordNodeAdded(
            child.get(), 
            this, 
            child_nodes_.size() - 1
        );
    }
}
```

#### 3.4.2 RemoveChild

```cpp
void Node::RemoveChild(std::shared_ptr<Node> child) {
    auto it = std::find(child_nodes_.begin(), child_nodes_.end(), child);
    size_t index = std::distance(child_nodes_.begin(), it);
    
    // 1. 记录变化（在修改前）
    if (auto doc = GetOwnerDocument()) {
        doc->GetDirtyTracker().RecordNodeRemoved(child.get(), this, index);
    }
    
    // 2. 修改 DOM 树
    child_nodes_.erase(it);
    child->SetParentNode(nullptr);
}
```

#### 3.4.3 ReplaceChild（关键改进）

```cpp
void Node::ReplaceChild(std::shared_ptr<Node> new_child, std::shared_ptr<Node> old_child) {
    auto it = std::find(child_nodes_.begin(), child_nodes_.end(), old_child);
    size_t index = std::distance(child_nodes_.begin(), it);
    
    // 1. 记录为原子替换操作（不是先删后加）
    if (auto doc = GetOwnerDocument()) {
        doc->GetDirtyTracker().RecordNodeReplaced(
            old_child.get(),
            new_child.get(),
            this,
            index
        );
    }
    
    // 2. 修改 DOM 树
    *it = new_child;
    old_child->SetParentNode(nullptr);
    new_child->SetParentNode(shared_from_this());
}
```

### 3.5 渲染树同步策略

```cpp
bool RenderTreeSynchronizer::Synchronize(DirtyNodeTracker& tracker,
                                          std::shared_ptr<RenderObject> render_tree) {
    if (!tracker.HasPendingChanges()) {
        return false;
    }
    
    // 优化变化列表（合并冗余操作）
    tracker.Optimize();
    
    const auto& structural_changes = tracker.GetStructuralChanges();
    
    // 策略选择：根据变化量决定增量更新还是重建
    if (NeedsSubtreeRebuild(structural_changes)) {
        // 大量变化：重建受影响的子树
        std::unordered_set<Node*> affected_roots;
        for (const auto& change : structural_changes) {
            if (auto parent = change.parent.lock()) {
                affected_roots.insert(parent.get());
            }
        }
        
        for (Node* root : affected_roots) {
            RebuildSubtree(root);
        }
    } else {
        // 少量变化：增量更新
        ProcessStructuralChanges(structural_changes);
    }
    
    // 处理样式和文本变化
    ProcessStyleChanges(tracker.GetStyleChanges());
    ProcessTextChanges(tracker.GetTextChanges());
    
    return true;
}

bool RenderTreeSynchronizer::NeedsSubtreeRebuild(
    const std::vector<DirtyNodeTracker::StructuralChange>& changes) {
    
    // 启发式规则：
    // 1. 变化数量超过阈值
    if (changes.size() > 10) {
        return true;
    }
    
    // 2. 有 Replaced 操作且涉及复杂子树
    for (const auto& change : changes) {
        if (change.type == DirtyNodeTracker::StructuralChangeType::Replaced) {
            // 检查被替换的节点是否有子节点
            if (auto node = change.node.lock()) {
                if (node->GetChildNodes().size() > 5) {
                    return true;
                }
            }
        }
    }
    
    // 3. 同一父节点下有多个变化
    std::unordered_map<Node*, int> parent_change_count;
    for (const auto& change : changes) {
        if (auto parent = change.parent.lock()) {
            if (++parent_change_count[parent.get()] > 3) {
                return true;
            }
        }
    }
    
    return false;
}
```

---

## 4. 实现计划

### 4.1 阶段一：基础设施（2-3 周）

1. **实现 DirtyNodeTracker**
   - 变化记录
   - 变化优化（合并冗余）
   - 单元测试

2. **修改 DOM 操作**
   - AppendChild/RemoveChild/ReplaceChild 改为记录变化
   - InsertBefore/RemoveAllChildren 等其他操作
   - 保持向后兼容

3. **实现 RenderTreeSynchronizer**
   - 基本同步逻辑
   - 增量更新
   - 子树重建

### 4.2 阶段二：布局引擎重构（3-4 周）

1. **统一 RenderObject 和 LayoutNode**
   - 将 LayoutNode 的字段移入 RenderObject
   - 移除 NativeLayoutEngine 中的 nodes_ 和 render_to_node_
   - 直接遍历 RenderObject 树

2. **匿名块盒处理**
   - 作为 RenderObject 的内部状态
   - 不创建独立节点
   - 在布局时动态处理

3. **IFC 布局集成**
   - IFC 结果存储在 RenderObject 中
   - 增量更新支持

### 4.3 阶段三：优化和测试（2-3 周）

1. **性能优化**
   - 变化合并优化
   - 缓存策略
   - 增量布局优化

2. **测试**
   - 单元测试
   - 集成测试
   - 性能测试
   - Preact demo 测试

3. **文档**
   - API 文档
   - 架构文档
   - 迁移指南

---

## 5. 风险和缓解

### 5.1 风险：大规模重构可能引入 bug

**缓解**：
- 分阶段实施
- 保持旧代码可用（feature flag）
- 充分的测试覆盖

### 5.2 风险：性能可能下降

**缓解**：
- 性能基准测试
- 增量优化
- 回退机制

### 5.3 风险：与现有代码不兼容

**缓解**：
- 保持 API 兼容
- 渐进式迁移
- 详细的迁移文档

---

## 6. 附录

### 6.1 当前代码结构

```
core/
├── dom/
│   ├── node.cpp              # DOM 节点操作
│   ├── dom_observer.h        # DOM 观察者接口
│   └── document.cpp          # 文档管理
├── render/
│   ├── render_object.h       # 渲染对象
│   ├── render_tree_updater.h # 渲染树更新器（当前）
│   └── style_resolver.cpp    # 样式解析
├── layout/
│   ├── native_layout_engine.h # 布局引擎
│   └── native_layout_engine.cpp
└── window/
    └── window.cpp            # 窗口和渲染循环
```

### 6.2 新增代码结构

```
core/
├── dom/
│   └── dirty_node_tracker.h  # 新增：脏节点追踪器
├── render/
│   ├── render_tree_synchronizer.h  # 新增：渲染树同步器
│   └── render_pipeline.h     # 新增：渲染管线
└── ...
```

### 6.3 参考资料

- [Chromium Blink Rendering Pipeline](https://chromium.googlesource.com/chromium/src/+/master/third_party/blink/renderer/core/README.md)
- [WebKit Rendering](https://webkit.org/blog/114/webcore-rendering-i-the-basics/)
- [React Fiber Architecture](https://github.com/acdlite/react-fiber-architecture)
- [Flutter Rendering Pipeline](https://flutter.dev/docs/resources/architectural-overview#rendering)
