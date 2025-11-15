# 当前布局问题分析

## 问题描述

用户报告 `window_demo` 的 todo list 布局不正确。具体表现为：
- checkbox、文本和按钮没有正确对齐
- `display: flex` 和 `justify-content: space-between` 没有生效

## 根本原因

经过全面分析，发现 **MBink 根本没有实现 Flexbox 布局**，虽然文档声称支持：

### 1. Layout Engine 是空的

```cpp
// core/layout/layout_engine.cpp
namespace lightui {
// TODO: 实现函数
}
```

```cpp
// core/layout/layout_engine.h
namespace lightui {
// TODO: 添加类定义和函数声明
}
```

### 2. Yoga 没有被集成

虽然 `CMakeLists.txt` 链接了 Yoga 库，但：
- `RenderObject` 没有 `YGNodeRef` 字段
- 没有任何代码调用 Yoga API
- 布局完全是手动计算的

### 3. ComputedStyle 缺少 Flexbox 字段

```cpp
struct ComputedStyle {
    // 只有这些字段：
    RenderObjectType display;
    CSSLength width, height;
    CSSEdges margin, padding;
    CSSBorder border;
    std::string background_color;
    std::string color, font_family;
    // ...
    
    // ❌ 没有 flex-direction
    // ❌ 没有 justify-content
    // ❌ 没有 align-items
    // ❌ 没有任何 Flexbox 属性！
};
```

### 4. StyleResolver 不解析 Flexbox 属性

```cpp
void StyleResolver::ParseStyleProperty(ComputedStyle& style,
                                       const std::string& property,
                                       const std::string& value) {
    if (property == "display") {
        style.display = ParseDisplay(resolved_value);
    }
    else if (property == "width") {
        style.width = CSSValue::ParseLength(resolved_value);
    }
    // ...
    // ❌ 没有 "justify-content"
    // ❌ 没有 "align-items"
    // ❌ 没有任何 Flexbox 属性解析！
}
```

### 5. 当前布局算法

`RenderObject::Layout()` 使用简单的手动算法：
- **块级元素**：垂直堆叠
- **内联元素**：水平排列，垂直居中

```cpp
// core/render/render_object.cpp
void RenderObject::Layout(float available_width, float available_height) {
    // 简单的垂直堆叠或水平排列
    // 完全没有 Flexbox 逻辑！
}
```

## 影响范围

### 受影响的功能

1. **Todo List**（当前问题）
   - `<li style="display: flex; justify-content: space-between;">` 不生效
   - 按钮没有对齐到右侧

2. **所有使用 Flexbox 的布局**
   - 任何 `display: flex` 都会被忽略
   - 所有 Flexbox 属性都无效

3. **响应式布局**
   - 无法使用 Flexbox 实现响应式设计
   - 无法使用 `flex-wrap`

### 不受影响的功能

1. **基本块级布局**：正常工作
2. **基本内联布局**：正常工作
3. **盒模型**：margin, padding, border 正常
4. **文本渲染**：正常
5. **事件处理**：正常

## 解决方案

### 短期方案（阶段 1）：最小化 Flexbox 支持

**目标**：让 todo list 正确显示

**步骤**：
1. 在 `ComputedStyle` 中添加基本 Flexbox 字段
2. 在 `StyleResolver` 中解析 Flexbox 属性
3. 在 `RenderObject` 中集成 Yoga 进行布局
4. 只支持最常用的 Flexbox 属性：
   - `display: flex`
   - `flex-direction: row | column`
   - `justify-content: flex-start | flex-end | center | space-between | space-around`
   - `align-items: flex-start | flex-end | center | stretch`

**预计时间**：2-3 天

### 长期方案：完整 CSS 和布局系统

参见 `docs/CSS_LAYOUT_COMPLETION_PLAN.md`

## 技术细节

### Yoga 集成步骤

1. **在 RenderObject 中添加 Yoga 节点**

```cpp
class RenderObject {
private:
    YGNodeRef yoga_node_ = nullptr;  // 添加这个字段
};
```

2. **创建和销毁 Yoga 节点**

```cpp
RenderObject::RenderObject() {
    yoga_node_ = YGNodeNew();
}

RenderObject::~RenderObject() {
    if (yoga_node_) {
        YGNodeFree(yoga_node_);
    }
}
```

3. **将 ComputedStyle 应用到 Yoga**

```cpp
void RenderObject::ApplyYogaStyle(const ComputedStyle& style) {
    // Display
    if (style.display == RenderObjectType::FLEX) {
        YGNodeStyleSetDisplay(yoga_node_, YGDisplayFlex);
    }
    
    // Flex direction
    if (style.flex_direction == "row") {
        YGNodeStyleSetFlexDirection(yoga_node_, YGFlexDirectionRow);
    } else if (style.flex_direction == "column") {
        YGNodeStyleSetFlexDirection(yoga_node_, YGFlexDirectionColumn);
    }
    
    // Justify content
    if (style.justify_content == "flex-start") {
        YGNodeStyleSetJustifyContent(yoga_node_, YGJustifyFlexStart);
    } else if (style.justify_content == "center") {
        YGNodeStyleSetJustifyContent(yoga_node_, YGJustifyCenter);
    } else if (style.justify_content == "space-between") {
        YGNodeStyleSetJustifyContent(yoga_node_, YGJustifySpaceBetween);
    }
    // ... 等等
}
```

4. **使用 Yoga 计算布局**

```cpp
void RenderObject::Layout(float available_width, float available_height) {
    // 应用样式到 Yoga
    ApplyYogaStyle(computed_style_);
    
    // 计算布局
    YGNodeCalculateLayout(yoga_node_, available_width, available_height, YGDirectionLTR);
    
    // 从 Yoga 读取布局结果
    layout_info_.x = YGNodeLayoutGetLeft(yoga_node_);
    layout_info_.y = YGNodeLayoutGetTop(yoga_node_);
    layout_info_.width = YGNodeLayoutGetWidth(yoga_node_);
    layout_info_.height = YGNodeLayoutGetHeight(yoga_node_);
    
    // 递归布局子元素
    for (size_t i = 0; i < children_.size(); i++) {
        auto& child = children_[i];
        child->Layout(layout_info_.width, layout_info_.height);
    }
}
```

### 需要修改的文件

1. **core/render/render_object.h**
   - 添加 `YGNodeRef yoga_node_` 字段
   - 添加 `ApplyYogaStyle()` 方法
   - 添加 `#include <yoga/Yoga.h>`

2. **core/render/render_object.cpp**
   - 实现 Yoga 节点创建/销毁
   - 实现 `ApplyYogaStyle()`
   - 修改 `Layout()` 使用 Yoga

3. **core/render/render_object.h** (ComputedStyle)
   - 添加 Flexbox 字段

4. **core/render/style_resolver.cpp**
   - 在 `ParseStyleProperty()` 中添加 Flexbox 属性解析
   - 在 `ParseDisplay()` 中添加 `flex` 和 `inline-flex`

5. **core/render/CMakeLists.txt**
   - 确保链接 Yoga 库

## 测试计划

### 测试用例 1：Todo List

```html
<li style="display: flex; justify-content: space-between; align-items: center;">
    <span>Todo item text</span>
    <div>
        <button>Done</button>
        <button>Delete</button>
    </div>
</li>
```

**预期结果**：
- 文本在左侧
- 按钮在右侧
- 垂直居中对齐

### 测试用例 2：简单 Flexbox

```html
<div style="display: flex; flex-direction: row;">
    <div style="width: 100px; height: 100px; background: red;"></div>
    <div style="width: 100px; height: 100px; background: blue;"></div>
</div>
```

**预期结果**：
- 两个方块水平排列

### 测试用例 3：Flex Direction Column

```html
<div style="display: flex; flex-direction: column;">
    <div style="width: 100px; height: 100px; background: red;"></div>
    <div style="width: 100px; height: 100px; background: blue;"></div>
</div>
```

**预期结果**：
- 两个方块垂直排列

## 风险和注意事项

1. **向后兼容性**
   - 现有的非 Flexbox 布局必须继续工作
   - 需要保留原有的块级和内联布局逻辑

2. **性能**
   - Yoga 布局计算可能比手动计算慢
   - 需要测试性能影响

3. **Yoga 版本**
   - 确认项目使用的 Yoga 版本
   - 确保 API 兼容性

4. **子元素管理**
   - Yoga 有自己的子节点树
   - 需要与 RenderObject 的子节点树同步

## 下一步行动

1. ✅ 创建详细的修复计划文档
2. ⏭️ 开始实施阶段 1
3. ⏭️ 测试 todo list 布局
4. ⏭️ 修复发现的问题
5. ⏭️ 继续后续阶段

---

**创建时间**：2025-11-15  
**作者**：Augment Agent  
**相关文档**：`docs/CSS_LAYOUT_COMPLETION_PLAN.md`

