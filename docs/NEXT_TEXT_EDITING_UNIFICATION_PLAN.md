# 下一阶段计划：文本编辑命中测量统一与 IME 接入

## 背景

当前 `input fast path` 已完成主要重构，并已解决：
- 单行 input 的编辑命令统一
- 局部 dirty region 刷新
- 中英混排下 caret / selection 宽度计算不一致
- 单行 input 鼠标点击定位不准
- 焦点 outline 与示例样式的可用性问题

下一阶段建议在此基础上继续推进“文本编辑基础设施统一”，避免后续在 `input / textarea / contenteditable` 上重复修同类问题。

---

## 总目标

建立一套可复用的“文本命中测试 + 宽度测量 + 光标/选区定位”共享能力，并在此基础上正式接入 IME 事件流，为后续 `textarea` 与 `contenteditable` 复用统一编辑层做准备。

---

## 本阶段建议范围

### 1. 统一文本测量与命中测试工具

目标：
- 把当前分散在多个文件中的字符宽度测量、点击定位、前缀宽度计算收敛到共享工具
- 保证 `DrawTextWithEmoji(...)` 与测量逻辑始终一致
- 降低中文、emoji、中英混排再次出现“绘制正常但命中不准”的风险

重点覆盖：
- 单行 `input`
- `textarea`
- 未来可扩展到 `contenteditable`

建议抽取能力：
- `MeasureTextWidthWithEmoji(...)` 的统一包装
- 按 UTF-8 字符位置计算前缀宽度
- 根据 `local_x` 反推字符位置（hit test）
- password 模式下的星号宽度命中
- 文本 clip / baseline 相关辅助计算

建议优先排查文件：
- `core/event/dispatch/mouse_event_dispatcher.cpp`
- `core/render/objects/render_inline_block.cpp`
- `core/dom/elements/html_textarea_element.cpp`
- `core/render/text/text_renderer.cpp`
- `core/editing/selection_manager.cpp`

---

### 2. 正式接入 IME / composition 事件

目标：
- 把当前已经预留好的 `InputEditCommand` composition 生命周期真正接到 SDL / 平台输入事件
- 让单行 input 在输入法预编辑、上屏、取消时走统一命令层

当前已有基础：
- `StartComposition`
- `UpdateComposition`
- `CommitComposition`
- `CancelComposition`
- `InputEditingController`
- `InputEditState::composition_state`

下一步重点：
- 找到 SDL / 平台 IME 事件入口
- 翻译为统一编辑命令
- 区分预编辑文本与正式提交文本
- 确认 caret / selection / repaint 行为

建议优先排查文件：
- `core/event/dispatch/keyboard_event_dispatcher.cpp`
- SDL 事件主循环入口
- `core/dom/elements/html_input_element.cpp`
- `core/editing/input_editing_controller.cpp`

---

### 3. 评估 textarea / contenteditable 的复用接入点

目标：
- 明确哪些逻辑已经可以直接复用
- 哪些逻辑还需要抽象层补齐
- 为下一轮真正统一多行编辑做铺垫

建议输出：
- 可直接复用清单
- 必须拆分的差异点清单
- 不建议现在统一的风险点

重点关注：
- 多行换行模型
- 滚动同步
- 选择范围跨行计算
- contenteditable 的 DOM range / text node 映射

---

## 建议实施顺序

### Phase 1：文本命中与宽度测量共享化
优先级：最高

原因：
- 这是 `input / textarea / contenteditable` 的共同底座
- 先统一测量逻辑，能减少 IME 与后续编辑功能接入时的连锁 bug

### Phase 2：IME 事件正式接线
优先级：高

原因：
- 统一命令层已具备基础
- 继续推进可以尽快打通中文输入的正式生命周期

### Phase 3：textarea / contenteditable 复用设计评估
优先级：中高

原因：
- 这一步更适合建立在共享测量工具和 IME 能力稳定之后

---

## 建议的验收标准

### 文本测量共享化验收
- 单行 input 点击定位在中英混排下稳定正确
- textarea 点击定位与拖动选择继续正确
- password 模式点击定位正确
- 不再出现绘制与命中走不同宽度模型的分叉

### IME 接入验收
- 中文输入法预编辑能显示
- 上屏后文本、光标、选区状态正确
- 取消 composition 不污染正式 value
- repaint 与 caret 更新无明显异常

### 复用评估验收
- 形成明确的下一轮实施清单
- 标出可直接复用与必须拆分的模块边界

---

## 新会话建议开场语

可直接在新会话里使用：

> 请先阅读 `docs/NEXT_TEXT_EDITING_UNIFICATION_PLAN.md`，然后基于文档创建 tasks，并按阶段推进实现。执行命令请使用 windows-cmd，编译请异步执行。

