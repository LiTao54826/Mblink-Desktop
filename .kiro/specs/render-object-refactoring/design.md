# render_object.cpp 拆分设计文档

## 概述

本文档描述了对 render_object.cpp (5016 行) 进行深度拆分的详细设计方案。目标是将其拆分为多个专注的模块文件。

## 1. 当前结构分析

```
render_object.cpp (5016 行) 包含:
├── RenderObject 基类实现 (1-1072 行, ~1070 行)
│   ├── 构造/析构/基础方法 (~200 行)
│   ├── 子节点管理 (~50 行)
│   ├── 布局样式转换 (~200 行)
│   ├── Paint 基类方法 (~200 行)
│   ├── 滚动相关方法 (~200 行)
│   └── 内容尺寸计算 (~220 行)
│
├── RenderBlock 实现 (1073-2748 行, ~1675 行) ← 主要拆分目标
│   ├── Layout 方法 (~230 行)
│   ├── Paint 方法 (~660 行)
│   ├── PaintInputElement (~210 行) ← 重复代码
│   ├── PaintTextAreaElement (~90 行) ← 重复代码
│   └── PaintContentEditableCaret (~455 行)
│
├── RenderInline 实现 (2749-3384 行, ~635 行)
│   ├── Layout 方法 (~70 行)
│   ├── PositionChildrenOnly (~35 行)
│   ├── MeasureIntrinsicSize (~55 行)
│   ├── Paint 方法 (~160 行)
│   ├── PaintInputElement (~210 行) ← 重复代码
│   └── PaintTextAreaElement (~90 行) ← 重复代码
│
├── RenderText 实现 (3385-3715 行, ~330 行)
│   ├── Layout 方法 (~85 行)
│   └── Paint 方法 (~240 行)
│
├── RenderTable 系列 (3716-4840 行, ~1125 行)
│   ├── RenderTable (~665 行)
│   │   ├── CollectColumnStyles (~80 行)
│   │   ├── CalculateColumnWidths (~215 行)
│   │   ├── Layout (~285 行)
│   │   └── Paint (~75 行)
│   ├── RenderTableRowGroup (~45 行)
│   ├── RenderTableRow (~160 行)
│   ├── RenderTableCell (~170 行)
│   └── RenderTableCaption (~80 行)
│
└── LayerInfo + 属性树方法 (4841-5016 行, ~175 行)
    ├── LayerInfo 实现 (~20 行)
    └── 属性树状态方法 (~155 行)
```

## 2. 拆分方案

### 2.1 目标文件结构

```
core/render/
├── render_object.cpp      (~1000 行) - 基类实现 + LayerInfo + 属性树
├── render_object.h        (保持不变)
├── render_block.cpp       (~1200 行) - RenderBlock 实现
├── render_inline.cpp      (~400 行)  - RenderInline 实现
├── render_text.cpp        (~350 行)  - RenderText 实现
├── render_table.cpp       (~1200 行) - 表格系列实现
└── painters/
    └── form_element_painter.cpp (已存在) - 统一的表单元素绘制
```

### 2.2 render_object.cpp (目标 ~1000 行)

保留内容：
- RenderObject 构造/析构
- 子节点管理方法
- 布局样式转换辅助函数
- Paint 基类方法
- 滚动相关方法
- 内容尺寸计算方法
- LayerInfo 实现
- 属性树状态方法

### 2.3 render_block.cpp (新建, 目标 ~1200 行)

```cpp
// render_block.cpp
#include "render_object.h"
#include "painters/form_element_painter.h"
// ... 其他必要的 include

namespace lightui {

void RenderBlock::Layout(float parent_width, float parent_height) {
    // 从 render_object.cpp 迁移
}

void RenderBlock::Paint(SkCanvas* canvas) {
    // 从 render_object.cpp 迁移
    // 表单元素绘制改为调用 FormElementPainter
}

void RenderBlock::PaintContentEditableCaret(SkCanvas* canvas, Element* element, const Box& box) {
    // 从 render_object.cpp 迁移
}

// 删除 PaintInputElement 和 PaintTextAreaElement
// 改为在 Paint 中调用 FormElementPainter

} // namespace lightui
```

### 2.4 render_inline.cpp (新建, 目标 ~400 行)

```cpp
// render_inline.cpp
#include "render_object.h"
#include "painters/form_element_painter.h"

namespace lightui {

void RenderInline::Layout(float parent_width, float parent_height) {
    // 从 render_object.cpp 迁移
}

void RenderInline::PositionChildrenOnly() {
    // 从 render_object.cpp 迁移
}

std::pair<float, float> RenderInline::MeasureIntrinsicSize(float available_width) {
    // 从 render_object.cpp 迁移
}

void RenderInline::Paint(SkCanvas* canvas) {
    // 从 render_object.cpp 迁移
    // 表单元素绘制改为调用 FormElementPainter
}

// 删除 PaintInputElement 和 PaintTextAreaElement

} // namespace lightui
```

### 2.5 render_text.cpp (新建, 目标 ~350 行)

```cpp
// render_text.cpp
#include "render_object.h"

namespace lightui {

void RenderText::Layout(float parent_width, float parent_height) {
    // 从 render_object.cpp 迁移
}

void RenderText::Paint(SkCanvas* canvas) {
    // 从 render_object.cpp 迁移
}

} // namespace lightui
```

### 2.6 render_table.cpp (新建, 目标 ~1200 行)

```cpp
// render_table.cpp
#include "render_object.h"

namespace lightui {

// ========== RenderTable ==========
void RenderTable::CollectColumnStyles() { /* ... */ }
void RenderTable::CalculateColumnWidths(float available_width) { /* ... */ }
void RenderTable::Layout(float parent_width, float parent_height) { /* ... */ }
void RenderTable::Paint(SkCanvas* canvas) { /* ... */ }

// ========== RenderTableRowGroup ==========
void RenderTableRowGroup::Layout(float parent_width, float parent_height) { /* ... */ }
void RenderTableRowGroup::Paint(SkCanvas* canvas) { /* ... */ }

// ========== RenderTableRow ==========
void RenderTableRow::Layout(float parent_width, float parent_height) { /* ... */ }
void RenderTableRow::Paint(SkCanvas* canvas) { /* ... */ }

// ========== RenderTableCell ==========
void RenderTableCell::Layout(float parent_width, float parent_height) { /* ... */ }
void RenderTableCell::Paint(SkCanvas* canvas) { /* ... */ }

// ========== RenderTableCaption ==========
void RenderTableCaption::Layout(float parent_width, float parent_height) { /* ... */ }
void RenderTableCaption::Paint(SkCanvas* canvas) { /* ... */ }

} // namespace lightui
```

## 3. 重复代码删除方案

### 3.1 当前重复代码

RenderBlock 和 RenderInline 中都有：
- `PaintInputElement` (~210 行 × 2 = ~420 行)
- `PaintTextAreaElement` (~90 行 × 2 = ~180 行)

总计约 600 行重复代码。

### 3.2 统一方案

已有 `FormElementPainter` 类，修改 RenderBlock::Paint 和 RenderInline::Paint：

```cpp
// 在 Paint 方法中，替换原来的 PaintInputElement/PaintTextAreaElement 调用
if (node && node->GetNodeType() == NodeType::ELEMENT_NODE) {
    FormElementPainter form_painter(canvas);
    FormElementPaintParams form_params;
    form_params.font_family = style.font_family;
    form_params.font_size = style.font_size;
    form_params.text_color = style.color;
    
    auto element = std::static_pointer_cast<Element>(node);
    form_params.has_focus = element->HasPseudoClass("focus");
    
    auto input_element = std::dynamic_pointer_cast<HTMLInputElement>(node);
    if (input_element) {
        form_painter.PaintInputElement(input_element.get(), box, form_params);
    }

    auto textarea_element = std::dynamic_pointer_cast<HTMLTextAreaElement>(node);
    if (textarea_element) {
        form_painter.PaintTextAreaElement(textarea_element.get(), box, form_params);
    }
}
```

## 4. 正确性属性

*属性是系统在所有有效执行中应保持为真的特征或行为——本质上是关于系统应该做什么的正式声明。*

### Property 1: 功能等价性
*对于任何* 渲染操作，拆分后的代码应产生与拆分前完全相同的渲染结果
**验证: 需求 1.5, 2.4, 3.3, 4.3, 5.5**

### Property 2: 编译通过
*对于任何* 拆分操作，编译必须通过且无新增错误
**验证: 需求 6.4**

### Property 3: 文件大小约束
*对于任何* 新创建的文件，行数不应超过 1500 行
**验证: 需求 1.4, 2.3, 3.2, 4.2, 6.3**

### Property 4: 基类文件大小
*对于* render_object.cpp，拆分后行数不应超过 1200 行
**验证: 需求 6.2**

## 5. 错误处理

- 如果迁移过程中发现循环依赖，需要重新设计 include 结构
- 如果编译失败，需要检查 include 路径和前向声明
- 如果功能异常，需要回滚并分析原因

## 6. 测试策略

### 单元测试
- 验证每个新文件可以独立编译
- 验证表单元素绘制功能正常

### 集成测试
- 运行现有的示例程序验证渲染功能
- 对比拆分前后的渲染结果

### 回归测试
- 确保所有现有功能正常工作
- 检查性能是否有明显下降
