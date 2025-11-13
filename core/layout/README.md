# Layout 模块

## 📋 概述

Layout 模块是 MBink 的布局引擎，基于 Facebook 的 Yoga 库实现 Flexbox 布局。它负责将 CSS 样式转换为布局属性，计算元素的位置和尺寸，为渲染引擎提供精确的布局信息。

## 🎯 主要功能

- **Flexbox 布局**: 完整的 CSS Flexbox 规范支持
- **布局计算**: 自动计算元素位置和尺寸
- **样式映射**: CSS 属性到 Yoga 属性的转换
- **布局树管理**: 维护与 DOM 树对应的布局树
- **增量更新**: 只重新计算变化的部分
- **响应式布局**: 支持百分比、auto 等响应式单位

## 📁 文件结构

```
layout/
├── CMakeLists.txt        # 构建配置
├── layout_engine.h       # 布局引擎头文件
└── layout_engine.cpp     # 布局引擎实现
```

## 🔌 核心类

### LayoutEngine

```cpp
class LayoutEngine {
public:
    LayoutEngine();
    ~LayoutEngine();
    
    // 布局计算
    void CalculateLayout(std::shared_ptr<Element> root, 
                        float available_width,
                        float available_height);
    
    // 获取布局结果
    LayoutRect GetLayoutRect(std::shared_ptr<Element> element);
    
    // 样式更新
    void UpdateStyle(std::shared_ptr<Element> element);
    
    // 布局树管理
    void AttachNode(std::shared_ptr<Element> element);
    void DetachNode(std::shared_ptr<Element> element);
    
    // 增量更新
    void MarkDirty(std::shared_ptr<Element> element);
    bool IsDirty(std::shared_ptr<Element> element);
};
```

### LayoutRect (布局结果)

```cpp
struct LayoutRect {
    float x;          // X 坐标
    float y;          // Y 坐标
    float width;      // 宽度
    float height;     // 高度
    
    // 边距
    float margin_top;
    float margin_right;
    float margin_bottom;
    float margin_left;
    
    // 内边距
    float padding_top;
    float padding_right;
    float padding_bottom;
    float padding_left;
    
    // 边框
    float border_top;
    float border_right;
    float border_bottom;
    float border_left;
};
```

## 💡 使用示例

### 基础布局计算

```cpp
auto layout_engine = std::make_shared<LayoutEngine>();
auto root = doc->GetBody();

// 设置样式
root->GetStyle()->SetProperty("display", "flex");
root->GetStyle()->SetProperty("flex-direction", "column");
root->GetStyle()->SetProperty("width", "800px");
root->GetStyle()->SetProperty("height", "600px");

// 计算布局
layout_engine->CalculateLayout(root, 800, 600);

// 获取布局结果
auto rect = layout_engine->GetLayoutRect(root);
std::cout << "Position: " << rect.x << ", " << rect.y << std::endl;
std::cout << "Size: " << rect.width << " x " << rect.height << std::endl;
```

### Flexbox 布局

```cpp
// 创建容器
auto container = doc->CreateElement("div");
container->GetStyle()->SetProperty("display", "flex");
container->GetStyle()->SetProperty("flex-direction", "row");
container->GetStyle()->SetProperty("justify-content", "space-between");
container->GetStyle()->SetProperty("align-items", "center");

// 创建子元素
auto child1 = doc->CreateElement("div");
child1->GetStyle()->SetProperty("flex", "1");
child1->GetStyle()->SetProperty("height", "100px");

auto child2 = doc->CreateElement("div");
child2->GetStyle()->SetProperty("flex", "2");
child2->GetStyle()->SetProperty("height", "100px");

container->AppendChild(child1);
container->AppendChild(child2);

// 计算布局
layout_engine->CalculateLayout(container, 800, 600);
```

### 响应式布局

```cpp
auto element = doc->CreateElement("div");

// 使用百分比
element->GetStyle()->SetProperty("width", "50%");
element->GetStyle()->SetProperty("height", "100%");

// 使用 auto
element->GetStyle()->SetProperty("margin", "auto");

// 最小/最大尺寸
element->GetStyle()->SetProperty("min-width", "200px");
element->GetStyle()->SetProperty("max-width", "800px");
```

### 增量更新

```cpp
// 标记元素为脏（需要重新布局）
layout_engine->MarkDirty(element);

// 只重新计算脏元素
layout_engine->CalculateLayout(root, 800, 600);
```

## 🔗 依赖关系

### 依赖的模块

- `third_party/yoga` - Yoga 布局引擎
- `core/dom` - DOM 元素和样式
- `core/utils` - 工具函数

### 被依赖的模块

- `core/render` - 渲染引擎（使用布局结果）
- `core/event` - 事件系统（命中测试需要布局信息）

## 🏗️ 架构说明

Layout 模块在架构中的位置：

```
┌─────────────────────────────────────────┐
│  DOM Tree (core/dom)                    │
│  Element with CSS styles                │
└─────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────┐
│  Layout Module (core/layout) ← 当前模块  │
│  LayoutEngine + Yoga                    │
└─────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────┐
│  Render Module (core/render)            │
│  使用布局结果进行渲染                     │
└─────────────────────────────────────────┘
```

## 📊 支持的 CSS 属性

### Flexbox 属性

#### 容器属性
- `display: flex | inline-flex`
- `flex-direction: row | column | row-reverse | column-reverse`
- `flex-wrap: nowrap | wrap | wrap-reverse`
- `justify-content: flex-start | flex-end | center | space-between | space-around`
- `align-items: flex-start | flex-end | center | stretch | baseline`
- `align-content: flex-start | flex-end | center | stretch | space-between | space-around`

#### 子元素属性
- `flex: <grow> <shrink> <basis>`
- `flex-grow: <number>`
- `flex-shrink: <number>`
- `flex-basis: <length> | auto`
- `align-self: auto | flex-start | flex-end | center | stretch | baseline`
- `order: <integer>`

### 盒模型属性

#### 尺寸
- `width`, `height`
- `min-width`, `min-height`
- `max-width`, `max-height`

#### 边距
- `margin`, `margin-top`, `margin-right`, `margin-bottom`, `margin-left`
- `padding`, `padding-top`, `padding-right`, `padding-bottom`, `padding-left`

#### 边框
- `border-width`, `border-top-width`, `border-right-width`, etc.

### 定位
- `position: relative | absolute`
- `top`, `right`, `bottom`, `left`

## 🔧 样式映射

### CSS 到 Yoga 的转换

```cpp
// CSS: display: flex
YGNodeStyleSetDisplay(node, YGDisplayFlex);

// CSS: flex-direction: row
YGNodeStyleSetFlexDirection(node, YGFlexDirectionRow);

// CSS: width: 100px
YGNodeStyleSetWidth(node, 100);

// CSS: width: 50%
YGNodeStyleSetWidthPercent(node, 50);

// CSS: width: auto
YGNodeStyleSetWidthAuto(node);

// CSS: margin: 10px
YGNodeStyleSetMargin(node, YGEdgeAll, 10);

// CSS: padding: 5px 10px
YGNodeStyleSetPadding(node, YGEdgeVertical, 5);
YGNodeStyleSetPadding(node, YGEdgeHorizontal, 10);
```

## ⚠️ 注意事项

1. **布局顺序**: 必须先计算布局，再进行渲染
2. **性能**: 避免频繁的布局计算，使用增量更新
3. **单位转换**: 确保正确处理 px、%、auto 等单位
4. **循环依赖**: 避免父子元素尺寸相互依赖

## 🚀 性能优化

### 增量布局

```cpp
// 只标记变化的元素
element->GetStyle()->SetProperty("width", "200px");
layout_engine->MarkDirty(element);

// Yoga 会自动只重新计算必要的部分
layout_engine->CalculateLayout(root, 800, 600);
```

### 布局缓存

```cpp
// 缓存布局结果
std::unordered_map<Element*, LayoutRect> layout_cache_;

// 只在脏元素时重新计算
if (layout_engine->IsDirty(element)) {
    auto rect = layout_engine->GetLayoutRect(element);
    layout_cache_[element.get()] = rect;
}
```

### 批量更新

```cpp
// 批量修改样式
element->GetStyle()->SetProperty("width", "200px");
element->GetStyle()->SetProperty("height", "100px");
element->GetStyle()->SetProperty("margin", "10px");

// 一次性计算布局
layout_engine->CalculateLayout(root, 800, 600);
```

## 🐛 调试技巧

### 打印布局树

```cpp
void PrintLayoutTree(std::shared_ptr<Element> element, int depth = 0) {
    auto rect = layout_engine->GetLayoutRect(element);
    
    std::string indent(depth * 2, ' ');
    std::cout << indent << element->GetTagName() 
              << " [" << rect.x << ", " << rect.y 
              << ", " << rect.width << ", " << rect.height << "]"
              << std::endl;
    
    for (auto child : element->GetChildren()) {
        PrintLayoutTree(child, depth + 1);
    }
}
```

### 可视化布局

```cpp
// 在渲染时绘制布局边界
void DrawLayoutBounds(SkCanvas* canvas, std::shared_ptr<Element> element) {
    auto rect = layout_engine->GetLayoutRect(element);
    
    SkPaint paint;
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setColor(SK_ColorRED);
    paint.setStrokeWidth(1);
    
    canvas->drawRect(SkRect::MakeXYWH(rect.x, rect.y, 
                                      rect.width, rect.height), 
                     paint);
}
```

## 📚 相关文档

- [Yoga 官方文档](https://yogalayout.com/)
- [CSS Flexbox 规范](https://www.w3.org/TR/css-flexbox-1/)
- [渲染引擎文档](../render/README.md)
- [性能优化指南](../../docs/PERFORMANCE.md)

## 🔮 未来改进

1. **Grid 布局**: 支持 CSS Grid
2. **绝对定位**: 完善 absolute/fixed 定位
3. **文本布局**: 集成文本换行和对齐
4. **动画支持**: 布局属性的平滑过渡

---

**维护者**: MBink Team  
**最后更新**: 2025-11-12

