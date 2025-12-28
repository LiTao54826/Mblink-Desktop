# render_object.cpp 拆分需求文档

## 简介

本文档定义了对 render_object.cpp (5016 行) 进行深度拆分重构的需求。目标是将这个超大文件拆分为多个专注的模块，每个模块不超过 1500 行，提高代码可维护性和可读性。

## 术语表

- **RenderObject**: 渲染对象基类，所有渲染对象的父类
- **RenderBlock**: 块级渲染对象，处理块级元素的布局和绘制
- **RenderInline**: 行内渲染对象，处理行内元素的布局和绘制
- **RenderText**: 文本渲染对象，处理文本节点的布局和绘制
- **RenderTable**: 表格渲染对象，处理表格元素的布局和绘制
- **Painter**: 绘制器，负责特定类型内容的绘制

## 当前文件结构分析

| 代码段 | 行数范围 | 估计行数 | 描述 |
|--------|----------|----------|------|
| RenderObject 基类 | 1-1072 | ~1070 | 基类实现、滚动、内容尺寸计算 |
| RenderBlock | 1073-2748 | ~1675 | 块级元素布局和绘制 |
| RenderInline | 2749-3384 | ~635 | 行内元素布局和绘制 |
| RenderText | 3385-3715 | ~330 | 文本节点布局和绘制 |
| RenderTable 系列 | 3716-4840 | ~1125 | 表格相关类实现 |
| LayerInfo + 属性树 | 4841-5016 | ~175 | 层信息和属性树方法 |

## 需求

### 需求 1: 提取 RenderBlock 到独立文件

**用户故事:** 作为开发者，我希望 RenderBlock 的实现在独立文件中，以便更容易理解和维护块级元素的渲染逻辑。

#### 验收标准

1. WHEN 创建 render_block.cpp THEN 系统 SHALL 包含 RenderBlock 类的所有方法实现
2. WHEN 创建 render_block.cpp THEN 系统 SHALL 包含 PaintInputElement 和 PaintTextAreaElement 方法
3. WHEN 创建 render_block.cpp THEN 系统 SHALL 包含 PaintContentEditableCaret 方法
4. WHEN 拆分完成 THEN render_block.cpp SHALL 不超过 1500 行
5. WHEN 拆分完成 THEN 所有现有功能 SHALL 保持不变

### 需求 2: 提取 RenderInline 到独立文件

**用户故事:** 作为开发者，我希望 RenderInline 的实现在独立文件中，以便更容易理解和维护行内元素的渲染逻辑。

#### 验收标准

1. WHEN 创建 render_inline.cpp THEN 系统 SHALL 包含 RenderInline 类的所有方法实现
2. WHEN 创建 render_inline.cpp THEN 系统 SHALL 包含 PaintInputElement 和 PaintTextAreaElement 方法
3. WHEN 拆分完成 THEN render_inline.cpp SHALL 不超过 800 行
4. WHEN 拆分完成 THEN 所有现有功能 SHALL 保持不变

### 需求 3: 提取 RenderText 到独立文件

**用户故事:** 作为开发者，我希望 RenderText 的实现在独立文件中，以便更容易理解和维护文本渲染逻辑。

#### 验收标准

1. WHEN 创建 render_text.cpp THEN 系统 SHALL 包含 RenderText 类的所有方法实现
2. WHEN 拆分完成 THEN render_text.cpp SHALL 不超过 500 行
3. WHEN 拆分完成 THEN 所有现有功能 SHALL 保持不变

### 需求 4: 提取 RenderTable 系列到独立文件

**用户故事:** 作为开发者，我希望表格相关的渲染类在独立文件中，以便更容易理解和维护表格渲染逻辑。

#### 验收标准

1. WHEN 创建 render_table.cpp THEN 系统 SHALL 包含 RenderTable、RenderTableRowGroup、RenderTableRow、RenderTableCell、RenderTableCaption 类的所有方法实现
2. WHEN 拆分完成 THEN render_table.cpp SHALL 不超过 1500 行
3. WHEN 拆分完成 THEN 所有现有功能 SHALL 保持不变

### 需求 5: 删除重复的表单元素绘制代码

**用户故事:** 作为开发者，我希望删除 RenderBlock 和 RenderInline 中重复的 PaintInputElement/PaintTextAreaElement 方法，统一使用 FormElementPainter。

#### 验收标准

1. WHEN 删除重复代码 THEN RenderBlock::PaintInputElement SHALL 被删除或改为调用 FormElementPainter
2. WHEN 删除重复代码 THEN RenderBlock::PaintTextAreaElement SHALL 被删除或改为调用 FormElementPainter
3. WHEN 删除重复代码 THEN RenderInline::PaintInputElement SHALL 被删除或改为调用 FormElementPainter
4. WHEN 删除重复代码 THEN RenderInline::PaintTextAreaElement SHALL 被删除或改为调用 FormElementPainter
5. WHEN 删除重复代码 THEN 所有表单元素绘制功能 SHALL 保持不变

### 需求 6: 最终文件大小目标

**用户故事:** 作为开发者，我希望拆分后的 render_object.cpp 只包含基类实现，大小合理。

#### 验收标准

1. WHEN 拆分完成 THEN render_object.cpp SHALL 只包含 RenderObject 基类实现
2. WHEN 拆分完成 THEN render_object.cpp SHALL 不超过 1200 行
3. WHEN 拆分完成 THEN 所有新文件 SHALL 不超过 1500 行
4. WHEN 拆分完成 THEN 编译 SHALL 通过且无新增错误
5. WHEN 拆分完成 THEN 所有测试 SHALL 通过
