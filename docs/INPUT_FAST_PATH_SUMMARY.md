# Input Fast Path 改造总结

## 目标

基于 `docs/INPUT_FAST_PATH_PLAN.md` 完成 input 专用编辑快速路径重构，重点解决：
- 单行 input 文本编辑不再走零散逻辑
- 文本、选区、光标刷新尽量走局部 fast-path
- 绘制层与编辑状态层解耦
- 为 IME / textarea 复用预留统一命令边界

## 本轮完成内容

### 1. 编辑状态与命令层
新增：
- `core/editing/input_edit_state.h/.cpp`
- `core/editing/input_edit_command.h/.cpp`
- `core/editing/input_editing_controller.h/.cpp`

完成内容：
- 建立 `InputEditState`，统一托管：
  - `text`
  - `selection_anchor`
  - `selection_focus`
  - `caret_position`
  - `composition_state`
  - `dirty_flags`
- 建立 `InputEditCommand` 统一编辑命令模型
- `InputEditingController` 接管：
  - 插入 / 删除
  - 光标移动
  - 选择
  - 剪切 / 粘贴
  - composition 生命周期

### 2. HTMLInputElement 接线
改造：
- `core/dom/elements/html_input_element.h/.cpp`

完成内容：
- `HTMLInputElement` 持有 `InputEditState`
- 新增统一入口：
  - `ApplyEditCommand(...)`
  - `ExecuteEditCommand(...)`
  - `RequestInputRepaint()`
- `SetValue / SetChecked / SetCursorPosition / SetSelection` 已统一走 repaint 收口
- `readonly / disabled / maxlength` 校验集中到命令执行阶段

### 3. 键盘事件统一翻译
改造：
- `core/event/dispatch/keyboard_event_dispatcher.cpp`

完成内容：
- 将 input 键盘输入翻译为统一编辑命令
- 支持：
  - `Backspace / Delete`
  - `ArrowLeft / ArrowRight`
  - `Home / End`
  - `Ctrl+A / Ctrl+X / Ctrl+V`
  - `Shift + ArrowLeft / ArrowRight / Home / End`
- 修复 clipboard 事件分发后提前 `return`，导致 `Ctrl+X / Ctrl+V` 默认编辑行为失效的问题

### 4. 可视模型与绘制层
新增：
- `core/render/input/input_paint_model.h/.cpp`

改造：
- `core/render/painters/form_element_painter.h/.cpp`
- `core/render/objects/render_inline_block.cpp`

完成内容：
- 建立 `InputPaintModel`，承载：
  - `value`
  - `display_text`
  - `selection_start`
  - `selection_end`
  - `caret_position`
  - `is_placeholder`
  - `is_password`
- `FormElementPainter` 已拆分为：
  - Text layer
  - Selection layer
  - Caret layer
- 修复单行 input 在中英混排下 caret / selection 计算错误：
  - 实际生效主路径位于 `RenderInlineBlock::PaintInputElement()`
  - 将 selection / caret 宽度计算统一改为 `TextRenderer::MeasureTextWidthWithEmoji(...)`
  - 与 `DrawTextWithEmoji(...)` 保持一致，避免中文时光标落在字中间

### 5. Fast Path 与局部刷新
完成内容：
- `HTMLInputElement::RequestInputRepaint()` 已接入：
  - `MarkDirty(DirtyType::PAINT)`
  - `Window::AddDirtyRect(...)`
  - `RenderPipeline::MarkDirtyRegion(...)`
  - `RenderPipeline::MarkNeedsPaint()`
  - `Window::SetNeedsRepaint()`
- input 的文本、选区、光标变化已能走控件级局部 dirty region 刷新

### 6. IME / 复用边界
完成内容：
- `InputEditState` 增加 `HasActiveComposition()`
- `InputEditCommand` 补齐 `CancelComposition()`
- `InputEditingController` 已承接：
  - `StartComposition`
  - `UpdateComposition`
  - `CommitComposition`
  - `CancelComposition`

## 关键验收结果

本轮已完成：
- 编译通过
- 运行通过
- 人工交互验收通过

已确认修复：
- `Ctrl+X / Ctrl+V` 无效
- `Shift + 方向键 / Home / End` 不扩展选区
- 单行 input 在中英混排时光标、高亮位置错乱
- 单行 input 鼠标点击定位与实际渲染宽度不一致

补充优化：
- `mouse_event_dispatcher.cpp` 中单行 input 点击命中宽度计算已统一改为 `TextRenderer::MeasureTextWidthWithEmoji(...)`
- `RenderInlineBlock` 中单行 input 文本基线与裁剪区域已微调，减少中文字符在较紧凑单行输入框中的裁剪
- `style_resolver.cpp` 中 `:focus / :focus-visible` outline 已调整为更接近 Chromium 的半透明 focus ring
- `modern_desktop_demo` 示例 input 已调整默认内容、padding、边框与字号，便于直接人工复验中英混排与焦点样式场景

## 主要变更文件

- `core/editing/input_edit_state.h/.cpp`
- `core/editing/input_edit_command.h/.cpp`
- `core/editing/input_editing_controller.h/.cpp`
- `core/dom/elements/html_input_element.h/.cpp`
- `core/event/dispatch/keyboard_event_dispatcher.cpp`
- `core/render/input/input_paint_model.h/.cpp`
- `core/render/painters/form_element_painter.h/.cpp`
- `core/render/objects/render_inline_block.cpp`

## 后续建议

- 将单行 input 与 textarea 的字符命中测试、前缀宽度计算继续下沉为共享工具
- 后续接 SDL / 平台 IME 事件时，优先继续复用 `InputEditCommand`
- 若继续推进 textarea / contenteditable，可复用当前编辑状态层与命令层抽象

