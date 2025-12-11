# 布局树与渲染树统一重构计划

## 背景

### 当前架构（三棵树）
```
DOM Tree (Node/Element)
    ↓ RenderTreeBuilder
Render Tree (RenderObject)
    ↓ NativeLayoutEngine::BuildLayoutTree()
Layout Tree (LayoutNode)  ← 问题根源
```

### 问题
1. LayoutNode 与 RenderObject 需要同步，同步失败导致增量布局 bug
2. 内存冗余（布局信息存两份）
3. ID 映射复杂（render_to_node_）

### 目标架构（两棵树）
```
DOM Tree (Node/Element)
    ↓ RenderTreeBuilder
RenderObject (包含布局信息和布局算法)
```

## 重构策略

**核心思路**：将 LayoutNode 的数据和布局计算逻辑移入 RenderObject

### 保留
- Flexbox/Grid/Block 布局算法（flex_layout.cpp, grid.cpp, block_layout.cpp）
- 布局类型定义（types/）
- IFC 布局（ifc/）

### 删除/重构
- `NativeLayoutEngine::LayoutNode` 结构体
- `NativeLayoutEngine::nodes_` 存储
- `NativeLayoutEngine::render_to_node_` 映射
- `BuildLayoutTree()` / `BuildSubtree()`

### 新增
- `RenderObject` 内嵌布局相关数据
- `RenderObject` 实现 LayoutTree 接口

---

## 详细步骤

### 阶段 1：扩展 RenderObject（预计 2-3 小时）

#### 1.1 添加布局相关字段到 RenderObject

```cpp
// core/render/render_object.h

class RenderObject {
    // ... 现有字段 ...
    
    // === 新增：布局树相关 ===
    
    // 布局样式（从 LayoutNode::style 移入）
    Style layout_style_;
    
    // Block 布局样式
    BlockContainerStyle block_container_style_;
    BlockItemStyle block_item_style_;
    
    // Flex 布局样式
    FlexboxContainerStyle flex_container_style_;
    FlexboxItemStyle flex_item_style_;
    
    // Grid 布局样式
    GridContainerStyle grid_container_style_;
    GridItemStyle grid_item_style_;
    
    // 布局缓存
    Cache layout_cache_;
    
    // 布局输出
    LayoutOutput layout_output_;
    
    // IFC 容器标记
    bool is_ifc_container_ = false;
    
public:
    // 布局样式访问
    Style& GetLayoutStyle() { return layout_style_; }
    const Style& GetLayoutStyle() const { return layout_style_; }
    
    // 更新布局样式（从 ComputedStyle 转换）
    void UpdateLayoutStyle();
    
    // 布局缓存访问
    Cache& GetLayoutCache() { return layout_cache_; }
    
    // 布局输出访问
    LayoutOutput& GetLayoutOutput() { return layout_output_; }
};
```

#### 1.2 实现 UpdateLayoutStyle()

```cpp
// core/render/render_object.cpp

void RenderObject::UpdateLayoutStyle() {
    // 从 ComputedStyle 转换为 layout Style
    // 复用 NativeLayoutEngine::ConvertStyle() 的逻辑
    layout_style_ = ConvertComputedStyleToLayoutStyle(computed_style_);
    
    // 更新各种布局样式
    UpdateBlockStyles();
    UpdateFlexStyles();
    UpdateGridStyles();
    
    // 检查是否为 IFC 容器
    is_ifc_container_ = CheckIsIFCContainer();
}
```

---

### 阶段 2：重构 NativeLayoutEngine（预计 3-4 小时）

#### 2.1 修改 NativeLayoutEngine 使用 RenderObject 而非 LayoutNode

```cpp
// core/layout/native_layout_engine.h

class NativeLayoutEngine : public LayoutBlockContainer {
public:
    // 简化后的接口
    void ComputeLayout(RenderObject* root, float width, float height);
    
    // LayoutTree 接口 - 直接操作 RenderObject
    size_t ChildCount(RenderObject* node) const;
    RenderObject* GetChild(RenderObject* node, size_t index) const;
    Cache& GetCache(RenderObject* node);
    void SetUnroundedLayout(RenderObject* node, const Layout& layout);
    const Layout& GetLayout(RenderObject* node) const;
    
private:
    // 删除：
    // std::unordered_map<NodeId, LayoutNode> nodes_;
    // std::unordered_map<RenderObject*, NodeId> render_to_node_;
    
    // 布局计算方法（保留，但参数从 NodeId 改为 RenderObject*）
    LayoutOutput ComputeNodeLayout(RenderObject* node, const LayoutInput& inputs);
    LayoutOutput ComputeBlockLayout(RenderObject* node, const LayoutInput& inputs);
    LayoutOutput ComputeFlexLayout(RenderObject* node, const LayoutInput& inputs);
    LayoutOutput ComputeGridLayout(RenderObject* node, const LayoutInput& inputs);
    LayoutOutput ComputeIFCLayout(RenderObject* node, const LayoutInput& inputs);
};
```

#### 2.2 更新布局算法接口

flex_layout.cpp, grid.cpp, block_layout.cpp 中的 LayoutTree 接口需要适配。

**方案 A**：修改 LayoutTree 接口使用 RenderObject*
**方案 B**：创建一个轻量适配器

推荐方案 B（改动更小）：

```cpp
// 适配器：让现有布局算法能操作 RenderObject
class RenderObjectLayoutAdapter : public LayoutBlockContainer {
public:
    RenderObjectLayoutAdapter(NativeLayoutEngine& engine, RenderObject* root)
        : engine_(engine), root_(root) {}
    
    size_t ChildCount(NodeId node) const override {
        auto* obj = IdToRenderObject(node);
        return obj ? obj->GetChildren().size() : 0;
    }
    
    NodeId GetChildId(NodeId node, size_t index) const override {
        auto* obj = IdToRenderObject(node);
        if (obj && index < obj->GetChildren().size()) {
            return RenderObjectToId(obj->GetChildren()[index].get());
        }
        return 0;
    }
    
    // ... 其他接口实现 ...
    
private:
    // 使用 RenderObject 指针作为 ID（简单直接）
    NodeId RenderObjectToId(RenderObject* obj) const {
        return reinterpret_cast<NodeId>(obj);
    }
    
    RenderObject* IdToRenderObject(NodeId id) const {
        return reinterpret_cast<RenderObject*>(id);
    }
};
```

---

### 阶段 3：更新 Window 渲染流程（预计 1-2 小时）

#### 3.1 简化 Render() 方法

```cpp
// core/window/window.cpp

void Window::Render() {
    // ... 前置检查 ...
    
    // 布局（直接在 RenderObject 上计算）
    if (root_render_object_->NeedsLayout()) {
        layout_engine_->ComputeLayout(root_render_object_.get(), width, height);
    }
    
    // 绘制
    root_render_object_->Paint(canvas);
}
```

#### 3.2 删除冗余代码

- 删除 `BuildLayoutTree()` 调用
- 删除 `GetLayoutInfo()` 调用（布局结果已直接在 RenderObject 中）
- 简化 `LayoutDirtySubtree()`

---

### 阶段 4：增量布局实现（预计 2-3 小时）

#### 4.1 基于 RenderObject 的增量布局

```cpp
bool NativeLayoutEngine::ComputeIncrementalLayout(RenderObject* root, float width, float height) {
    if (!root) return false;
    
    // 收集脏节点
    std::vector<RenderObject*> dirty_nodes;
    CollectDirtyNodes(root, dirty_nodes);
    
    if (dirty_nodes.empty()) return false;
    
    // 对每个脏节点重新计算布局
    for (auto* node : dirty_nodes) {
        // 找到需要重新布局的最高祖先
        auto* layout_root = FindLayoutRoot(node);
        
        // 重新计算该子树
        RecomputeSubtree(layout_root, width, height);
    }
    
    return true;
}
```

---

### 阶段 5：测试和清理（预计 1-2 小时）

1. 运行 preact_todo_app 测试基础功能
2. 测试增量布局（添加/删除元素）
3. 测试 hover/focus 状态变化
4. 删除废弃代码
5. 更新注释和文档

---

## 文件修改清单

| 文件 | 修改类型 | 说明 |
|------|---------|------|
| core/render/render_object.h | 修改 | 添加布局相关字段 |
| core/render/render_object.cpp | 修改 | 实现 UpdateLayoutStyle() |
| core/layout/native_layout_engine.h | 重构 | 删除 LayoutNode，简化接口 |
| core/layout/native_layout_engine.cpp | 重构 | 核心重构，~1500 行 |
| core/layout/flex_layout.cpp | 小改 | 适配新接口 |
| core/layout/block_layout.cpp | 小改 | 适配新接口 |
| core/layout/grid/grid.cpp | 小改 | 适配新接口 |
| core/window/window.cpp | 简化 | 删除冗余调用 |

---

## 风险和回退策略

### 风险
1. 布局算法适配可能有遗漏
2. 某些边界情况未覆盖

### 回退策略
- 在新分支开发，主分支不受影响
- 每个阶段完成后进行测试
- 如果问题严重，可随时 checkout 回主分支

---

## 时间估计

| 阶段 | 预计时间 |
|------|---------|
| 阶段 1：扩展 RenderObject | 2-3 小时 |
| 阶段 2：重构 NativeLayoutEngine | 3-4 小时 |
| 阶段 3：更新 Window 渲染流程 | 1-2 小时 |
| 阶段 4：增量布局实现 | 2-3 小时 |
| 阶段 5：测试和清理 | 1-2 小时 |
| **总计** | **9-14 小时（1-2 天）** |

---

## 开始执行

确认计划后，从阶段 1 开始执行。

