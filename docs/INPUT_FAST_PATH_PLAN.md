# Input 专用编辑快速路径计划

## 背景
当前 `input` 的文本输入链路主要复用普通控件更新路径：键盘事件进入 DOM 分发后，`HTMLInputElement` 直接修改 `value_` 并触发 `input` 事件，但文本变化后的 repaint / paint dirty 语义不稳定，容易出现“值已更新、字符上屏稍晚”的体感问题。

本计划采用“一步到位”方案：直接将 `input` 升级为框架内一级编辑组件，建立专用编辑状态、命令系统、局部失效模型和快速刷新通道。

---

## 目标
1. 单字符输入、删除、替换选区稳定在 1 帧内可见。
2. 输入后的可见反馈不再依赖普通控件 repaint 副作用。
3. 将 text / selection / caret 分层，减少高频小变化带动整段文本重绘。
4. 为 IME composition 与 textarea 复用预留稳定结构边界。

---

## 目标架构

### 1. 编辑状态层
新增 `InputEditState`，统一持有：
- `text`
- `selection_anchor` / `selection_focus`
- `caret_position`
- `composition_state`
- `dirty_flags`
- `revision_id`

### 2. 编辑命令层
所有输入行为收敛为统一命令：
- `InsertText`
- `DeleteBackward`
- `DeleteForward`
- `ReplaceSelection`
- `MoveCaretLeft/Right`
- `SelectAll`
- `SetSelection`
- `SetCaretFromMouse`
- `PasteText`
- `CutSelection`
- `Start/Update/Commit/CancelComposition`

### 3. 局部失效层
新增 `EditDirtyFlags`：
- `TextChanged`
- `SelectionChanged`
- `CaretChanged`
- `CompositionChanged`
- `StyleChanged`
- `GeometryChanged`

根据 dirty 类型计算最小 dirty region，而不是把 input 变化粗暴提升为普通控件重绘。

### 4. 快速刷新层
建立 `input-fast-path`：
- 编辑命令执行完成后，直接向所属 `Window` 和 `RenderPipeline` 提交控件级最小刷新请求。
- 保证输入可见反馈优先于普通 UI repaint。
- 只影响当前 input 所在 window 与最小必要区域。

### 5. 可视模型层
新增 `InputPaintModel`，负责派生与缓存：
- `display_text`
- `placeholder/password mask`
- 文本布局结果
- `selection_rects`
- `caret_rect`
- `composition_rects`

### 6. 绘制层
将当前 input 绘制拆为：
- `Text layer`
- `Selection layer`
- `Caret layer`

`FormElementPainter` 只消费 `InputPaintModel`，不再承担编辑推导逻辑。

---

## 文件级改造清单

### 需要新增
- `core/editing/input_edit_state.h/.cpp`
- `core/editing/input_edit_command.h`
- `core/editing/input_editing_controller.h/.cpp`
- `core/render/input/input_paint_model.h/.cpp`
- （可选）`core/render/input/input_fast_path.h/.cpp`

### 需要重构
- `core/dom/elements/html_input_element.h/.cpp`
  - 从“直接编辑 + 直接刷新”改为“DOM 壳 + 编辑控制器接入层”。
- `core/event/dispatch/keyboard_event_dispatcher.cpp`
  - 将 input 的键盘/文本事件翻译为编辑命令。
- `core/render/painters/form_element_painter.cpp`
  - 改为基于 `InputPaintModel` 的三层绘制。
- `core/window/window.cpp`
  - 接入 input-fast-path repaint request。
- `core/render/pipeline/render_pipeline.cpp`
  - 支持控件级 dirty region 与更细粒度 paint 请求。

### 暂不直接改造但需兼容
- `core/dom/elements/html_textarea_element.cpp`
- `core/editing/contenteditable_handler.cpp`

---

## 实施步骤

### 阶段 1：编辑状态与命令统一
1. 建立 `InputEditState`。
2. 建立 `EditDirtyFlags`。
3. 建立 `InputEditCommand`。
4. 将 `HandleTextInput / HandleKeyPress / 鼠标定位 / 剪贴板操作` 收敛到统一命令执行流。

### 阶段 2：快速刷新与局部失效
1. 设计 input-fast-path repaint request。
2. 建立 input 局部 dirty region 规则。
3. 将最小刷新请求接入 `Window` / `RenderPipeline`。

### 阶段 3：可视模型与三层绘制
1. 建立 `InputPaintModel`。
2. 拆分 `Text/Selection/Caret` 三层绘制。
3. 将 painter 改为纯绘制消费层。

### 阶段 4：IME 与复用边界
1. 纳入 composition state 与命令。
2. 预留 composition 可视规则。
3. 定义向 textarea 复用的共享边界。

### 阶段 5：编译运行与体验验收
1. 编译通过。
2. 运行后人工验证：连续输入、删除、替换选区、剪贴板、鼠标定位、caret blink。
3. 后续补充 IME 场景验证。

---

## 验收指标
- `keydown/textinput -> edit state update`
- `edit state update -> repaint request`
- `repaint request -> render start`
- `render start -> present/swap`

### 目标阈值
- 单字符输入稳定 1 帧内可见。
- 不出现“逻辑已改值但屏幕未刷新”的空窗。
- 连续输入节拍稳定，不因普通 UI 更新明显抖动。

---

## 风险与边界
1. 这是输入子系统重构，不是补丁级修复。
2. 需重点防止回归：placeholder、password、选择范围、剪贴板、焦点时序、受控 input。
3. 当前只聚焦单行 `input`，不直接扩大战线到 `contentEditable`。

---

## 建议执行顺序
先完成：状态模型 → 命令系统 → 快速刷新 → 局部失效。
再完成：可视模型 → 三层绘制 → IME 预留 → textarea 复用边界。
最后执行：编译、运行、人工交互验收。

---

## 更严格的文件级实施清单

### `core/dom/elements/html_input_element.h/.cpp`
**改造目标**
- 从直接编辑 `value_ / selection_*` 的实现，收缩为 DOM 壳与编辑控制器接入层。
- 保留对外 DOM API、属性同步、focus/blur、事件入口。

**需要迁出的职责**
- 文本插入、删除、替换选区的具体状态变更。
- selection/caret 计算后的 dirty 判定。
- 文本变更后的 repaint 决策。

**需要新增的连接点**
- 持有或访问 `InputEditState`。
- 持有 `InputEditingController`。
- 将 `HandleTextInput`、`HandleKeyPress`、鼠标定位等转发为 `InputEditCommand`。

**验证关注点**
- `value` getter/setter 与内部编辑状态保持一致。
- placeholder、password、selectionStart/End、focus 时序不回归。

### `core/event/dispatch/keyboard_event_dispatcher.cpp`
**改造目标**
- 将聚焦 input 时的 key/text/mouse 动作翻译为统一编辑命令。
- 明确“普通 DOM 键盘事件派发”和“input 默认编辑行为”的分层。

**关键改造点**
- `SDL_EVENT_TEXT_INPUT` -> `InsertText` / composition commit。
- Backspace/Delete -> `DeleteBackward/DeleteForward`。
- Arrow/Home/End/Shift 组合 -> caret/selection 命令。
- Ctrl+A/C/X/V -> 选择与剪贴板命令。

**验证关注点**
- `beforeinput/input/change` 生命周期不乱序。
- `preventDefault` 与默认编辑行为边界清晰。

### `core/editing/input_edit_state.h/.cpp`
**新增目标**
- 定义 `InputEditState`、`CompositionState`、`EditDirtyFlags`。
- 作为 input 编辑的单一事实来源。

**建议字段**
- `text`
- `selection_anchor / selection_focus`
- `caret_position`
- `dirty_flags`
- `revision_id`
- `composition_state`

**验证关注点**
- 插入、删除、替换、全选、鼠标定位后状态一致。
- dirty flags 能正确区分 text/selection/caret/composition 变化。

### `core/editing/input_edit_command.h`
**新增目标**
- 定义统一命令枚举/结构，承载编辑动作输入。

**建议命令**
- `InsertText`
- `DeleteBackward / DeleteForward`
- `ReplaceSelection`
- `MoveCaretLeft / MoveCaretRight`
- `SelectAll / SetSelection / SetCaretFromMouse`
- `PasteText / CutSelection`
- `Start/Update/Commit/CancelComposition`

### `core/editing/input_editing_controller.h/.cpp`
**新增目标**
- 执行编辑命令，更新 `InputEditState`，产生 dirty 与 repaint request。
- 统一发出 `beforeinput/input/change`。

**关键接口**
- `ApplyCommand(...)`
- `ComputeDirtyRegion(...)`
- `DispatchEditingEvents(...)`
- `RequestFastRepaint(...)`

**验证关注点**
- 所有编辑路径都收敛到该控制器。
- 删除/插入/替换选区拥有一致刷新语义。

### `core/render/input/input_paint_model.h/.cpp`
**新增目标**
- 将编辑状态转换为可视状态，缓存文本布局与几何结果。

**建议输出**
- `display_text`
- `selection_rects`
- `caret_rect`
- `composition_rects`
- placeholder/password mask 对应显示结果

**验证关注点**
- caret 变化不要求重建整段文本布局。
- selection 变化只影响 selection/caret 相关区域。

### `core/render/painters/form_element_painter.cpp`
**改造目标**
- 从临时推导式绘制改为消费 `InputPaintModel` 的三层绘制。

**拆分入口建议**
- `PaintInputTextLayer(...)`
- `PaintInputSelectionLayer(...)`
- `PaintInputCaretLayer(...)`

**验证关注点**
- caret blink 不重画整段文本。
- selection 高亮与文本绘制结果保持一致。

### `core/window/window.cpp`
**改造目标**
- 接收 input-fast-path repaint request，并优先处理控件级最小刷新。

**关键改造点**
- 为 input 快速路径提供请求入口。
- 在现有 `needs_repaint_` 体系上兼容局部 dirty region。
- 避免文本变化后因无 repaint 标志而提前 return。

**验证关注点**
- 单字符输入不会因快速返回条件而漏帧。
- 仅刷新所属 window，不扩大为全局窗口刷新。

### `core/render/pipeline/render_pipeline.cpp`
**改造目标**
- 识别 input 专用 paint 请求，支持更细粒度 dirty region 与最小 paint/composite。

**关键改造点**
- 接收控件级 dirty region。
- 对 text/selection/caret 变化采用不同 paint 粒度。
- 保持与现有通用 incremental rasterize 兼容。

**验证关注点**
- input 局部更新不会破坏现有通用管线。
- 连续输入时节拍稳定，无明显全量 paint 放大。

### `core/dom/elements/html_textarea_element.cpp`
**当前策略**
- 本阶段不直接重构，但要为未来复用同一套编辑状态/命令/dirty 语义留接口。

### `core/editing/contenteditable_handler.cpp`
**当前策略**
- 暂不纳入本轮实现，仅确保 input 快速路径设计不与富文本编辑模型冲突。

### 推荐改造顺序
1. `input_edit_state` / `input_edit_command` / `input_editing_controller`
2. `html_input_element` / `keyboard_event_dispatcher`
3. `input_paint_model` / `form_element_painter`
4. `window` / `render_pipeline`
5. 编译、运行、人工输入验收


