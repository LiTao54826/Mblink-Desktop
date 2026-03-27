# Next Plan：contenteditable IME / caret geometry 完整接线

## 背景

当前 `input / textarea` 的文本编辑统一化已经推进到较完整阶段，已完成：

- 共享文本测量 / hit test 能力
- input / textarea 的 composition 事件接线
- textarea 多行编辑 state/controller 抽象
- input / textarea 的 IME preedit 自绘
- Windows IME 候选窗 area / DPI / baseline / composition anchor 修复
- 候选窗 X 锚点锁定到 composition 起始字符
- contenteditable 的基础 caret mapping 验证点

其中 `contenteditable` 目前仍然只具备**基础点击定位验证能力**，尚未完成真正可用于 IME 的：

- DOM text run 到可见文本的稳定映射
- caret / selection / composition 的真实几何计算
- `SDL_SetTextInputArea(...)` 的完整接线
- composition anchor 几何与系统候选窗对齐

因此下一阶段重点转向：**补齐 contenteditable 的 IME / caret geometry 完整链路**。

---

## 总目标

让 `contenteditable` 具备与 `input / textarea` 同等级别的：

1. caret 几何定位
2. selection 几何映射
3. composition 起止区间映射
4. IME candidate window 锚点同步
5. 后续可纳入统一编辑底座的共享几何模型

---

## 阶段拆分

### Phase A：盘点 contenteditable 现状与缺口

目标：

- 梳理 `contenteditable` 当前点击、选区、文本节点定位的入口
- 明确现有能力与 IME 所需能力之间的缺口
- 识别低风险切入点

重点关注：

- `SelectionManager`
- `MouseEventDispatcher::UpdateSelectionFromClick(...)`
- contenteditable 相关 `Element / Node / Text` 结构
- RenderTree 中可提供文本位置映射的对象
- 当前焦点管理与 `FocusManager::UpdateTextInputArea()` 对 contenteditable 的处理现状

输出：

- contenteditable 当前“逻辑位置 -> 几何位置”链路清单
- 下一步可复用的数据源清单

---

### Phase B：建立 DOM text run / visible text 映射层

目标：

- 为 contenteditable 建立可复用的文本运行单元（text run）视图
- 支持 DOM 文本节点与“可见文本偏移”之间的双向映射
- 为 caret / selection / composition 几何计算提供稳定输入

建议能力：

- 收集 contenteditable 子树中的文本 run
- 记录每个 run：
  - 对应 DOM 节点
  - 文本内容
  - 全局字符区间
  - 局部字符区间
  - RenderObject / style / font 信息
- 建立：
  - DOM position -> visible text offset
  - visible text offset -> DOM position

输出：

- `contenteditable` 可见文本模型
- text run 映射 helper

---

### Phase C：实现 caret geometry / selection geometry

目标：

- 从“文本偏移”提升到“屏幕几何”
- 支持 contenteditable 中真实 caret rect 计算
- 支持 selection 区间的基础几何范围计算

建议能力：

- 给定 visible text offset，求：
  - caret x/y
  - baseline
  - line box
  - 对应 viewport/client rect
- 对 selection：
  - 至少先支持单 run / 同行区间几何
  - 后续再扩到跨 run / 跨行

重点：

- 尽量复用现有字体测量链路
- 尽量贴近真实绘制几何，而不是纯估算
- 与 DPI / display scale / scroll 保持一致

输出：

- contenteditable caret geometry helper
- contenteditable selection geometry helper（基础版）

---

### Phase D：接入 IME composition anchor 与 SDL text input area

目标：

- 让 contenteditable 在 active composition 时，也能正确同步系统候选窗锚点
- 候选窗位置基于 composition 起始字符，而不是 composition 末尾

建议策略：

- `FocusManager::UpdateTextInputArea()` 扩展 contenteditable 路径
- 无 composition 时：
  - 锚到当前 caret geometry
- 有 composition 时：
  - 锚到 composition start geometry
- 向 `SDL_SetTextInputArea(...)` 同步：
  - area rect
  - cursor x offset

重点：

- 保持与 input / textarea 当前修复后的策略一致
- 特别注意：
  - composition start anchor
  - DPI scale
  - scroll offset
  - baseline / line-height 对齐

输出：

- contenteditable 的 IME candidate anchor 完整接线

---

### Phase E：接入 preedit 可视化与 repaint 联动

目标：

- 让 contenteditable 在 IME preedit 期间具备基础可视反馈
- 至少达到当前 input / textarea 同级别的“可见性”

建议能力：

- composition text 可视插入
- composition underline / dashed underline
- selection / caret / composition 三者对齐
- composition 更新时触发 repaint
- 焦点 / 滚动变化时同步刷新 IME area

输出：

- contenteditable IME preedit 基础可视化

---

### Phase F：沉淀统一编辑几何抽象

目标：

- 将 `input / textarea / contenteditable` 三类可编辑目标中共性的几何逻辑沉淀为统一底座
- 为后续统一编辑层第三轮推进做准备

候选抽象：

- visible text model
- composition anchor model
- caret geometry provider
- selection geometry provider
- text input area sync helper

输出：

- 下一轮统一编辑底座实施输入

---

## 风险点

### 1. contenteditable 的 DOM 结构复杂度更高
- 与 input/textarea 的单值模型不同
- 需要处理多 text node、多层嵌套、inline/inline-block 混合情况

### 2. 文本偏移与真实绘制对象可能不完全一一对应
- 需要谨慎设计 text run 抽象
- 避免逻辑位置和实际几何脱节

### 3. selection / composition 的上游状态模型可能仍不足
- 当前 input / textarea 有 edit_state
- contenteditable 可能还需要额外状态承载 composition / anchor / focus position

### 4. Windows IME 平台行为仍可能存在特殊边界
- 即使几何正确，系统候选窗仍可能有平台自身偏移策略
- 需要保留调试日志能力

---

## 实施顺序建议

推荐严格按以下顺序推进：

1. 盘点现状与缺口
2. 建 DOM text run / visible text 映射
3. 做 caret geometry
4. 接 IME anchor / SDL text input area
5. 做 preedit 可视化
6. 最后再抽统一编辑几何底座

不要一开始就急着抽象统一层，避免在 contenteditable 真实几何尚未跑通前过早封装。

---

## 修改前规则

继续遵守当前仓库规则：

- 修改函数 / 类 / 方法前，先做 GitNexus impact
- 修改后做 `gitnexus_detect_changes()`
- HIGH / CRITICAL 风险必须显式提示
- 技术文档查询优先 Context7
- 不生成测试脚本
- 需要生成总结性 Markdown 文档
- 需要编译
- 需要运行
- 编译使用 Release
- cmake 编译优先异步执行并查询状态

---

## 本阶段建议涉及文件（初步）

可能重点查看：

- `core/editing/selection_manager.cpp`
- `core/editing/selection_manager.h`
- `core/event/dispatch/mouse_event_dispatcher.cpp`
- `core/event/input/focus_manager.cpp`
- `core/render/input/text_edit_metrics.h`
- `core/render/input/text_edit_metrics.cpp`
- `core/render/objects/render_text.cpp`
- `core/render/objects/render_inline_block.cpp`
- `core/dom/element.h`
- `core/dom/element.cpp`

视实现情况可能新增：

- `core/editing/contenteditable_*`
- `core/render/input/contenteditable_*`

---

## 完成标志

当以下条件都满足时，可认为本阶段完成：

- contenteditable 可稳定计算 caret geometry
- contenteditable 可在 IME composition 时提供正确 candidate anchor
- composition anchor 锁定在第一个字符位置
- preedit 具备基础可视反馈
- Release 编译通过
- Release 运行通过
- 总结性 Markdown 文档已生成
- 未生成测试脚本

