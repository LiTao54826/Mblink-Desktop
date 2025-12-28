# 大文件拆分需求文档

## 简介

本文档定义了对超过 2000 行的大型源文件进行拆分重构的需求。目标是将这些文件拆分为更小、更专注的模块，提高代码可维护性和可读性。

## 术语表

- **EventLoop**: 主事件循环类，负责处理 SDL 事件和分发到 DOM
- **RenderObject**: 渲染对象类，负责元素的布局和绘制
- **Dispatcher**: 事件分发器，负责将特定类型的事件分发到目标

## 需要拆分的文件

| 文件 | 行数 | 优先级 |
|------|------|--------|
| render_object.cpp | 5360 | P1 |
| event_loop.cpp | 3920 | P1 |
| native_layout_engine.cpp | 3804 | P2 |
| window.cpp | 3368 | P2 |
| style_resolver.cpp | 3068 | P2 |
| dom_bindings.cpp | 3009 | P3 |
| contenteditable_handler.cpp | 2519 | P3 |

## 需求

### 需求 1: event_loop.cpp 拆分

**用户故事:** 作为开发者，我希望 event_loop.cpp 的事件处理逻辑被拆分到独立的分发器类中，以便更容易理解和维护。

#### 验收标准

1. WHEN 鼠标事件发生 THEN MouseEventDispatcher SHALL 处理所有鼠标相关逻辑
2. WHEN 键盘事件发生 THEN KeyboardEventDispatcher SHALL 处理所有键盘相关逻辑
3. WHEN 滚轮事件发生 THEN WheelEventDispatcher SHALL 处理所有滚轮相关逻辑
4. WHEN 拆分完成 THEN event_loop.cpp SHALL 减少至少 1500 行代码
5. WHEN 拆分完成 THEN 所有现有功能 SHALL 保持不变

### 需求 2: render_object.cpp 拆分

**用户故事:** 作为开发者，我希望 render_object.cpp 的绘制逻辑被拆分到独立的渲染器类中，以便更容易扩展和维护。

#### 验收标准

1. WHEN 绘制背景 THEN BackgroundPainter SHALL 处理背景绘制逻辑
2. WHEN 绘制边框 THEN BorderPainter SHALL 处理边框绘制逻辑
3. WHEN 绘制滚动条 THEN ScrollbarPainter SHALL 处理滚动条绘制逻辑
4. WHEN 绘制表单元素 THEN FormElementPainter SHALL 处理表单元素绘制逻辑
5. WHEN 拆分完成 THEN render_object.cpp SHALL 减少至少 2000 行代码
6. WHEN 拆分完成 THEN 所有现有功能 SHALL 保持不变

### 需求 3: window.cpp 拆分

**用户故事:** 作为开发者，我希望 window.cpp 的渲染和事件处理逻辑被拆分，以便更容易理解窗口管理。

#### 验收标准

1. WHEN 渲染窗口内容 THEN WindowRenderer SHALL 处理渲染逻辑
2. WHEN 处理窗口事件 THEN WindowEventHandler SHALL 处理事件逻辑
3. WHEN 拆分完成 THEN window.cpp SHALL 减少至少 1000 行代码
4. WHEN 拆分完成 THEN 所有现有功能 SHALL 保持不变

### 需求 4: 通用拆分原则

**用户故事:** 作为开发者，我希望所有拆分遵循一致的原则，以保持代码风格统一。

#### 验收标准

1. WHEN 创建新类 THEN 新类 SHALL 遵循单一职责原则
2. WHEN 创建新文件 THEN 新文件 SHALL 不超过 1500 行
3. WHEN 拆分代码 THEN 原有的公共 API SHALL 保持不变
4. WHEN 拆分代码 THEN 编译 SHALL 通过且无新增错误
5. WHEN 拆分代码 THEN 所有测试 SHALL 通过
