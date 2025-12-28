# Painters 模块

本目录包含从 `render_object.cpp` 提取的绘制逻辑，遵循单一职责原则。

## 文件说明

| 文件 | 职责 | 行数 |
|------|------|------|
| `background_painter.cpp/h` | 背景绘制（纯色、渐变、图片） | ~400 |
| `border_painter.cpp/h` | 边框绘制（实线、虚线、点线、圆角） | ~350 |
| `scrollbar_painter.cpp/h` | 滚动条绘制（轨道、滑块、角落） | ~250 |
| `form_element_painter.cpp/h` | 表单元素绘制（input、textarea、checkbox、radio） | ~350 |

## BackgroundPainter

负责渲染元素的背景，支持：

- **纯色背景**: `background-color`
- **线性渐变**: `linear-gradient()`
- **径向渐变**: `radial-gradient()`
- **背景图片**: `background-image: url()`
- **圆角裁剪**: 配合 `border-radius` 使用

### 使用示例

```cpp
#include "core/render/painters/background_painter.h"

void RenderObject::Paint(SkCanvas* canvas) {
    BackgroundPainter bg_painter(canvas);
    bg_painter.Paint(box, computed_style_, paint_cache_);
}
```

## BorderPainter

负责渲染元素的边框，支持：

- **实线边框**: `border-style: solid`
- **虚线边框**: `border-style: dashed`
- **点线边框**: `border-style: dotted`
- **双线边框**: `border-style: double`
- **圆角边框**: 配合 `border-radius` 使用
- **每边独立样式**: 支持四边不同的宽度、样式、颜色
- **fieldset legend**: 特殊处理上边框在 legend 位置断开

### 使用示例

```cpp
#include "core/render/painters/border_painter.h"

void RenderObject::Paint(SkCanvas* canvas) {
    BorderPainter border_painter(canvas);
    border_painter.Paint(box, computed_style_, paint_cache_, &children_);
}
```

## 设计原则

1. **单一职责**: 每个 Painter 只负责一种类型的绘制
2. **无状态**: Painter 不保存绘制状态，每次调用独立
3. **可组合**: 多个 Painter 可以组合使用
4. **性能优先**: 利用 PaintCache 避免重复计算

## ScrollbarPainter

负责渲染元素的滚动条，支持：

- **水平滚动条**: 轨道和滑块
- **垂直滚动条**: 轨道和滑块
- **滚动条角落**: 两个滚动条都存在时的右下角区域
- **自定义颜色**: 可设置轨道和滑块颜色

### 使用示例

```cpp
#include "core/render/painters/scrollbar_painter.h"

void RenderObject::Paint(SkCanvas* canvas) {
    ScrollbarPainter scrollbar_painter(canvas);
    auto params = ScrollbarPainter::CreateParams(*this, box, computed_style_);
    scrollbar_painter.Paint(params);
}
```

## FormElementPainter

负责渲染表单控件，支持：

- **文本输入框**: `<input type="text/password/email/tel/url/search/number">`
- **多行文本框**: `<textarea>`
- **复选框**: `<input type="checkbox">`
- **单选按钮**: `<input type="radio">`
- **文本选中高亮**: 选中文本的蓝色背景
- **输入光标**: 闪烁的文本光标

### 使用示例

```cpp
#include "core/render/painters/form_element_painter.h"

void RenderObject::PaintInputElement(SkCanvas* canvas, HTMLInputElement* input, const Box& box) {
    FormElementPainter painter(canvas);
    FormElementPaintParams params;
    params.font_family = computed_style_.font_family;
    params.font_size = computed_style_.font_size;
    params.text_color = computed_style_.color;
    params.has_focus = element->HasPseudoClass("focus");
    painter.PaintInputElement(input, box, params);
}
```

## 参考

- 设计文档: `.kiro/specs/large-file-refactoring/design.md`
- 任务列表: `.kiro/specs/large-file-refactoring/tasks.md`
