# Style 结构统一 - 技术设计

## 实现状态

> **完成度: 100%** (2024-12-22 更新)

| Phase | 状态 | 说明 |
|-------|------|------|
| Phase 1: Flexbox 统一 | ✅ 完成 | 接口和实现已迁移到统一 Style |
| Phase 2: Grid 统一 | ✅ 完成 | 接口和实现已迁移，Grid 特有数据单独存储 |
| Phase 3: Block 统一 | ✅ 完成 | 接口和实现已迁移到统一 Style |
| Phase 4: 清理 | ✅ 完成 | 废弃代码已移除 |

---

## 原始架构（已废弃）

```
┌─────────────────────────────────────────────────────────────┐
│                      LayoutNode                              │
├─────────────────────────────────────────────────────────────┤
│  Style style                    ← 主样式（完整）             │
│  BlockContainerStyle            ← 冗余复制                   │
│  BlockItemStyle                 ← 冗余复制                   │
│  FlexboxContainerStyle          ← 冗余复制                   │
│  FlexboxItemStyle               ← 冗余复制                   │
│  GridContainerStyle             ← 冗余复制                   │
│  GridItemStyle                  ← 冗余复制                   │
└─────────────────────────────────────────────────────────────┘
                    ↓ UpdateStyle 需要同步 7 处
```

## 当前架构（已实现）

```
┌─────────────────────────────────────────────────────────────┐
│                      LayoutNode                              │
├─────────────────────────────────────────────────────────────┤
│  Style style                    ← 唯一样式存储 ✅            │
│  GridContainerStyle             ← Grid 特有数据（模板等）    │
│  GridItemStyle                  ← Grid 项目特有数据          │
└─────────────────────────────────────────────────────────────┘
                    ↓ UpdateStyle 只需更新 1 处 ✅
```

## 详细设计

### 1. Style 结构扩展 ✅ 已完成

将所有布局相关属性统一到 `Style` 结构：

```cpp
// types/style.h
struct Style : public CoreStyle {
    // === 通用对齐属性 ===
    std::optional<AlignItems> align_items;
    std::optional<AlignSelf> align_self;
    std::optional<AlignContent> align_content;
    std::optional<JustifyContent> justify_content;
    std::optional<AlignItems> justify_items;
    std::optional<AlignSelf> justify_self;
    Size<LengthPercentage> gap;

    // === Flexbox 属性 ===
    FlexDirection flex_direction;
    FlexWrap flex_wrap;
    Dimension flex_basis;
    float flex_grow;
    float flex_shrink;
    int order;

    // === Grid 属性 ===
    GridAutoFlow grid_auto_flow;
    // grid-template-columns/rows 较复杂，保留单独存储
    
    // === Block 属性 ===
    BlockTextAlign text_align;
};
```

### 2. 布局接口统一 ✅ 已完成

#### 2.1 Flexbox 接口 ✅

```cpp
// flex_layout.h
class LayoutFlexboxContainer : public LayoutTree {
public:
    // Before: 返回专门的 style 结构
    // virtual const FlexboxContainerStyle& GetFlexboxContainerStyle(NodeId) const = 0;
    // virtual const FlexboxItemStyle& GetFlexboxChildStyle(NodeId) const = 0;
    
    // After: 直接返回 Style
    virtual const Style& GetStyle(NodeId node) const = 0;
};
```

#### 2.2 Grid 接口 ✅

```cpp
// grid/grid.h
class LayoutGridContainer : public LayoutTree {
public:
    virtual const Style& GetStyle(NodeId node) const = 0;
    
    // Grid 特有数据单独获取
    virtual const GridTemplateData& GetGridTemplateData(NodeId node) const = 0;
};
```

#### 2.3 Block 接口 ✅

```cpp
// block_layout.h
class LayoutBlockContainer : public LayoutTree {
public:
    virtual const Style& GetStyle(NodeId node) const = 0;
    virtual const Style& GetChildStyle(NodeId node) const = 0;
};
```

### 3. NativeLayoutEngine 修改 ✅ 已完成

#### 3.1 LayoutNode 简化 ✅

```cpp
struct LayoutNode {
    NodeId id = 0;
    RenderObject* render_obj = nullptr;
    NodeId parent = 0;
    std::vector<NodeId> children;

    // 唯一样式存储
    Style style;
    
    // Grid 特有数据（仅 Grid 容器需要）
    GridTemplateData grid_template_data;

    // 其他字段保持不变...
    Cache cache;
    LayoutOutput output;
    Layout layout;
    // ...
};
```

#### 3.2 UpdateStyle 简化 ✅

```cpp
void NativeLayoutEngine::UpdateStyle(RenderObject* render_obj, const ComputedStyle& computed) {
    auto it = render_to_node_.find(render_obj);
    if (it == render_to_node_.end()) return;
    
    LayoutNode* node = GetNode(it->second);
    if (!node) return;

    Style new_style = ConvertStyle(computed);
    
    // 检查布局相关属性是否变化
    bool layout_changed = CheckLayoutChanged(node->style, new_style);
    
    // 只需更新一处！
    node->style = new_style;
    
    // Grid 特有数据单独更新
    if (new_style.display == Display::Grid) {
        node->grid_template_data = ParseGridTemplateData(computed);
    }
    
    if (layout_changed) {
        PropagateLayoutDirty(it->second, DetermineLayoutScope(node));
    }
}
```

#### 3.3 BuildSubtree 简化 ✅

```cpp
NodeId NativeLayoutEngine::CreateNode(RenderObject* render_obj, NodeId parent_id) {
    LayoutNode node;
    node.id = next_node_id_++;
    node.render_obj = render_obj;
    node.parent = parent_id;
    
    // 只需设置一处！
    node.style = ConvertStyle(render_obj->GetComputedStyle());
    
    // Grid 特有数据
    if (node.style.display == Display::Grid) {
        node.grid_template_data = ParseGridTemplateData(render_obj->GetComputedStyle());
    }
    
    nodes_[node.id] = std::move(node);
    return node.id;
}
```

### 4. 布局算法修改 ✅ 已完成

#### 4.1 Flex 布局 ✅

```cpp
// flex_layout.cpp
static std::vector<FlexItem> GenerateItemList(...) {
    for (size_t i = 0; i < child_count; ++i) {
        NodeId child = tree.GetChildId(node, i);
        
        // Before: const auto& child_style = tree.GetFlexboxChildStyle(child);
        // After:
        const auto& child_style = tree.GetStyle(child);
        
        FlexItem item;
        item.size = MaybeResolve(child_style.size, ...);
        item.flex_grow = child_style.flex_grow;
        item.flex_shrink = child_style.flex_shrink;
        item.align_self = child_style.align_self.value_or(
            static_cast<AlignSelf>(constants.align_items)
        );
        // ...
    }
}
```

### 5. 迁移策略 ✅ 已完成

采用渐进式迁移，每个阶段独立可测试：

```
Phase 1: Flexbox 统一 ✅
  ├── 修改 flex_layout.h 接口 ✅
  ├── 修改 flex_layout.cpp 实现 ✅
  ├── 修改 NativeLayoutEngine 适配 ✅
  └── 测试验证 ✅

Phase 2: Grid 统一 ✅
  ├── 修改 grid.h 接口 ✅
  ├── 修改 grid.cpp 实现 ✅
  └── 测试验证 ✅

Phase 3: Block 统一 ✅
  ├── 修改 block_layout.h 接口 ✅
  ├── 修改 block_layout.cpp 实现 ✅
  └── 测试验证 ✅

Phase 4: 清理 ✅
  ├── 移除 LayoutNode 中的冗余字段 ✅
  ├── 移除废弃的 style 结构定义 ✅
  ├── 移除废弃的 getter 方法 ✅
  └── 最终测试 ✅
```

## 风险与缓解

| 风险 | 缓解措施 |
|------|----------|
| 引入布局 bug | 每个 Phase 后运行完整测试 |
| 类型不匹配 | 使用 `.value_or()` 处理 optional |
| 性能回退 | 基准测试对比 |

## 文件修改清单

| 文件 | 修改类型 | Phase | 状态 |
|------|----------|-------|------|
| `types/style.h` | 扩展 Style 结构 | 1 | ✅ |
| `flex_layout.h` | 修改接口 | 1 | ✅ |
| `flex_layout.cpp` | 使用新接口 | 1 | ✅ |
| `grid/grid.h` | 修改接口 | 2 | ✅ |
| `grid/grid.cpp` | 使用新接口 | 2 | ✅ |
| `block_layout.h` | 修改接口 | 3 | ✅ |
| `block_layout.cpp` | 使用新接口 | 3 | ✅ |
| `native_layout_engine.h` | 简化 LayoutNode | 4 | ✅ |
| `native_layout_engine.cpp` | 简化 UpdateStyle/BuildSubtree | 4 | ✅ |
