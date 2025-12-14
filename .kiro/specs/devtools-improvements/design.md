# DevTools Improvements Design Document

## Overview

本设计文档描述 MBink DevTools 的三项改进：计算样式显示真实值、盒模型完整数值显示、以及盒模型鼠标悬停高亮效果。这些改进将增强 DevTools 的实用性，使开发者能够更准确地查看元素的样式和布局信息。

### 设计目标

1. **准确性**: 显示真实的计算样式值，而非占位符
2. **完整性**: 显示盒模型的所有数值（margin、border、padding、content）
3. **交互性**: 提供鼠标悬停高亮反馈，增强用户体验
4. **性能**: 最小化对渲染性能的影响

## Architecture

### 修改范围

```
core/devtools/styles/
├── computed_styles_view.h      # 添加 RenderObject 查找辅助方法
├── computed_styles_view.cpp    # 修改 RefreshStyles() 获取真实值
├── box_model_view.h            # 已有 HandleMouseMove 和 HitTest 声明
├── box_model_view.cpp          # 实现 border/padding 数值显示和悬停高亮
└── styles_panel.cpp            # 传递鼠标事件到 BoxModelView
```

### 数据流

```
┌─────────────────┐     GetComputedStyle()    ┌──────────────────┐
│  RenderObject   │ ◀─────────────────────────│ ComputedStylesView│
│  (渲染树节点)    │                           │                  │
└─────────────────┘                           └──────────────────┘
        │
        │ GetLayoutInfo()
        ▼
┌─────────────────┐     GetBoxModelData()     ┌──────────────────┐
│   LayoutInfo    │ ◀─────────────────────────│   BoxModelView   │
│  (布局信息)      │                           │                  │
└─────────────────┘                           └──────────────────┘
```

## Components and Interfaces

### 1. ComputedStylesView 改进

修改 `RefreshStyles()` 方法，从 RenderObject 获取真实的计算样式值。

```cpp
class ComputedStylesView {
public:
    // 现有接口保持不变
    void SetElement(std::shared_ptr<Element> element);
    void Render(SkCanvas* canvas, float x, float y, float width, float height);
    std::vector<ComputedStyleCategory> GetComputedStyles() const;

private:
    // 新增：查找元素对应的 RenderObject
    std::shared_ptr<RenderObject> FindRenderObject(std::shared_ptr<Element> element) const;
    
    // 新增：从 ComputedStyle 获取属性值的字符串表示
    std::string GetPropertyValue(const ComputedStyle& style, const std::string& property_name) const;
    
    // 新增：判断属性值是否为默认值
    bool IsDefaultValue(const std::string& property_name, const std::string& value) const;
    
    // 修改：RefreshStyles 将调用上述方法获取真实值
    void RefreshStyles();
};
```

### 2. BoxModelView 改进

完善数值显示和悬停高亮功能。

```cpp
class BoxModelView {
public:
    // 现有接口
    void SetElement(std::shared_ptr<Element> element);
    BoxModelData GetBoxModelData() const;
    void Render(SkCanvas* canvas, float x, float y, float width, float height);
    
    // 悬停处理（已声明，需实现）
    bool HandleMouseMove(int x, int y);
    BoxAreaType GetHoveredArea() const;
    void ResetHover();

private:
    BoxAreaType hovered_area_ = BoxAreaType::None;
    
    // 渲染位置信息（用于命中测试）
    float diagram_x_ = 0;
    float diagram_y_ = 0;
    float diagram_width_ = 300;
    float diagram_height_ = 200;
    float margin_inset_ = 0;
    float border_inset_ = 0;
    float padding_inset_ = 0;
    float content_inset_ = 0;
    
    // 新增：命中测试实现
    BoxAreaType HitTest(int x, int y) const;
    
    // 修改：渲染时根据悬停状态调整颜色
    void RenderBoxDiagram(SkCanvas* canvas, float x, float y, float width, float height,
                          const BoxModelData& data);
    
    // 新增：渲染 border 和 padding 数值
    void RenderBorderValues(SkCanvas* canvas, const BoxModelData& data);
    void RenderPaddingValues(SkCanvas* canvas, const BoxModelData& data);
};
```

### 3. StylesPanel 事件传递

修改 StylesPanel 将鼠标事件传递给 BoxModelView。

```cpp
class StylesPanel {
public:
    // 修改：处理鼠标事件时传递给 BoxModelView
    bool HandleMouseEvent(const MouseEvent& event);
    
private:
    // 当 BoxModel 标签页激活时，传递鼠标移动事件
    void OnMouseMove(int x, int y);
};
```

## Data Models

### ComputedStyle 属性映射

创建属性名到 ComputedStyle 字段的映射表：

```cpp
// 属性名 -> 获取值的函数
const std::unordered_map<std::string, std::function<std::string(const ComputedStyle&)>> PROPERTY_GETTERS = {
    // Layout
    {"display", [](const ComputedStyle& s) { return RenderObjectTypeToString(s.display); }},
    {"position", [](const ComputedStyle& s) { return s.position.empty() ? "static" : s.position; }},
    {"top", [](const ComputedStyle& s) { return s.top.ToString(); }},
    {"right", [](const ComputedStyle& s) { return s.right.ToString(); }},
    {"bottom", [](const ComputedStyle& s) { return s.bottom.ToString(); }},
    {"left", [](const ComputedStyle& s) { return s.left.ToString(); }},
    {"z-index", [](const ComputedStyle& s) { return std::to_string(s.z_index); }},
    {"overflow", [](const ComputedStyle& s) { return s.overflow.empty() ? "visible" : s.overflow; }},
    {"visibility", [](const ComputedStyle& s) { return s.visibility; }},
    
    // Box Model
    {"width", [](const ComputedStyle& s) { return s.width.ToString(); }},
    {"height", [](const ComputedStyle& s) { return s.height.ToString(); }},
    {"min-width", [](const ComputedStyle& s) { return s.min_width.ToString(); }},
    {"max-width", [](const ComputedStyle& s) { return s.max_width.ToString(); }},
    {"min-height", [](const ComputedStyle& s) { return s.min_height.ToString(); }},
    {"max-height", [](const ComputedStyle& s) { return s.max_height.ToString(); }},
    
    // Typography
    {"font-family", [](const ComputedStyle& s) { return s.font_family.empty() ? "inherit" : s.font_family; }},
    {"font-size", [](const ComputedStyle& s) { return std::to_string(s.font_size) + "px"; }},
    {"font-weight", [](const ComputedStyle& s) { return s.font_weight.empty() ? "normal" : s.font_weight; }},
    {"font-style", [](const ComputedStyle& s) { return s.font_style.empty() ? "normal" : s.font_style; }},
    {"line-height", [](const ComputedStyle& s) { return std::to_string(s.line_height); }},
    {"text-align", [](const ComputedStyle& s) { return s.text_align.empty() ? "start" : s.text_align; }},
    {"text-decoration", [](const ComputedStyle& s) { return s.text_decoration.empty() ? "none" : s.text_decoration; }},
    
    // Colors
    {"color", [](const ComputedStyle& s) { return s.color.empty() ? "inherit" : s.color; }},
    {"background-color", [](const ComputedStyle& s) { return s.background_color.empty() ? "transparent" : s.background_color; }},
    {"opacity", [](const ComputedStyle& s) { return std::to_string(s.opacity); }},
    
    // Flexbox
    {"flex-direction", [](const ComputedStyle& s) { return s.flex_direction; }},
    {"flex-wrap", [](const ComputedStyle& s) { return s.flex_wrap; }},
    {"justify-content", [](const ComputedStyle& s) { return s.justify_content; }},
    {"align-items", [](const ComputedStyle& s) { return s.align_items; }},
    {"flex-grow", [](const ComputedStyle& s) { return std::to_string(s.flex_grow); }},
    {"flex-shrink", [](const ComputedStyle& s) { return std::to_string(s.flex_shrink); }},
    {"flex-basis", [](const ComputedStyle& s) { return s.flex_basis.ToString(); }},
};
```

### 默认值映射

```cpp
const std::unordered_map<std::string, std::string> DEFAULT_VALUES = {
    {"display", "block"},
    {"position", "static"},
    {"top", "auto"},
    {"right", "auto"},
    {"bottom", "auto"},
    {"left", "auto"},
    {"z-index", "0"},
    {"overflow", "visible"},
    {"visibility", "visible"},
    {"width", "auto"},
    {"height", "auto"},
    {"min-width", "auto"},
    {"max-width", "none"},
    {"min-height", "auto"},
    {"max-height", "none"},
    {"font-size", "16px"},
    {"font-weight", "normal"},
    {"font-style", "normal"},
    {"line-height", "1.2"},
    {"text-align", "start"},
    {"text-decoration", "none"},
    {"color", "inherit"},
    {"background-color", "transparent"},
    {"opacity", "1"},
    {"flex-direction", "row"},
    {"flex-wrap", "nowrap"},
    {"justify-content", "flex-start"},
    {"align-items", "normal"},
    {"flex-grow", "0"},
    {"flex-shrink", "1"},
    {"flex-basis", "auto"},
};
```

## Correctness Properties

*A property is a characteristic or behavior that should hold true across all valid executions of a system-essentially, a formal statement about what the system should do. Properties serve as the bridge between human-readable specifications and machine-verifiable correctness guarantees.*

Based on the prework analysis, the following correctness properties have been identified:

### Property 1: Computed Style Value Accuracy
*For any* selected element with a RenderObject, the ComputedStylesView SHALL display values that match the corresponding fields in RenderObject::GetComputedStyle() for all supported CSS properties.
**Validates: Requirements 1.1, 1.2**

### Property 2: Non-Default Value Detection
*For any* computed style property, the is_default flag SHALL be true if and only if the property value equals the CSS default value for that property.
**Validates: Requirements 1.3**

### Property 3: Box Model Margin Display Completeness
*For any* selected element, the BoxModelView SHALL display exactly four margin values (top, right, bottom, left) that match the values from GetBoxModelData().
**Validates: Requirements 2.1**

### Property 4: Box Model Border Display Completeness
*For any* selected element, the BoxModelView SHALL display exactly four border values (top, right, bottom, left) that match the values from GetBoxModelData().
**Validates: Requirements 2.2**

### Property 5: Box Model Padding Display Completeness
*For any* selected element, the BoxModelView SHALL display exactly four padding values (top, right, bottom, left) that match the values from GetBoxModelData().
**Validates: Requirements 2.3**

### Property 6: Box Model Content Display
*For any* selected element, the BoxModelView SHALL display the content width and height that match the values from GetBoxModelData().
**Validates: Requirements 2.4**

### Property 7: Box Model Hover State Consistency
*For any* mouse position within the BoxModelView diagram, HandleMouseMove() SHALL set hovered_area_ to exactly one of {None, Margin, Border, Padding, Content} based on the hit test result.
**Validates: Requirements 3.1, 3.2, 3.3, 3.4**

### Property 8: Box Model Hover Reset
*For any* mouse position outside the BoxModelView diagram, HandleMouseMove() SHALL set hovered_area_ to BoxAreaType::None.
**Validates: Requirements 3.5**

## Error Handling

### 错误场景

1. **RenderObject 未找到**: 当元素没有对应的 RenderObject 时（例如 display: none），显示占位符值
2. **属性值转换失败**: 当 CSSLength 等类型转换为字符串失败时，显示 "unknown"
3. **鼠标事件坐标越界**: 当鼠标坐标超出视图范围时，重置悬停状态

### 错误处理策略

```cpp
// RenderObject 未找到时的处理
if (!render_obj) {
    // 显示占位符，标记所有属性为默认值
    for (auto& prop : category.properties) {
        prop.value = "N/A";
        prop.is_default = true;
    }
    return;
}

// 属性值获取失败时的处理
try {
    prop.value = getter(style);
} catch (...) {
    prop.value = "unknown";
    prop.is_default = true;
}
```

## Testing Strategy

### 属性测试框架

使用 RapidCheck 作为 C++ 属性测试库。

### 属性测试要求

- 每个属性测试必须运行至少 100 次迭代
- 每个属性测试必须使用注释标注对应的正确性属性
- 注释格式: `// **Feature: devtools-improvements, Property {number}: {property_text}**`

### 测试文件

```
tests/devtools/
└── test_devtools_improvements.cpp  # 新增测试文件
```

### 关键测试场景

1. **计算样式准确性测试**
   - 创建具有各种样式的元素
   - 验证 ComputedStylesView 显示的值与 RenderObject::GetComputedStyle() 一致

2. **盒模型数值完整性测试**
   - 创建具有各种 margin/border/padding 值的元素
   - 验证 BoxModelView 显示所有 12 个数值（4 margin + 4 border + 4 padding）

3. **悬停状态测试**
   - 模拟鼠标在不同区域移动
   - 验证 hovered_area_ 状态正确变化

