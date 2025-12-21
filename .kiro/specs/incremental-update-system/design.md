# Design Document: 增量更新系统重构

## Overview

本设计文档描述了 LightUI 渲染引擎增量更新系统的重构方案。核心思想是采用**延迟同步 + 统一树 + 生命周期**的设计，解决当前系统存在的双树同步困难、ReplaceChild 时序问题、缺乏原子性等问题。

### 设计目标

1. **解决双树同步问题**：通过延迟同步机制，避免中间状态不一致
2. **原子性操作**：ReplaceChild 等操作作为原子操作处理
3. **性能优化**：批量处理 DOM 变化，减少不必要的更新
4. **清晰的生命周期**：定义明确的渲染管线阶段

## Architecture

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
│  - pending_structural_changes_  (结构变化：添加/删除/移动/替换)      │
│  - pending_style_changes_       (样式变化)                          │
│  - pending_text_changes_        (文本变化)                          │
└─────────────────────────────────────────────────────────────────────┘
                                    │
                                    │ 渲染前同步（SyncRenderTree）
                                    ▼
┌─────────────────────────────────────────────────────────────────────┐
│                      RenderTreeSynchronizer                          │
│  - ProcessStructuralChanges()                                       │
│  - ProcessStyleChanges()                                            │
│  - ProcessTextChanges()                                             │
│  - NeedsSubtreeRebuild() 策略选择                                   │
└─────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────┐
│                      RenderPipeline                                  │
│  Lifecycle: Idle → RenderTreeSync → StyleRecalc → Layout → Paint    │
└─────────────────────────────────────────────────────────────────────┘
```

## Components and Interfaces

### 1. DirtyNodeTracker (脏节点追踪器)

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
    
    // 记录变化
    void RecordNodeAdded(Node* node, Node* parent, size_t index);
    void RecordNodeRemoved(Node* node, Node* parent, size_t index);
    void RecordNodeReplaced(Node* old_node, Node* new_node, Node* parent, size_t index);
    void RecordStyleChanged(Element* element, const std::string& property,
                           const std::string& old_value, const std::string& new_value);
    void RecordTextChanged(Node* node, const std::string& old_text, const std::string& new_text);
    
    // 查询
    bool HasPendingChanges() const;
    const std::vector<StructuralChange>& GetStructuralChanges() const;
    const std::vector<StyleChange>& GetStyleChanges() const;
    const std::vector<TextChange>& GetTextChanges() const;
    
    // 管理
    void Clear();
    void Optimize();  // 合并冗余变化
    
private:
    std::vector<StructuralChange> structural_changes_;
    std::vector<StyleChange> style_changes_;
    std::vector<TextChange> text_changes_;
    std::unordered_set<Node*> added_nodes_;
    std::unordered_set<Node*> removed_nodes_;
};
```

### 2. RenderTreeSynchronizer (渲染树同步器)

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
     * @param tracker 脏节点追踪器
     * @param render_tree 渲染树根节点
     * @return 是否有变化被应用
     */
    bool Synchronize(DirtyNodeTracker& tracker, 
                     std::shared_ptr<RenderObject> render_tree);
    
private:
    void ProcessStructuralChanges(const std::vector<DirtyNodeTracker::StructuralChange>& changes);
    void ProcessStyleChanges(const std::vector<DirtyNodeTracker::StyleChange>& changes);
    void ProcessTextChanges(const std::vector<DirtyNodeTracker::TextChange>& changes);
    
    bool NeedsSubtreeRebuild(const std::vector<DirtyNodeTracker::StructuralChange>& changes);
    void RebuildSubtree(Node* root);
    void UpdateNode(Node* node);
};
```

### 3. RenderPipeline (渲染管线)

```cpp
/**
 * @brief 渲染生命周期
 */
enum class RenderLifecycle {
    Idle,                    // 空闲
    DOMModification,         // DOM 修改中
    RenderTreeSync,          // 渲染树同步
    StyleRecalc,             // 样式重算
    Layout,                  // 布局计算
    Paint,                   // 绘制
};

/**
 * @brief 渲染管线
 */
class RenderPipeline {
public:
    void ProcessFrame();
    RenderLifecycle GetLifecycle() const;
    
    void SetDirtyTracker(DirtyNodeTracker* tracker);
    void SetRenderTree(std::shared_ptr<RenderObject> render_tree);
    void SetLayoutEngine(NativeLayoutEngine* layout_engine);
    
    void MarkNeedsStyleRecalc();
    void MarkNeedsLayout();
    
private:
    RenderLifecycle lifecycle_ = RenderLifecycle::Idle;
    DirtyNodeTracker* dirty_tracker_ = nullptr;
    RenderTreeSynchronizer synchronizer_;
    NativeLayoutEngine* layout_engine_ = nullptr;
    std::shared_ptr<RenderObject> render_tree_;
    
    bool needs_style_recalc_ = false;
    bool needs_layout_ = false;
    float width_ = 0;
    float height_ = 0;
};
```

## Data Models

### 变化记录数据结构

| 字段 | 类型 | 描述 |
|------|------|------|
| type | StructuralChangeType | 变化类型 |
| node | weak_ptr<Node> | 变化的节点 |
| parent | weak_ptr<Node> | 父节点 |
| index | size_t | 在父节点中的索引 |
| new_node | weak_ptr<Node> | 新节点（仅 Replaced） |

### 重建策略阈值

| 条件 | 阈值 | 策略 |
|------|------|------|
| 变化数量 | > 10 | 子树重建 |
| 被替换节点子节点数 | > 5 | 子树重建 |
| 同一父节点变化数 | > 3 | 子树重建 |

## Correctness Properties

*A property is a characteristic or behavior that should hold true across all valid executions of a system-essentially, a formal statement about what the system should do. Properties serve as the bridge between human-readable specifications and machine-verifiable correctness guarantees.*

### Property 1: 变化记录一致性

*For any* DOM operation (add, remove, replace, style change, text change), the DirtyNodeTracker SHALL record the change with correct type, node reference, parent reference, and index.

**Validates: Requirements 1.1, 1.2, 1.3, 1.4, 1.5**

### Property 2: 原子替换操作

*For any* ReplaceChild operation, the DirtyNodeTracker SHALL record it as a single StructuralChange with type Replaced, containing both old_node and new_node references.

**Validates: Requirements 1.3, 2.5**

### Property 3: 变化优化合并

*For any* sequence of changes where a node is added then removed (or vice versa), calling Optimize() SHALL merge or cancel these redundant changes.

**Validates: Requirements 1.7**

### Property 4: 同步完整性

*For any* set of pending changes, after Synchronize() completes, all structural, style, and text changes SHALL be processed and the DirtyNodeTracker SHALL be empty.

**Validates: Requirements 2.1, 2.2, 2.3, 2.6**

### Property 5: 重建策略选择

*For any* set of structural changes, if the count exceeds 10, or a replaced node has >5 children, or same parent has >3 changes, NeedsSubtreeRebuild() SHALL return true.

**Validates: Requirements 5.1, 5.2, 5.3**

### Property 6: 管线阶段顺序

*For any* ProcessFrame() call, the lifecycle stages SHALL transition in order: Idle → RenderTreeSync → StyleRecalc → Layout → Paint → Idle.

**Validates: Requirements 4.1, 4.6**

### Property 7: 向后兼容等价性

*For any* DOM tree, the final render result after using the new deferred synchronization system SHALL be identical to the result using the old immediate update system.

**Validates: Requirements 6.1**

## Error Handling

### 弱引用失效

当 `weak_ptr` 指向的节点已被销毁时：
1. 在处理变化时检查 `lock()` 是否成功
2. 跳过已失效的变化记录
3. 记录警告日志

### 索引越界

当记录的索引超出父节点子节点范围时：
1. 使用 `std::min(index, parent->children.size())` 进行边界保护
2. 记录警告日志
3. 继续处理其他变化

### 循环引用

防止节点成为自己的祖先：
1. 在 RecordNodeAdded 时检查是否形成循环
2. 拒绝形成循环的操作
3. 抛出异常或记录错误

## Testing Strategy

### 单元测试

1. **DirtyNodeTracker 测试**
   - 各类变化的记录
   - HasPendingChanges 状态
   - Clear 和 Optimize 功能

2. **RenderTreeSynchronizer 测试**
   - 结构变化处理
   - 样式变化处理
   - 文本变化处理
   - 重建策略选择

3. **RenderPipeline 测试**
   - 生命周期阶段转换
   - 条件触发逻辑

### 属性测试 (Property-Based Testing)

使用 rapidcheck 库进行属性测试，每个属性测试配置运行 100 次迭代。

```cpp
// 示例：Property 1 测试
RC_GTEST_PROP(DirtyNodeTracker, ChangeRecordingConsistency, ()) {
    // **Feature: incremental-update-system, Property 1: 变化记录一致性**
    // **Validates: Requirements 1.1, 1.2, 1.3, 1.4, 1.5**
    
    auto tracker = DirtyNodeTracker();
    auto node = createRandomNode();
    auto parent = createRandomNode();
    size_t index = *rc::gen::inRange(0, 10);
    
    tracker.RecordNodeAdded(node.get(), parent.get(), index);
    
    RC_ASSERT(tracker.HasPendingChanges());
    const auto& changes = tracker.GetStructuralChanges();
    RC_ASSERT(changes.size() == 1);
    RC_ASSERT(changes[0].type == DirtyNodeTracker::StructuralChangeType::Added);
    RC_ASSERT(changes[0].index == index);
}
```

### 窗口实际渲染测试

根据用户要求，测试部分使用窗口实际渲染情况进行验证：

1. **基础渲染测试**
   - 创建窗口并添加 DOM 元素
   - 验证元素正确渲染

2. **动态更新测试**
   - 动态添加/删除元素
   - 验证窗口更新正确

3. **ReplaceChild 测试**
   - 执行 ReplaceChild 操作
   - 验证新元素正确显示

4. **文本更新测试**
   - 修改文本内容
   - 验证文本正确更新

5. **样式变化测试**
   - 修改元素样式
   - 验证样式变化正确反映

### 集成测试

1. **Preact Demo 测试**
   - 运行现有 Preact demo
   - 验证功能正常

2. **窗口 Resize 测试**
   - 调整窗口大小
   - 验证布局正确更新

## Implementation Notes

### 文件结构

```
core/
├── dom/
│   └── dirty_node_tracker.h/.cpp    # 新增：脏节点追踪器
├── render/
│   ├── render_tree_synchronizer.h/.cpp  # 新增：渲染树同步器
│   └── render_pipeline.h/.cpp       # 新增：渲染管线
└── window/
    └── window.cpp                   # 修改：集成新系统
```

### 修改现有代码

1. **Node 类修改**
   - AppendChild/RemoveChild/ReplaceChild 改为记录变化
   - 不再立即触发渲染树更新

2. **Window 类修改**
   - 集成 RenderPipeline
   - 在渲染循环中调用 ProcessFrame

3. **Document 类修改**
   - 持有 DirtyNodeTracker 实例
   - 提供 GetDirtyTracker() 方法

