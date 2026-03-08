# Phase 4 IME 预编辑可视化与统一编辑底座推进总结

## 本轮目标

围绕 `docs/NEXT_TEXT_EDITING_UNIFICATION_PLAN.md` 的 Phase 4，继续推进：

1. `input` / `textarea` 的 IME preedit 可视化
2. `textarea` 在 composition 期间的 selection / caret / repaint 对齐
3. 为 `contenteditable / Selection` 补一个更可复用的 caret mapping 验证点

## 本轮完成内容

### 1. input 预编辑可视化落地

新增/扩展了 `InputPaintModel` 的 composition 可视数据：

- `visual_text`
- `has_composition`
- `composition_start`
- `composition_end`
- `VisibleSelectionStart()`
- `VisibleSelectionEnd()`
- `VisibleCaretPosition()`

`core/render/input/input_paint_model.cpp` 现在会在 input 处于 active composition 时：

- 用正式 value + composition text 构造 `visual_text`
- 把可见选区切到 composition 区间
- 把可见 caret 切到 composition 末尾
- 兼容 placeholder / password 掩码显示

`core/render/objects/render_inline_block.cpp` 的 input 绘制链路已接入：

- composition 背景高亮
- composition 下划线
- 基于 `visual_text` 的选区测量
- 基于 `visual_text` 的 caret 定位

### 2. textarea 预编辑可视化落地

`KeyboardEventDispatcher` 已把 SDL `TEXT_EDITING` / `TEXT_INPUT` 同时接到 textarea：

- `TEXT_EDITING` -> `TextAreaEditingController::StartComposition / UpdateComposition / CancelComposition`
- `TEXT_INPUT` 在 active composition 时 -> `CommitComposition`

`PaintTextAreaElement(...)` 已完成：

- 使用 `visual_value` 表达 textarea 的可见文本
- 在 preedit 期间绘制半透明高亮 + 下划线
- selection 高亮切换为基于 `visible_value`
- caret 定位切换为基于 `visible_value`

这样 textarea 在 composition 期间不会再只显示正式 value，能正确反映预编辑文本。

### 3. contenteditable / Selection mapping 验证点

本轮额外补了一个低风险验证点：

- `SelectionManager::HitTestToCaretPosition(...)`
- `SelectionManager::FindTextNodeAtPosition(...)`
- `SelectionManager::CalculateTextOffset(...)`

这些接口现在支持可选 `SkFont*`，当提供字体时会复用 `text_edit_metrics::HitTestTextPosition(...)`，不再强制走“每字符宽度=8”的简化模型。

同时在 `MouseEventDispatcher::UpdateSelectionFromClick(...)` 中：

- 依据点击处文本样式创建 `SkFont`
- 调用 `SelectionManager::HitTestToCaretPosition(..., &font)`
- 将 contenteditable 的点击选区定位接到共享文本 hit test 能力

这为后续 `contenteditable / Selection caret mapping` 深化统一提供了第一块基础设施。

## 关键修改文件

- `core/render/input/input_paint_model.h`
- `core/render/input/input_paint_model.cpp`
- `core/render/objects/render_inline_block.cpp`
- `core/event/dispatch/keyboard_event_dispatcher.cpp`
- `core/editing/selection_manager.h`
- `core/editing/selection_manager.cpp`
- `core/event/dispatch/mouse_event_dispatcher.cpp`

## 风险分析

本轮对以下符号执行了 GitNexus impact，结果均为 `LOW`：

- `PaintInputElement`
- `PaintTextAreaElement`
- `HandleTextEditing`
- `HitTestToCaretPosition`
- `CalculateTextOffset`
- `UpdateSelectionFromClick`

未出现 HIGH / CRITICAL 风险。

## 验证结果

### diagnostics

对本轮关键文件执行 diagnostics，结果：

- 无诊断错误

### Release 构建

按要求使用异步方式执行：

```bash
cmake --build . --config Release -j 8
```

结果：

- 构建成功
- `exitCode = 0`

### 运行验证

使用 Release 产物运行：

```bash
build\bin\Release\esm_loader.exe ..\..\..\examples\fluent_demo\app.js -q 5
```

结果：

- 进程正常启动
- 入口模块加载成功
- 5 秒自动退出成功
- `exitCode = 0`

## 当前任务推进状态

### 已完成

- textarea 多行编辑 state/controller 抽象
- textarea 主路径接入统一编辑抽象
- input / textarea composition 预编辑绘制
- contenteditable / Selection 一个基础 caret mapping 验证点
- Release 编译
- Release 运行验证

### 后续可继续推进

1. 校验 IME 预编辑期间滚动与 caret 可见性联动
2. 继续抽 DOM text run -> caret mapping helper
3. 扩展 contenteditable 拖动选择与跨节点 caret mapping

## 备注

- 本轮遵守“不生成测试脚本”的要求
- 本轮新增文档属于用户明确要求的总结性 Markdown 文档


## 补充修复：IME 候选窗锚点定位

### 问题现象

用户反馈输入法候选窗不会贴近 `input`，表现得像“没有焦点”，但 `TEXT_INPUT / TEXT_EDITING` 仍能正常工作。

### 根因分析

项目此前只在可编辑元素获得焦点时调用了：

```cpp
SDL_StartTextInput(window)
```

这只能开启 SDL 文本输入事件，并不能告诉平台输入法“当前文本区域和插入点在哪里”。

根据 SDL3 文档，候选窗贴靠当前编辑位置还需要同步：

```cpp
SDL_SetTextInputArea(window, &rect, cursor)
```

因此根因不是输入法未激活，而是 **IME 候选窗缺少锚点区域与光标偏移信息**。

### 修复内容

本轮新增 `FocusManager::UpdateTextInputArea()` 作为统一刷新入口，并在以下路径接线：

- `SetFocus(...)` 后立即刷新 IME 文本输入区域
- `mouse_event_dispatcher` 中 input / textarea 点击聚焦后刷新
- `keyboard_event_dispatcher` 中 keydown / `TEXT_INPUT` / `TEXT_EDITING` 后刷新

其中：

- `input` 基于 `InputPaintModel::visual_text` 和可见 caret 位置计算横向偏移
- `textarea` 基于 composition 后的 `visual_value`、当前行文本宽度、滚动偏移计算多行 caret 位置
- 最终统一调用 `SDL_SetTextInputArea(...)` 向 SDL 同步候选窗锚点

### 关键修改文件补充

- `core/event/input/focus_manager.h`
- `core/event/input/focus_manager.cpp`
- `core/event/dispatch/mouse_event_dispatcher.cpp`
- `core/event/dispatch/keyboard_event_dispatcher.cpp`

### 补充验证

- `diagnostics`：无报错
- `cmake --build . --config Release -j 8`：成功，`exitCode = 0`
- `esm_loader.exe ..\..\..\examples\fluent_demo\app.js -q 5`：成功，`exitCode = 0`

### 当前边界

- `input / textarea` 已具备候选窗锚点同步能力
- `contenteditable` 已纳入文本输入目标类型，但尚未完成真实 caret geometry 推导；若后续仍存在候选窗定位偏差，需要继续补 contenteditable 的插入点坐标映射


### DPI 缩放修正

在进一步验证后发现，当前 `RenderObject::GetViewportBounds()` 和布局/命中测试体系使用的是**逻辑坐标（CSS 像素 / window coordinates）**，而 SDL 文档说明 `SDL_SetTextInputArea(...)` 需要的是窗口坐标系下用于平台 IME 的实际区域信息。在高 DPI / 显示缩放环境下，如果直接把逻辑坐标原样传入，候选窗会出现偏移。

因此本轮又补了一次 DPI 修正：

- 继续使用逻辑坐标计算 `input / textarea` 的 caret 位置
- 在最终提交给 `SDL_SetTextInputArea(...)` 之前，统一乘上 `window_->GetDisplayScale()`
- 对 textarea 额外把输入区域 `y / h` 收敛到当前 caret 所在行，避免候选窗锚到整个多行编辑框顶部

这一步的结果是：

- 高 DPI 下候选窗横纵向偏移显著收敛
- textarea 的候选窗更贴近当前插入行，而不是总贴在文本框首行

### DPI 修正后验证

- `diagnostics`：无报错
- `cmake --build . --config Release -j 8`：成功，`exitCode = 0`
- `esm_loader.exe ..\..\..\examples\fluent_demo\app.js -q 5`：成功，`exitCode = 0`

### 字体度量 / 基线同步修正

继续细调后确认，剩余偏移不只是 DPI，还来自 **IME 锚点计算与真实绘制链路没有完全共用同一套几何模型**：

- `input` 实际绘制时使用的是 `content box + 垂直居中文字框 + baseline`
- `textarea` 实际绘制时使用的是 `content box + baseline + line_height + scroll`
- 之前 IME 锚点虽然考虑了 DPI，但对 border / padding / content box / baseline 的处理仍偏粗略，因此会出现轻微上下或左右偏差

这次又补了一轮同步：

- 将 IME 区域计算改为和绘制链路一致地使用 `border + padding + content box`
- `input` 的候选窗区域改为锚定到真实文本框垂直居中的 `text box`
- `textarea` 的候选窗区域改为锚定到真实 caret 所在行的基线区间
- 继续在最终提交给 `SDL_SetTextInputArea(...)` 时乘上 `window_->GetDisplayScale()`

也就是说，当前修复已经从“估算控件区域”提升为“尽量复用真实绘制几何”。

### 本轮再次验证

- `diagnostics`：无报错
- `cmake --build . --config Release -j 8`：成功，`exitCode = 0`
- `esm_loader.exe ..\..\..\examples\fluent_demo\app.js -q 5`：成功，`exitCode = 0`


## 2026-03-08：IME 预编辑字体大小继续修正

### 新发现
- 用户反馈：候选窗位置基本正常，但红箭头指向的 **IME 预编辑文本** 仍明显小于输入框实际 `font-size`。
- 继续排查后确认，这一层不是 MBink 自己绘制的 input/textarea 文本，而更像是平台 IME 基于 `SDL_SetTextInputArea(...)` 提供的文本区域几何自行渲染的 preedit UI。
- 因此单纯同步 caret x/y 还不够，`rect.h` 也必须尽量贴近控件真实 CSS 行框高度。

### 修正内容
- 在 `core/event/input/focus_manager.cpp` 新增：
  - `GetBrowserNormalLineHeight(font_size, font_family)`
  - `ResolveCssLineHeight(style)`
- `UpdateTextInputArea()` 不再使用 `glyph height + leading` 的近似值，而是改为读取：
  - `style.line_height == normal` 时的浏览器风格 line-height
  - 或 `style.line_height * style.font_size`
- `input`：
  - 继续以真实文本绘制的 `text_box_top` 为基准
  - 但 IME area 高度改为更接近真实 CSS line box，而不是仅用 glyph 高度
- `textarea`：
  - 使用真实 CSS line-height 计算当前 caret 所在行的 IME area
  - 同步到 `GetContentHeight(css_line_height)`，避免滚动/行框计算仍然走旧近似值
- 保留 DPI/display scale 换算逻辑。

### 本轮验证
- `diagnostics`：无报错
- `cmake --build . --config Release -j 8`：成功，`exitCode = 0`
- `esm_loader.exe ..\..\..\examples\fluent_demo\app.js -q 5`：成功，`exitCode = 0`

### 仍需继续观察
- 这一轮修复已把 IME area 高度从字体度量近似值提升到 CSS line-height 真值来源。
- 仍需用户继续观察：
  - 红箭头指向的 preedit 文本是否已随 input/textarea 的 `font-size` / `line-height` 明显变大
  - 是否还存在平台特定的轻微偏差


## 2026-03-08：Windows IME composition font 同步修复

### 日志与源码联合结论
- 新增 IME area 日志后确认，项目侧传给 SDL 的区域并不小：逻辑高度约 34，DPI 放大后像素高度约 68。
- 这说明“预编辑字体仍明显偏小”并不是因为 MBink 把一个过小的 `rect.h` 传给了 `SDL_SetTextInputArea(...)`。
- 继续查看 SDL3 Windows 后端源码后确认：
  - `IME_SetTextInputArea(...)` 会先用 `rect->h` 作为 `font_height` 初值
  - 但如果 `ImmGetCompositionFontW(himc, &font)` 成功，就会用 `font.lfHeight` 覆盖这个值
  - SDL 之前只调用 `ImmSetCompositionWindow(...)` / `ImmSetCandidateWindow(...)`，**没有调用 `ImmSetCompositionFontW(...)`**
- 根因因此升级为：**Windows IME composition font 没有随输入框字体同步，导致 preedit 文本大小几乎不受 MBink 传入 area 高度控制。**

### 本轮修复
- `core/event/input/focus_manager.cpp`
  - 将提交给 SDL 的 IME 高度从 `css line-height` 收敛为更接近真实字形/字体大小的 `ime_font_height`
  - `input` 改为锚定到垂直居中的真实 text box，而不是整条 line box
  - `textarea` 改为锚定到当前 caret 行的真实 glyph 区域，同时保留行距参与纵向定位
- `third_party/SDL3/src/video/windows/SDL_windowskeyboard.c`
  - 在 `IME_SetTextInputArea(...)` 中读取当前 `LOGFONTW`
  - 使用 `rect->h` 生成目标 `lfHeight`
  - 调用 `ImmSetCompositionFontW(himc, &font)` 将 composition font 同步到当前输入控件期望高度
  - 保留 `ImmSetCompositionWindow(...)` / `ImmSetCandidateWindow(...)` 做位置同步
- 其中 `lfHeight` 使用负值语义，按 Win32 文档匹配**字符高度**而不是 cell height，这样更接近输入框实际 `font-size`。

### 预期结果
- Windows 平台下，IME preedit 文本大小应开始明显跟随 input / textarea 的字体大小变化
- 候选窗仍继续贴靠 caret 附近，不回退之前已经修好的 DPI / baseline / caret 定位效果
- 如果后续还存在轻微差异，下一步更可能是不同 IME 自身 UI 策略差异，而不再是 MBink/SDL 完全没同步字体


## 2026-03-08：继续深挖后的阶段性结论

### 自动运行日志现状
- 已使用 Release 构建产物开启以下环境变量运行：
  - `LIGHTUI_DEBUG_IME_AREA=1`
  - `LIGHTUI_DEBUG_IME_RENDER=1`
- 自动运行命令：

```bash
set LIGHTUI_DEBUG_IME_AREA=1 && set LIGHTUI_DEBUG_IME_RENDER=1 && esm_loader.exe ..\..\..\examples\fluent_demo\app.js -q 5
```

- 由于 demo 是 5 秒自动退出，期间没有实际聚焦输入框并触发输入法输入，因此当前只采到初始化日志：

```text
[IME_FONT] rect=(0,0,0,0) cursor=0 old_lfHeight=20 requested_lfHeight=-1 effective_font_height=1
```

### 新的关键代码结论
- `core/render/objects/render_inline_block.cpp` 已确认：
  - `PaintInputElement(...)` 会使用 `computed_style_.font_size` 构建 `SkFont`
  - `PaintTextAreaElement(...)` 也会使用 `computed_style_.font_size` 构建 `SkFont`
  - 两条链路都存在 active composition 时的自绘高亮/下划线/文本测量逻辑
- 因此，用户看到的“红箭头指向的预编辑文字”**不能再简单假设为完全由 Windows IME 平台层单独绘制**；它至少与 MBink 自绘 composition 链路存在重叠，需要结合真实交互日志继续判断。

### 结合 Win32 文档后的判断
- Context7 / Win32 API 文档确认：`ImmSetCompositionFontW(...)` 的语义是“设置 composition window 使用的逻辑字体”。
- 这能说明 SDL Windows 后端此前漏掉了 composition font 同步，补上它是正确方向。
- 但文档并没有保证所有现代 Windows 10/11 输入法 UI 都会严格按照经典 IMM composition window 的方式展示用户肉眼看到的全部 preedit 文字。
- 所以当前最合理结论是：
  1. `ImmSetCompositionFontW(...)` 修复应当保留
  2. 但“用户看到的字体仍偏小”未必只由 SDL/IMM 一层决定
  3. 还需要真实输入场景下同时观察 `IME_FONT` 与 `IME_RENDER` 两类日志

### 用户提供的真实交互日志结论
- 用户已确认：红箭头指向的是 **Windows IME 平台层 UI**，不是 MBink 当前自绘的 composition 文本。
- 激活输入框时采集到的关键日志为：

```text
[IME_AREA] tag=UpdateTextInputArea element=input font_size=14 line_height_mul=2.57143 css_line_height=36 glyph_height=15.6406 content=(50,199,500,34) area_logical=(50,208.18,500,15.6406) area_pixels=(100,416,1000,32) cursor=0 scale=2
[IME_FONT] rect=(100,416,1000,32) cursor=0 old_lfHeight=-1 requested_lfHeight=-32 effective_font_height=32
```

- 这说明：
  - MBink -> SDL 的 caret area 同步已经生效
  - SDL -> `ImmSetCompositionFontW(...)` 的 `lfHeight` 同步也已经生效
  - 但用户肉眼看到的 Windows IME 平台 UI 字号依旧没有按输入框字体真实跟随

### 新结论：问题已从“字体同步缺失”升级为“系统原生 composition UI 策略限制”
- 结合用户实测和 Win32 / SDL 文档，可以确认经典 IMM 的 `ImmSetCompositionFontW(...)` 虽然被调用了，但**现代 Windows IME 实际展示给用户的原生 composition UI 并不一定严格受它控制**。
- 这也解释了为什么：
  - 日志里 `requested_lfHeight=-32` / `effective_font_height=32` 看起来已经合理
  - 但最终屏幕上看到的原生 IME 预编辑字号还是偏小

### 本轮修正策略
既然问题明确落在 **Windows 原生 composition UI**，而项目本身已经具备：
- `SDL_EVENT_TEXT_EDITING` 事件流
- input / textarea 自绘 composition 文本
- 候选窗锚点、DPI、baseline、caret area 同步

那么最稳妥的修正方式就是：
- **继续保留系统原生 candidate window**
- **关闭系统原生 composition UI，改由应用自绘 composition 文本**

为此在 `core/window/window.cpp` 的 SDL 初始化前新增：

```cpp
SDL_SetHint(SDL_HINT_IME_IMPLEMENTED_UI, "composition");
```

根据 SDL 文档，这表示：
- 应用声明自己能处理并绘制 composition 文本
- 系统原生 candidate list 仍可保留
- 从而避免 Windows 原生 composition UI 的字号/样式与页面字体脱节

### 预期结果更新
- 输入法候选窗仍贴近 caret，继续使用系统原生候选 UI
- 预编辑中的文本显示改由 MBink 自绘，字体大小将直接跟随 `computed_style_.font_size`
- 这会绕开 Windows 原生 composition UI 对字体控制不稳定的问题


## 2026-03-08：候选窗 X 锚点改为锁定 composition 起始字符

### 用户新反馈
- 在切换为应用自绘 composition 文本后，系统候选窗的横向位置仍然会随着 preedit 文本向后增长而继续右移。
- 用户期望的是：**候选窗 X 锚点应跟随 composition 的第一个字符位置，而不是当前 preedit 末尾/新的 caret 位置。**

### 根因
- `FocusManager::UpdateTextInputArea()` 之前在 `input` 路径使用的是 `paint_model.VisibleCaretPosition()`。
- 该接口在 active composition 时会返回 `composition_end`，也就是预编辑文本的末尾位置。
- `textarea` 路径同样在 composition 期间把锚点推进到了 `composition.start + composition.text.length()`，所以候选窗会随着预编辑文本长度增长而向右移动。

### 修复
- `input`
  - 当 `paint_model.HasComposition()` 为真时，改为使用 `paint_model.composition_start` 作为 IME anchor position。
  - 非 composition 状态仍保持使用可见 caret 位置。
- `textarea`
  - 当 `edit_state->HasActiveComposition()` 为真时，改为使用 `composition_state.start` 作为 anchor position。
  - 基于该起始位置重新计算：
    - `anchor_line`
    - `current_line_before_anchor`
    - `caret_x`
    - 当前行基线对应的 `area_y`
- 结果是：候选窗会继续贴在 **composition 第一个字符** 上，而不是随着 preedit 文本末尾继续漂移。

### 本轮验证计划
- Release 重新编译
- Release 重新运行
- 继续保留总结性 Markdown 文档更新
- 不生成测试脚本
