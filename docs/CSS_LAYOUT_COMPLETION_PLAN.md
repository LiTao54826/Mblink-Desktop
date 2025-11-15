# MBink CSS 和布局完善计划

## 📊 当前状态分析

### ✅ 已实现的功能

#### 1. CSS 属性解析（部分）
- ✅ `display` (block, inline, inline-block)
- ✅ `width`, `height`, `min-width`, `max-width`, `min-height`, `max-height`
- ✅ `margin`, `padding` (四个方向)
- ✅ `border` (width, style, color, radius)
- ✅ `background-color`, `background-image`, `background-repeat`, `background-size`
- ✅ `color`, `font-family`, `font-size`, `font-weight`, `font-style`
- ✅ `text-align`, `text-decoration`, `line-height`
- ✅ `box-shadow`, `text-shadow`
- ✅ `opacity`
- ✅ `transition`
- ✅ `filter`, `backdrop-filter`
- ✅ CSS 变量 (`--custom-property`, `var()`)
- ✅ 线性渐变和径向渐变

#### 2. 布局系统（简单实现）
- ✅ 基本的块级布局（垂直堆叠）
- ✅ 基本的内联布局（水平排列）
- ✅ 盒模型计算（content, padding, border, margin）

#### 3. 样式解析
- ✅ 内联样式解析 (`style` 属性)
- ✅ 样式继承
- ✅ 伪类样式 (`:hover`, `:active`, `:focus-visible`, `:disabled`)

### ❌ 缺失的关键功能

#### 1. **Flexbox 布局（完全缺失）**
虽然文档声称支持 Flexbox，但实际上：
- ❌ `layout_engine.cpp` 和 `layout_engine.h` 几乎是空的（只有 TODO）
- ❌ Yoga 布局引擎没有被集成到渲染管线
- ❌ `ComputedStyle` 缺少所有 Flexbox 字段
- ❌ `StyleResolver` 不解析 Flexbox 属性
- ❌ `RenderObject` 不使用 Yoga 进行布局

**缺失的 Flexbox 属性：**
- `display: flex` / `display: inline-flex`
- `flex-direction` (row, row-reverse, column, column-reverse)
- `flex-wrap` (nowrap, wrap, wrap-reverse)
- `flex-flow` (shorthand)
- `justify-content` (flex-start, flex-end, center, space-between, space-around, space-evenly)
- `align-items` (flex-start, flex-end, center, baseline, stretch)
- `align-content` (flex-start, flex-end, center, space-between, space-around, stretch)
- `align-self` (auto, flex-start, flex-end, center, baseline, stretch)
- `flex` (shorthand)
- `flex-grow`
- `flex-shrink`
- `flex-basis`
- `order`
- `gap`, `row-gap`, `column-gap`

#### 2. **Grid 布局（完全缺失）**
- ❌ `display: grid` / `display: inline-grid`
- ❌ `grid-template-columns`, `grid-template-rows`
- ❌ `grid-template-areas`
- ❌ `grid-column`, `grid-row`
- ❌ `grid-gap`, `column-gap`, `row-gap`
- ❌ `justify-items`, `align-items`
- ❌ `justify-content`, `align-content`
- ❌ `grid-auto-flow`, `grid-auto-columns`, `grid-auto-rows`

#### 3. **定位（部分缺失）**
- ⚠️ `position` 字段存在但未实现
- ❌ `position: static` / `relative` / `absolute` / `fixed` / `sticky`
- ❌ `top`, `right`, `bottom`, `left`
- ❌ `z-index`

#### 4. **其他重要 CSS 属性**
- ❌ `overflow` (虽然字段存在，但未实现裁剪)
- ❌ `overflow-x`, `overflow-y`
- ❌ `visibility` (visible, hidden, collapse)
- ❌ `cursor`
- ❌ `transform` (虽然有 transform.h，但未集成到样式系统)
- ❌ `transform-origin`
- ❌ `vertical-align`
- ❌ `white-space` (normal, nowrap, pre, pre-wrap, pre-line)
- ❌ `word-wrap` / `overflow-wrap`
- ❌ `text-overflow` (clip, ellipsis)
- ❌ `list-style` (type, position, image)
- ❌ `table` 相关属性 (border-collapse, border-spacing, etc.)

#### 5. **外部样式表（完全缺失）**
- ❌ `<style>` 标签解析
- ❌ `<link rel="stylesheet">` 支持
- ❌ CSS 选择器匹配
- ❌ CSS 级联和优先级计算
- ❌ 媒体查询

#### 6. **响应式单位（部分缺失）**
- ✅ `px`, `%` 已支持
- ❌ `em`, `rem`
- ❌ `vw`, `vh`, `vmin`, `vmax`
- ❌ `ch`, `ex`

---

## 🎯 修复计划

### 阶段 1：修复当前布局问题（紧急）

**目标**：让 `window_demo` 的 todo list 正确显示

#### 任务 1.1：添加 Flexbox 字段到 ComputedStyle
- [ ] 在 `ComputedStyle` 结构体中添加 Flexbox 相关字段
- [ ] 添加默认值初始化

#### 任务 1.2：实现 Flexbox 属性解析
- [ ] 在 `StyleResolver::ParseStyleProperty` 中添加 Flexbox 属性解析
- [ ] 实现 `ParseFlexDirection`, `ParseJustifyContent`, `ParseAlignItems` 等辅助函数

#### 任务 1.3：集成 Yoga 布局引擎（简化版）
- [ ] 在 `RenderObject` 中添加 Yoga 节点支持
- [ ] 实现 `ApplyYogaStyle` 方法，将 `ComputedStyle` 转换为 Yoga 属性
- [ ] 在 `Layout` 方法中使用 Yoga 计算布局
- [ ] 处理 `display: flex` 的元素

#### 任务 1.4：测试和验证
- [ ] 测试 todo list 布局是否正确
- [ ] 测试其他 Flexbox 场景

**预计时间**：2-3 天

---

### 阶段 2：完善 Flexbox 支持（重要）

**目标**：完整实现 Flexbox 规范

#### 任务 2.1：完整的 Flexbox 属性
- [ ] 实现所有 Flexbox 属性解析
- [ ] 实现 `flex` shorthand 解析
- [ ] 实现 `flex-flow` shorthand 解析
- [ ] 实现 `gap` 属性

#### 任务 2.2：嵌套 Flexbox
- [ ] 测试嵌套 Flexbox 容器
- [ ] 处理百分比尺寸
- [ ] 处理 `auto` 尺寸

#### 任务 2.3：边缘情况
- [ ] 处理空容器
- [ ] 处理单个子元素
- [ ] 处理溢出情况

**预计时间**：3-4 天

---

### 阶段 3：实现定位系统（重要）

**目标**：支持 `position` 属性

#### 任务 3.1：基础定位
- [ ] 实现 `position: static`（默认）
- [ ] 实现 `position: relative`
- [ ] 实现 `position: absolute`
- [ ] 实现 `position: fixed`

#### 任务 3.2：偏移属性
- [ ] 解析 `top`, `right`, `bottom`, `left`
- [ ] 在布局中应用偏移

#### 任务 3.3：层叠顺序
- [ ] 实现 `z-index`
- [ ] 实现层叠上下文

**预计时间**：3-4 天

---

### 阶段 4：外部样式表支持（重要）

**目标**：支持 `<style>` 和 `<link>` 标签

#### 任务 4.1：CSS 解析器集成
- [ ] 集成 Lexbor CSS 解析器
- [ ] 解析 `<style>` 标签内容
- [ ] 解析外部 CSS 文件

#### 任务 4.2：选择器匹配
- [ ] 实现选择器匹配引擎
- [ ] 支持基本选择器（标签、类、ID）
- [ ] 支持组合选择器（后代、子、相邻）
- [ ] 支持伪类选择器

#### 任务 4.3：级联和优先级
- [ ] 实现 CSS 级联算法
- [ ] 实现优先级计算（specificity）
- [ ] 处理 `!important`

**预计时间**：5-7 天

---

### 阶段 5：其他重要属性（中等优先级）

#### 任务 5.1：溢出处理
- [ ] 实现 `overflow: visible/hidden/scroll/auto`
- [ ] 实现内容裁剪
- [ ] 实现滚动条（如果需要）

#### 任务 5.2：文本处理
- [ ] 实现 `white-space`
- [ ] 实现 `word-wrap` / `overflow-wrap`
- [ ] 实现 `text-overflow`

#### 任务 5.3：Transform
- [ ] 将现有的 `transform.h` 集成到样式系统
- [ ] 解析 `transform` 属性
- [ ] 在渲染时应用变换

**预计时间**：4-5 天

---

### 阶段 6：Grid 布局（低优先级）

**目标**：支持 CSS Grid

#### 任务 6.1：Grid 基础
- [ ] 添加 Grid 相关字段到 `ComputedStyle`
- [ ] 解析 Grid 属性
- [ ] 实现基本的 Grid 布局算法

#### 任务 6.2：Grid 高级特性
- [ ] 实现 `grid-template-areas`
- [ ] 实现 `grid-auto-flow`
- [ ] 实现 `gap`

**预计时间**：7-10 天

---

## 📋 详细实现清单

### ComputedStyle 需要添加的字段

```cpp
struct ComputedStyle {
    // ... 现有字段 ...
    
    // Flexbox 属性
    std::string flex_direction = "row";  // row, row-reverse, column, column-reverse
    std::string flex_wrap = "nowrap";    // nowrap, wrap, wrap-reverse
    std::string justify_content = "flex-start";  // flex-start, flex-end, center, space-between, space-around, space-evenly
    std::string align_items = "stretch";  // flex-start, flex-end, center, baseline, stretch
    std::string align_content = "stretch";  // flex-start, flex-end, center, space-between, space-around, stretch
    std::string align_self = "auto";  // auto, flex-start, flex-end, center, baseline, stretch
    float flex_grow = 0.0f;
    float flex_shrink = 1.0f;
    CSSLength flex_basis;  // auto, content, or length
    int order = 0;
    CSSLength gap;  // gap between flex items
    CSSLength row_gap;
    CSSLength column_gap;
    
    // 定位属性
    // std::string position;  // 已存在但未使用
    CSSLength top;
    CSSLength right;
    CSSLength bottom;
    CSSLength left;
    int z_index = 0;
    
    // Grid 属性（未来）
    // ...
    
    // 其他属性
    std::string visibility = "visible";  // visible, hidden, collapse
    std::string white_space = "normal";  // normal, nowrap, pre, pre-wrap, pre-line
    std::string word_wrap = "normal";  // normal, break-word
    std::string text_overflow = "clip";  // clip, ellipsis
    std::string vertical_align = "baseline";  // baseline, top, middle, bottom, etc.
    std::string cursor = "default";
    
    // Transform
    std::string transform;
    std::string transform_origin = "50% 50%";
};
```

### StyleResolver 需要添加的解析函数

```cpp
// 在 ParseStyleProperty 中添加：
else if (property == "flex-direction") {
    style.flex_direction = resolved_value;
}
else if (property == "flex-wrap") {
    style.flex_wrap = resolved_value;
}
else if (property == "justify-content") {
    style.justify_content = resolved_value;
}
else if (property == "align-items") {
    style.align_items = resolved_value;
}
// ... 等等
```

---

## 🚀 立即开始

**第一步**：修复 todo list 布局问题（阶段 1）

这是最紧急的任务，因为用户已经报告了布局问题。我们需要：
1. 添加 Flexbox 字段到 `ComputedStyle`
2. 解析内联样式中的 Flexbox 属性
3. 集成 Yoga 进行布局计算
4. 测试 todo list 是否正确显示

---

## 📝 注意事项

1. **向后兼容**：确保现有的布局不被破坏
2. **性能**：Yoga 布局计算可能比较慢，需要优化
3. **测试**：每个阶段都需要充分测试
4. **文档**：更新文档以反映实际实现的功能
5. **渐进式**：一步一步来，不要一次性改太多

---

## 🎯 成功标准

### 阶段 1 成功标准
- ✅ todo list 的 checkbox、文本和按钮正确对齐
- ✅ `display: flex` 和 `justify-content: space-between` 生效
- ✅ 现有的非 Flexbox 布局不受影响

### 最终成功标准
- ✅ 所有常用 CSS 属性都被支持
- ✅ Flexbox 布局完全符合规范
- ✅ 外部样式表正常工作
- ✅ 定位系统正常工作
- ✅ 性能可接受（60fps）

