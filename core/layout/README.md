# Layout 模块

## 📋 概述

Layout 模块是 MBink 的原生布局引擎，实现了完整的 CSS 布局规范，包括 Block、Flexbox、Grid 和 IFC（Inline Formatting Context）布局。它负责将 CSS 样式转换为布局属性，计算元素的位置和尺寸，为渲染引擎提供精确的布局信息。

## 🎯 主要功能

- **Block 布局**: CSS 块级布局
- **Flexbox 布局**: 完整的 CSS Flexbox 规范支持
- **Grid 布局**: CSS Grid 布局支持
- **IFC 布局**: 内联格式化上下文，处理文本和内联元素
- **布局计算**: 自动计算元素位置和尺寸
- **布局树管理**: 维护与 RenderTree 对应的布局树
- **增量更新**: 只重新计算变化的部分
- **响应式布局**: 支持百分比、auto 等响应式单位

## 📁 文件结构

```
layout/
├── CMakeLists.txt           # 构建配置
├── layout_engine.h          # 布局引擎抽象接口
├── native_layout_engine.h   # 原生布局引擎实现
├── native_layout_engine.cpp
├── block_layout.h/cpp       # Block 布局算法
├── flex_layout.h/cpp        # Flexbox 布局算法
├── grid/                    # Grid 布局
│   ├── grid.h/cpp
│   └── types.h
├── ifc/                     # IFC (Inline Formatting Context)
│   ├── ifc_layout.h/cpp     # IFC 布局入口
│   ├── inline_box.h/cpp     # 内联盒
│   ├── line_box.h/cpp       # 行盒
│   ├── line_breaker.h/cpp   # 断行器
│   ├── text_run.h/cpp       # 文本片段
│   └── vertical_aligner.h/cpp # 垂直对齐
├── types/                   # 类型定义
│   ├── geometry.h           # Size, Point, Rect
│   ├── style.h              # Style 结构
│   ├── layout.h             # Layout 结果
│   ├── cache.h              # 缓存
│   └── traits.h             # 布局接口
└── util/                    # 工具函数
    ├── math.h
    └── resolve.h
```

## 🔌 核心类

### NativeLayoutEngine

```cpp
class NativeLayoutEngine : public LayoutEngine, public LayoutBlockContainer {
public:
    NativeLayoutEngine();
    ~NativeLayoutEngine() override;

    // 布局计算
    void CalculateLayout(RenderObject* root,
                        float available_width,
                        float available_height) override;

    // 布局树管理
    void BuildLayoutTree(RenderObject* root);
    void ReadLayoutResults(RenderObject* root);

    // 布局算法分发
    LayoutOutput ComputeNodeLayout(NodeId node, const LayoutInput& inputs);
    LayoutOutput ComputeBlockLayout(NodeId node, const LayoutInput& inputs);
    LayoutOutput ComputeFlexLayout(NodeId node, const LayoutInput& inputs);
    LayoutOutput ComputeGridLayout(NodeId node, const LayoutInput& inputs);
    LayoutOutput ComputeIFCLayout(NodeId node, const LayoutInput& inputs);
};
```

### IFCLayout (内联格式化上下文)

```cpp
class IFCLayout {
public:
    // 静态文本测量
    static TextMeasureResult MeasureTextStatic(
        const std::string& text,
        float font_size,
        const std::string& font_family,
        float letter_spacing = 0.0f,
        float word_spacing = 0.0f,
        float line_height_multiplier = 1.2f
    );

    // 布局计算
    IFCLayoutResult Layout(RenderObject* container, float available_width);
    IFCMeasureResult LayoutWithResult(RenderObject* container, float available_width);

    // 应用布局结果
    void ApplyLayoutResults(RenderObject* container, float padding_left, float padding_top);
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

- `core/render` - RenderObject 和样式
- `core/utils` - 工具函数
- `third_party/skia` - 文本测量

### 被依赖的模块

- `core/render` - 渲染引擎（使用布局结果）
- `core/event` - 事件系统（命中测试需要布局信息）

## 🏗️ 架构说明

Layout 模块在架构中的位置：

```
┌─────────────────────────────────────────┐
│  RenderTree (core/render)               │
│  RenderObject with ComputedStyle        │
└─────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────┐
│  Layout Module (core/layout) ← 当前模块  │
│  NativeLayoutEngine                     │
│  ├── Block Layout                       │
│  ├── Flex Layout                        │
│  ├── Grid Layout                        │
│  └── IFC Layout (inline content)        │
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

## 🔧 布局流程

### 布局计算流程

```cpp
// 1. 构建布局树
layout_engine->BuildLayoutTree(render_root);

// 2. 计算布局
layout_engine->CalculateLayout(render_root, viewport_width, viewport_height);

// 3. 读取布局结果（更新 RenderObject 的 LayoutInfo）
layout_engine->ReadLayoutResults(render_root);
```

### IFC 布局流程

```cpp
// IFC 布局自动处理内联内容：
// 1. 收集内联内容 (CollectInlineContent)
// 2. 断行 (LineBreaker)
// 3. 计算行度量 (VerticalAligner::CalculateLineMetrics)
// 4. 垂直对齐 (VerticalAligner::AlignBoxes)
// 5. 应用布局结果 (ApplyLayoutResults)
```

## ⚠️ 注意事项

1. **布局顺序**: 必须先计算布局，再进行渲染
2. **性能**: 避免频繁的布局计算，使用缓存
3. **单位转换**: 确保正确处理 px、%、auto 等单位
4. **IFC 容器**: 只有纯内联内容的块级容器才使用 IFC 布局

## 🚀 性能优化

### 布局缓存

布局引擎内置缓存机制，避免重复计算：

```cpp
// Cache 类自动缓存布局结果
struct Cache {
    std::optional<LayoutOutput> Get(
        const Size<std::optional<float>>& known_dimensions,
        const Size<AvailableSpace>& available_space,
        RunMode run_mode
    );

    void Store(
        const Size<std::optional<float>>& known_dimensions,
        const Size<AvailableSpace>& available_space,
        RunMode run_mode,
        const LayoutOutput& output
    );
};
```

## 🐛 调试技巧

### 打印布局树

```cpp
void PrintLayoutTree(RenderObject* obj, int depth = 0) {
    const auto& info = obj->GetLayoutInfo();

    std::string indent(depth * 2, ' ');
    std::cout << indent << "[" << obj->GetTagName() << "]"
              << " x=" << info.x << " y=" << info.y
              << " w=" << info.width << " h=" << info.height
              << std::endl;

    for (auto* child : obj->GetChildren()) {
        PrintLayoutTree(child, depth + 1);
    }
}
```

## 📚 相关文档

- [CSS Flexbox 规范](https://www.w3.org/TR/css-flexbox-1/)
- [CSS Grid 规范](https://www.w3.org/TR/css-grid-1/)
- [CSS Inline Layout 规范](https://www.w3.org/TR/css-inline-3/)
- [渲染引擎文档](../render/README.md)

## 🔮 未来改进

1. **绝对定位**: 完善 absolute/fixed 定位
2. **浮动布局**: 支持 float 属性
3. **动画支持**: 布局属性的平滑过渡
4. **性能优化**: 增量布局更新

---

**维护者**: MBink Team
**最后更新**: 2025-12-06

