# Style 结构统一重构

## 背景

当前布局引擎中每个 `LayoutNode` 存储 7 个 style 结构：
- `style` - 主样式
- `block_container_style` / `block_item_style` - Block 布局样式
- `flexbox_container_style` / `flexbox_item_style` - Flex 布局样式
- `grid_container_style` / `grid_item_style` - Grid 布局样式

这导致：
1. **同步问题**：`UpdateStyle` 需要手动同步所有结构，容易遗漏（已发生过 bug）
2. **数据冗余**：相同属性存储多份
3. **维护成本高**：新增属性需要在多处添加

## 目标

统一使用单一 `Style` 结构，消除冗余和同步问题。

## 需求

### 1. 统一 Flexbox 布局接口
- 1.1 修改 `LayoutFlexboxContainer` 接口，使用 `const Style&` 替代专门的 style 结构
- 1.2 修改 `flex_layout.cpp` 直接从 `Style` 读取属性
- 1.3 移除 `FlexboxContainerStyle` 和 `FlexboxItemStyle` 结构（或标记为废弃）

### 2. 统一 Grid 布局接口
- 2.1 修改 `LayoutGridContainer` 接口
- 2.2 修改 `grid.cpp` 直接从 `Style` 读取属性
- 2.3 Grid 特有属性（grid-template-columns 等）保留在 `Style` 或单独结构中

### 3. 统一 Block 布局接口
- 3.1 修改 `LayoutBlockContainer` 接口
- 3.2 修改 `block_layout.cpp` 直接从 `Style` 读取属性
- 3.3 移除 `BlockContainerStyle` 和 `BlockItemStyle` 结构

### 4. 简化 LayoutNode
- 4.1 从 `LayoutNode` 中移除冗余的 style 结构
- 4.2 只保留 `Style style` 作为唯一样式存储

### 5. 简化样式更新
- 5.1 简化 `UpdateStyle` 函数，只更新 `node->style`
- 5.2 简化 `BuildSubtree` 函数，只设置 `node.style`

### 6. 保持兼容性
- 6.1 确保所有现有布局功能正常工作
- 6.2 确保动态样式更新正常工作
- 6.3 确保增量布局优化正常工作

## 验收标准

1. `LayoutNode` 中只有一个 `Style style` 成员
2. `UpdateStyle` 函数代码量减少 80% 以上
3. 所有组件测试通过（test_checkbox, test_radio, test_flex_center, test_select）
4. 无性能回退

## 参考

- Blink 引擎使用单一 `ComputedStyle` 结构
- 重构计划文档：`docs/refactor_style_unification.md`
