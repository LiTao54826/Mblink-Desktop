# Style 结构统一重构计划

## 问题分析

### 当前设计
每个 `LayoutNode` 存储 7 个 style 结构：
```cpp
struct LayoutNode {
    Style style;                           // 主样式
    BlockContainerStyle block_container_style;
    BlockItemStyle block_item_style;
    FlexboxContainerStyle flexbox_container_style;
    FlexboxItemStyle flexbox_item_style;
    GridContainerStyle grid_container_style;
    GridItemStyle grid_item_style;
};
```

### 问题
1. **数据冗余**：相同属性存储多份（如 size, padding, margin 等）
2. **同步风险**：`UpdateStyle` 需要手动同步所有结构，容易遗漏
3. **维护成本**：新增属性需要在多处添加

### 根因
存在两套接口设计：
- `traits.h` 中 `LayoutFlexContainer` 使用 `const Style&`（延迟计算）
- `flex_layout.h` 中 `LayoutFlexboxContainer` 使用专门的 `FlexboxContainerStyle`（预计算）

## 重构方案

### 目标
统一使用 `Style` 结构，布局算法直接从 `node->style` 读取属性。

### 步骤

#### Phase 1: 统一 Flexbox 接口
1. 修改 `flex_layout.h` 中的 `LayoutFlexboxContainer` 接口：
   ```cpp
   // Before
   virtual const FlexboxContainerStyle& GetFlexboxContainerStyle(NodeId node) const = 0;
   virtual const FlexboxItemStyle& GetFlexboxChildStyle(NodeId node) const = 0;
   
   // After
   virtual const Style& GetContainerStyle(NodeId node) const = 0;
   virtual const Style& GetChildStyle(NodeId node) const = 0;
   ```

2. 修改 `flex_layout.cpp` 中的布局算法，直接从 `Style` 读取属性：
   ```cpp
   // Before
   const auto& child_style = tree.GetFlexboxChildStyle(child);
   float flex_grow = child_style.flex_grow;
   
   // After
   const auto& child_style = tree.GetChildStyle(child);
   float flex_grow = child_style.flex_grow;
   ```

3. 修改 `NativeLayoutEngine` 实现：
   ```cpp
   // Before
   const FlexboxItemStyle& GetFlexboxChildStyle(NodeId node) const {
       return nodes_[node].flexbox_item_style;
   }
   
   // After
   const Style& GetChildStyle(NodeId node) const {
       return nodes_[node].style;
   }
   ```

#### Phase 2: 统一 Grid 接口
同样的方式处理 `grid_layout.h` 和 `GridContainerStyle`/`GridItemStyle`。

#### Phase 3: 统一 Block 接口
处理 `block_layout.h` 和 `BlockContainerStyle`/`BlockItemStyle`。

#### Phase 4: 清理冗余结构
1. 从 `LayoutNode` 中移除冗余的 style 结构：
   ```cpp
   struct LayoutNode {
       Style style;  // 唯一的样式存储
       // 移除: block_container_style, block_item_style, 
       //       flexbox_container_style, flexbox_item_style,
       //       grid_container_style, grid_item_style
   };
   ```

2. 简化 `UpdateStyle`：
   ```cpp
   void UpdateStyle(RenderObject* render_obj, const ComputedStyle& style) {
       node->style = ConvertStyle(style);  // 只需更新一处
       // 不再需要同步其他结构
   }
   ```

3. 简化 `BuildSubtree`：
   ```cpp
   void BuildSubtree(RenderObject* render_obj, NodeId parent_id) {
       node.style = ConvertStyle(computed);  // 只需设置一处
       // 不再需要复制到其他结构
   }
   ```

### 特殊处理

#### Grid 特有属性
Grid 有一些特有属性（如 `grid_template_columns`）需要特殊处理：
- 方案 A：将这些属性添加到 `Style` 结构
- 方案 B：保留 `GridContainerStyle` 但只存储 Grid 特有属性

推荐方案 B，因为 Grid 属性较复杂且不常用。

#### 类型转换
某些属性类型不同（如 `align_items` 在 `Style` 中是 `optional`，在 `FlexboxContainerStyle` 中不是）：
- 在布局算法中使用 `.value_or(default)` 处理

### 影响范围

| 文件 | 修改内容 |
|------|----------|
| `types/traits.h` | 统一接口定义 |
| `flex_layout.h` | 移除 FlexboxContainerStyle/FlexboxItemStyle |
| `flex_layout.cpp` | 使用 Style 替代专门结构 |
| `grid/grid.h` | 简化 GridContainerStyle |
| `grid/grid.cpp` | 使用 Style 替代专门结构 |
| `block_layout.h` | 移除 BlockContainerStyle/BlockItemStyle |
| `block_layout.cpp` | 使用 Style 替代专门结构 |
| `native_layout_engine.h` | 简化 LayoutNode |
| `native_layout_engine.cpp` | 简化 UpdateStyle/BuildSubtree |

### 预期收益
1. **消除同步问题**：样式只存储一份，不存在同步遗漏
2. **减少内存**：每个节点减少约 6 个结构的内存
3. **简化代码**：UpdateStyle 从 ~100 行减少到 ~10 行
4. **降低维护成本**：新增属性只需修改一处

### 风险
1. 需要修改多个布局算法文件
2. 可能引入新的 bug，需要充分测试

### 测试计划
1. 运行所有现有组件测试
2. 特别测试动态样式变化场景
3. 测试 Flex/Grid/Block 混合布局
