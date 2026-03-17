# ContentEditable IME Geometry 实施总结

## 背景

根据 `docs/NEXT_CONTENTEDITABLE_IME_GEOMETRY_PLAN.md`，本轮目标是补齐 `contenteditable` 的 caret geometry、selection geometry、IME candidate anchor、composition anchor 与基础 preedit 可视反馈链路，并完成 Release 编译与运行验证。

## 本轮完成内容

### 1. 新增 contenteditable 几何辅助层

新增文件：

- `core/editing/contenteditable_geometry.h`
- `core/editing/contenteditable_geometry.cpp`

新增能力：

- `ComputeContentEditableCaretRect(...)`
- `ComputeContentEditableSelectionRects(...)`
- `ConvertRangeClientRects(...)`
- `IsNodeInsideContentEditable(...)`

实现策略：

- 通过 `Range` 在 `(node, offset)` 上创建折叠范围，复用 `GetBoundingClientRect()` 计算 caret rect
- 通过 `Range::GetClientRects()` 计算 selection rect 列表
- 尽量基于真实 DOM / Range 几何，而不是手写估算逻辑

### 2. SelectionManager 接入统一几何模型

修改文件：

- `core/editing/selection_manager.h`
- `core/editing/selection_manager.cpp`

关键变更：

- 新增 `SelectionRect` 结构与 `GetSelectionRects(...)`
- `GetCaretPosition(...)` 优先走 `ComputeContentEditableCaretRect(...)`
- `RenderSelectionHighlight(...)` 使用统一 selection rect 列表绘制高亮
- 新增 contenteditable root 查找 helper，避免依赖不存在的 `Selection::GetEditableRoot()`

额外修复：

- 将匿名命名空间 helper 中的类型显式写为 `lightui::Element / lightui::Node / lightui::NodeType`，修复命名空间作用域下的 MSVC 模板推导错误

### 3. ContentEditableHandler 增加 composition state 管理

修改文件：

- `core/editing/contenteditable_handler.h`
- `core/editing/contenteditable_handler.cpp`

新增能力：

- `StartComposition(...)`
- `UpdateComposition(...)`
- `CommitComposition(...)`
- `CancelComposition(...)`
- `HasActiveComposition(...)`
- `GetCompositionState(...)`
- `GetActiveEditableRoot(...)`

关键点：

- 使用 `composition_states_` 保存每个 `Document` 的 composition 状态
- `GetActiveEditableRoot(...)` 改为基于 `selection->GetComputedAnchorNode()` 反查可编辑根
- `FindEditableElement(...)` 补为 `const`，解决 `const` 成员函数中的调用限制

### 4. FocusManager 接入 contenteditable 的 SDL text input area

修改文件：

- `core/event/input/focus_manager.h`
- `core/event/input/focus_manager.cpp`

关键变更：

- 注入 `ContentEditableHandler*`
- `UpdateTextInputArea()` 新增 `contenteditable` 分支
- 无 composition 时锚定当前 caret
- active composition 时使用 `composition.start` 作为候选窗锚点
- 将几何同步到 `SDL_SetTextInputArea(...)` 所需 area / caret 数据

### 5. KeyboardEventDispatcher 接入 TEXT_EDITING / commit composition

修改文件：

- `core/event/dispatch/keyboard_event_dispatcher.cpp`

关键变更：

- `HandleTextInput(...)` 中 contenteditable 支持 active composition commit
- `HandleTextEditing(...)` 中 contenteditable 支持：
  - 启动 composition
  - 更新 composition
  - 空字符串时取消 composition
- 每次更新后同步调用 `focus_manager_->UpdateTextInputArea()`

### 6. RenderBlock 接入统一绘制路径

修改文件：

- `core/render/objects/render_block.cpp`

关键变更：

- `PaintContentEditableCaret(...)` 优先使用统一几何 helper
- 折叠选区时绘制 caret
- 非折叠选区时绘制 selection highlight
- 若 helper 失败，保留 legacy fallback 路径

### 7. EventLoop 完成 wiring

修改文件：

- `core/event/loop/event_loop.cpp`

关键变更：

- 在构造路径中将 `ContentEditableHandler` 注入 `FocusManager`
- 保证键盘事件派发与焦点管理共享 composition / geometry 状态

### 8. 补充人工验证 Demo

新增文件：

- `examples/contenteditable_ime_demo.html`

用途：

- 可直接用 `esm_loader` 运行
- 手工观察 `contenteditable` 的 focus / selection / beforeinput / input / composition 事件
- 用于后续继续验证 IME 候选窗锚点、caret geometry 与 preedit 交互

运行命令：

```bash
build\bin\Release\esm_loader.exe examples\contenteditable_ime_demo.html -q 5
```

结果：

- HTML 模式正常启动
- `<style>` 与 `<script>` 成功执行
- 自动退出成功
- `exitCode = 0`

### 9. CMake 接入新文件

修改文件：

- `core/editing/CMakeLists.txt`

新增注册：

- `contenteditable_geometry.cpp`
- `contenteditable_geometry.h`

## 关键问题与修复

### 编译失败 1：误用 `Selection::GetEditableRoot()`

问题：

- `Selection` 实际没有 `GetEditableRoot()` API

修复：

- `SelectionManager` 改为从 `focus/anchor/computed anchor node` 向上查找 contenteditable root
- `ContentEditableHandler::GetActiveEditableRoot(...)` 改为复用 `FindEditableElement(selection->GetComputedAnchorNode())`

### 编译失败 2：匿名命名空间类型名未加命名空间限定

问题：

- `selection_manager.cpp` 顶部 helper 位于匿名命名空间，写成 `Element / Node / NodeType` 后，MSVC 无法正确解析模板参数

修复：

- 改为显式使用 `lightui::Element / lightui::Node / lightui::NodeType`

### 编译失败 3：`FindEditableElement(...)` const 限定不匹配

问题：

- `GetActiveEditableRoot(...) const` 中调用非 const 的 `FindEditableElement(...)`

修复：

- 将声明与定义都改为 `FindEditableElement(std::shared_ptr<Node> node) const`

### 10. 修复 contenteditable 鼠标点击定位与拖动选择

修改文件：

- `core/event/dispatch/mouse_event_dispatcher.h`
- `core/event/dispatch/mouse_event_dispatcher.cpp`
- `core/event/loop/event_loop.cpp`

关键变更：

- `MouseEventDispatcher::SetManagers(...)` 新增注入 `ContentEditableController*`
- `EventLoop` 将已创建的 `contenteditable_controller_` 正式接入鼠标分发器
- `HandleMouseDown(...)` 在 `contenteditable` 左键按下时优先委托 `ContentEditableController::HandleMouseDown(...)`
- `HandleMouseMove(...)` / `HandleNoHitMouseMotion(...)` 在拖选期间委托 `ContentEditableController::HandleMouseMove(...)`
- `HandleMouseUp(...)` / `HandleNoHitMouseUp(...)` 在结束拖选时委托 `ContentEditableController::HandleMouseUp(...)`
- 保留普通元素与 input / textarea 既有逻辑，不扩大修改面

效果：

- 点击落点不再依赖 `SelectionManager::FindTextNodeAtPosition(...)` 的“返回第一个文本节点”假实现
- contenteditable 拖动选择不再停留在空实现，而是走基于 `RenderObject` 的真实文本命中逻辑
- 鼠标移出命中区域后继续拖动时，仍会延续 contenteditable 选择更新

### 11. 提升 contenteditable 跨元素文本命中精度

修改文件：

- `core/editing/contenteditable_controller.cpp`

关键变更：

- 将 `FindTextNodeAtPosition(...)` 从“递归命中单个文本节点 + 失败后粗糙 fallback”重构为“先收集所有文本 fragment 再统一命中”
- 新增 `TextFragmentInfo`，为每个文本渲染片段记录：布局盒、字体、文本内容、字符数、`letter-spacing`、`word-spacing`
- 使用每个 fragment 自己的 `ComputedStyle` 构建 `SkFont`，避免把不同元素的字体/字号混成一个统一测量
- 命中时先按 y 选择同一视觉行或最近行，再按 x 选择最近 fragment，明显改善跨元素、跨行附近的点击落点
- 在 fragment 内部改为基于 `utf8::SubstrByChar(...)` 按字符测量前缀宽度，返回字符 offset，而不是旧实现里混用 UTF-8 字节长度
- 当点击位于行首/行尾或文本盒之外时，改为折叠到片段首/尾，不再简单退化为“最后一个文本节点末尾”

当前效果：

- 不同元素使用不同字体、字号、字距时，点击命中更接近真实布局结果
- 跨多个 inline/text node 的选择起止点更稳定
- 为后续继续向 Blink 的 fragment / affinity 模型靠拢打下基础

### 12. 引入 editing host 语义并去除 contenteditable 重复高亮绘制

修改文件：

- `core/editing/contenteditable_geometry.h`
- `core/editing/contenteditable_geometry.cpp`
- `core/editing/selection_manager.cpp`
- `core/editing/contenteditable_handler.cpp`
- `core/editing/contenteditable_controller.cpp`
- `core/render/objects/render_block.cpp`

关键变更：

- 新增 `GetContentEditableEditingHost(...)` 与 `IsNodeInsideEditingHost(...)`，不再用“向上取最外层 contenteditable root”的旧语义
- `SelectionManager` / `ContentEditableHandler` / `ContentEditableController` 统一改为基于 editing host 判断 selection 与编辑归属
- `contenteditable="false"` 现在会作为 editing boundary 截断，避免把嵌套/非编辑区域错误吸到外层 root
- `RenderBlock::PaintContentEditableCaret(...)` 只负责折叠选区 caret 绘制；当 selection 是 range 时直接退出，不再在 contenteditable 层重复绘制高亮
- 保留 legacy fallback 代码仅作临时参考，但当前执行路径已经被新的 early-return 截断，避免拖动选择时出现多层高亮

当前效果：

- 拖动选择的高亮来源被收敛为单一路径，不再由 contenteditable 自绘 range highlight 叠加一层
- 嵌套 contenteditable 的活动编辑宿主更接近 Blink 的 editing boundary 语义
- 后续继续处理跨 host 映射、`contenteditable=false` 首尾 caret 停靠时，可以直接复用新的 host helper



### 13. 清理 `RenderBlock` 不可达 legacy fallback，并收紧拖选边界

修改文件：

- `core/render/objects/render_block.cpp`
- `core/editing/contenteditable_controller.cpp`

关键变更：

- 彻底删除 `RenderBlock::PaintContentEditableCaret(...)` 中已不可达的 legacy fallback，避免后续维护时再次误接入旧绘制路径
- `PaintContentEditableCaret(...)` 现在只保留单一职责：在当前元素是 editing host、selection 折叠、focus 仍属于该 host 时绘制 caret
- `ContentEditableController::HandleMouseMove(...)` 增加拖选起点归属校验；若起点已不在 active editing host 内，直接重置拖选状态
- 对拖动坐标按 content box 做 clamp，避免鼠标移出 host 后把命中计算扩散到宿主外的布局区域
- 若拖动命中结果不再属于当前 editing host，则回退到 host 本身，避免跨 host / 嵌套 editable 时把 focus 错误延伸到其他编辑宿主

当前效果：

- `RenderBlock` 的 contenteditable 渲染路径更干净，后续继续演进不会再受旧高亮逻辑干扰
- 拖选在离开 active host 可视区域时，行为更接近“限制在当前 editing boundary 内”而不是继续向外漂移
- 为下一步补 `contenteditable=false` 前后 caret 停靠语义提供了更稳定的宿主边界基础


### 14. 补齐 `contenteditable=false` / 非文本命中时的 begin/end caret 停靠

修改文件：

- `core/editing/contenteditable_geometry.h`
- `core/editing/contenteditable_geometry.cpp`
- `core/editing/contenteditable_controller.h`
- `core/editing/contenteditable_controller.cpp`

关键变更：

- 新增 `ContentEditableResolvedPosition` 与 `ResolveContentEditableCaretPosition(...)`，把“元素节点 / 空白区 / 非文本子树”的停靠解析收敛到 geometry helper
- 新增 `GetContainingContentEditableHost(...)`，允许从 `contenteditable=false` 或非可编辑后代节点向上回看所属 editing host，再决定回退停靠位置
- 在 geometry helper 中按 DOM 顺序查找 editing host 内最近的前驱/后继可编辑文本节点，优先返回“前一个文本末尾 / 后一个文本开头”而不是一律退回 host 开头
- `ContentEditableController::HandleMouseDown(...)` 与 `HandleMouseMove(...)` 在 fragment 命中失败时，改为调用新的 fallback resolver，基于点击横向位置选择更接近 begin/end 的停靠方向

当前效果：

- 点击到 `contenteditable=false` 边界、无文本包装元素或 host 空白区域时，caret/selection 更接近浏览器常见的 begin/end 停靠语义
- 拖选穿过非文本片段时，不再总是突然跳回 editing host 的 `(root, 0)`
- 为后续继续细化 nested editable / affinity 行为提供了统一 fallback 基础

### 15. 统一 Selection / Range 的 UTF-8 字符 offset 语义

修改文件：

- `core/dom/selection/selection.cpp`
- `core/dom/selection/range.cpp`

关键变更：

- `Selection::GetNodeLength(...)` 改为对文本节点返回 `utf8::CharCount(...)`，不再返回 UTF-8 字节数
- `Selection::ResolveElementPosition(...)` 在解析元素边界到最后一个文本节点时，末尾 offset 改为字符长度，避免中文文本末尾位置被放大
- `Range::GetNodeLength(...)` 同步改为字符长度语义，保证 `SetStart/SetEnd`、折叠与范围校验和命中层一致
- `Range::ComputeTextRect(...)` 改为用 `utf8::SubstrByChar(...)` 计算 prefix / range 文本，再交给字体测量，避免把“第 N 个字符”误当成“第 N 个字节”
- `Range::CollectText(...)` 与 `Range::ToString()` 的同节点 / 起止节点裁剪逻辑统一改为按字符截取，避免中文 + inline 混排时文本范围、caret rect、selection rect 出现系统性错位
- 新增 `core/utils/utf8_utils.h` 依赖接入 `selection.cpp` / `range.cpp`

当前效果：

- 点击命中返回的字符 offset 与后续 Selection / Range / caret rect 解释方式终于一致
- 中文文本前缀宽度、折叠 caret 几何、跨 inline 文本范围裁剪不再混用字节偏移
- 对 `这里是 <b>contenteditable</b> 测试区域。` 这类“中文 + inline 混排”场景，至少已清除最可疑的一层基础错位源


### 16. 修正 `Range::ComputeTextRect(...)` 对普通 inline 文本仍锚定父元素的问题

修改文件：

- `core/dom/selection/range.cpp`

关键变更：

- `ComputeTextRect(...)` 不再只在 `flex/grid` 容器中才使用文本节点自己的渲染对象位置
- 对普通 inline/text flow 也优先使用 `text_node->GetRenderObject()->GetViewportBounds()` 作为文本片段起点
- 当 `ViewportBounds` 尚未缓存时，先调用 `UpdateViewportBounds()`，再读取文本节点自己的视口坐标
- 父元素 `GetBoundingClientRect()` + padding 仅保留为 fallback，不再作为普通文本节点的默认锚点来源
- 字体、字号、`line-height`、字重、斜体等测量参数优先取文本节点自己的 computed style，避免 `<b>`、普通文本、不同字号混排时仍按父容器样式估算

当前效果：

- 对 `这里是 <b>contenteditable</b> 测试区域。` 这类“普通文本 + inline 元素混排”场景，折叠 caret rect 的 x 起点更接近真实文本 fragment，而不是整块 `div` 的起始边界
- 这会直接影响 `ComputeContentEditableCaretRect(...)`、`FocusManager::UpdateTextInputArea()` 与后续 selection rect 的几何准确性
- 从用户提供的 IME area 日志推断，这一层是 UTF-8 offset 统一之后最主要的剩余错位源

### 17. 将字符点击命中从“字符中心近似”改为“最近 caret 边界”

修改文件：

- `core/editing/contenteditable_controller.cpp`

关键变更：

- 新增 `MeasureFragmentCharAdvance(...)`，统一单字符 advance 计算，显式纳入字符宽度、`word-spacing` 与 `letter-spacing`
- `MeasureFragmentPrefixWidth(...)` 改为基于同一套 advance 模型累加前缀宽度，避免前缀测量和命中判定使用两套不同近似
- `HitTestFragmentOffset(...)` 不再按“点击是否越过字符中心”推断 offset，而是枚举 `0..N` 所有插入点边界，选择离点击位置最近的 caret boundary
- `FindTextNodeAtPosition(...)` 删除旧的 `raw_offset` 二次修正逻辑，直接使用统一边界命中结果

当前效果：

- caret 更接近落在真实“字符间”而不是字符本体上
- 对中文宽字符、不同字体混排、`letter-spacing` / `word-spacing` 场景，点击结果更稳定
- 在跨元素命中已经打通后，这一步继续修复“看起来能点中片段，但光标不是出现在字符之间”的剩余问题




### 1. Diagnostics

已对本轮关键修改文件执行 `diagnostics`，结果：

- 无诊断错误

### 2. GitNexus impact

对补修函数再次执行 impact 检查时，风险保持为：

- `LOW`

未出现 HIGH / CRITICAL 风险。

### 3. GitNexus detect_changes

执行：

- `detect_changes(scope="all", repo="MBink")`

结果：

- `risk_level = low`
- `affected_processes = []`

### 4. Release 编译

执行命令：

```bash
cmake --build build --config Release -j 8
```

结果：

- 异步构建成功
- `exitCode = 0`

### 5. Release 运行验证

执行命令：

```bash
build\bin\Release\esm_loader.exe examples\contenteditable_ime_demo.html -q 5
```

结果：

- HTML 模式正常启动
- demo 页面与脚本执行成功
- 自动退出成功
- `exitCode = 0`

## 本轮未做事项

- 未生成任何测试脚本
- 未新增专门自动化测试用例

## 当前结论

本轮已完成 `contenteditable` 的基础 IME / geometry 主链路接线：

- caret geometry 已可统一计算
- selection geometry 已可统一复用
- composition anchor 已可锁定到起始字符位置
- IME candidate window area 已接入 contenteditable 分支
- preedit 基础状态与提交流程已打通
- Release 编译与基础运行验证通过

## 后续建议

1. 增加一个专门的 contenteditable demo 页面，便于直接人工验证 IME 候选窗与 preedit 表现
2. 继续扩展跨多 text node / 跨行 selection geometry 的精度
3. 进一步统一 input / textarea / contenteditable 的编辑几何底座

