# Inline Whitespace 路径与语义完整修复方案

## 1. 问题定性

当前问题**不是单纯布局坐标错误**，也不是单纯空格宽度测量错误。

最准确的定性是：

- **根因：路径 / 分层问题**
  - 在 anonymous block 路径中，whitespace-only TEXT 节点在进入 IFC 之前被提前过滤。
- **表现：whitespace 语义错误**
  - 本应参与 `white-space` collapsing 的空白节点未进入 inline formatting context。
- **结果：布局间距错误**
  - 最终表现为 inline 元素之间缺失应有间隔。

## 2. 已确认的日志结论

通过 `DEBUG_ANON_INLINE_WS=1` 的 C++ 日志，已经确认：

1. mixed content 场景下 child 序列实际包含：
   - `TEXT(' ') -> INLINE(span1) -> TEXT(' ') -> INLINE(span2) -> TEXT(' ') -> BLOCK`
2. 但在 `IsInlineLevelElement()` 中，whitespace-only TEXT 被判定为 `result=0`
3. anonymous block 最终只收到了两个 `INLINE` child
4. 因此 anonymous block 的 box 序列中不存在中间空白 TEXT box

这说明：

> 当前错误发生在 **进入 IFC 之前**，而不是发生在 IFC 内部的最终 x 计算阶段。

## 3. 当前设计缺陷

现有实现把“空白是否应该显示”的决策放在了 tree building / anonymous block grouping 阶段：

- `IsInlineLevelElement()` 直接过滤 whitespace-only TEXT
- mixed content 构建 anonymous block 时因此丢失文本语义

这是不合理的职责分层，因为空白是否最终显示依赖：

- `white-space` 模式
- 相邻 inline 内容
- 行首 / 行尾位置
- 断行后的 trimming
- 跨 text box 的 collapsing

这些都应属于 **IFC 文本处理与 line breaking**，而不是 tree building。

## 4. 修复目标

本次修复的目标不是仅解决当前 `span + 空白 + span` case，而是建立一套完整、可扩展的 inline whitespace 处理链路，使后续类似问题不再需要逐个补丁修。

## 5. 总体修复原则

### 5.1 Tree building 层
负责：
- 区分 block-level 与 inline-level content
- 构建 anonymous block
- 保留 inline text 语义

不负责：
- 决定 whitespace-only TEXT 是否最终可见

### 5.2 IFC text preprocessing 层
负责：
- 基于 `white-space` 模式对文本进行规范化
- 主 IFC 路径与 anonymous block IFC 路径使用统一逻辑

### 5.3 Line breaking / line layout 层
负责：
- collapsible whitespace 折叠
- leading / trailing whitespace trimming
- 跨 text box 的空白折叠
- 断行后的新行起始空白处理

## 6. 实施阶段

### Phase 1：修正 anonymous block 路径输入

**目标**：保证 whitespace TEXT 能进入 inline formatting 链路。

**修改点**：
- `core/layout/native_layout_engine.cpp`

**内容**：
- 修改 `IsInlineLevelElement()`：TEXT 一律视为 inline-level content
- mixed content 构建 anonymous block 时，保留 TEXT child，不再在 tree building 层吞掉 whitespace-only TEXT

**预期效果**：
- anonymous block child 序列保留完整文本语义
- whitespace 节点进入后续 IFC 文本处理

### Phase 2：统一 text normalization

**目标**：主 IFC 与 anonymous block IFC 共享一致的 white-space 预处理语义。

**修改点**：
- `core/layout/ifc/ifc_layout.cpp`
- `core/layout/native_layout_engine.cpp`
- 如有必要可抽取共享 helper

**内容**：
- 统一 `white-space: normal / nowrap / pre / pre-wrap / pre-line` 的文本规范化处理
- 避免主 IFC 与 anonymous block IFC 逻辑分叉
- 将“是否生成文本盒”的初步判定建立在统一处理结果上

### Phase 3：增强 LineBreaker 的 collapsible whitespace 处理

**目标**：把 whitespace 的最终可见性决定放回 line layout 阶段。

**修改点**：
- `core/layout/ifc/line_breaker.cpp`
- 必要时 `line_box.cpp/.h`

**内容**：
1. leading collapsible whitespace trimming
2. trailing collapsible whitespace trimming
3. 跨 text box 边界的空白折叠
4. 断行后新行起始空白的继续裁剪
5. 确保 `normal / nowrap / pre-line` 与 `pre / pre-wrap` 行为分离

### Phase 4：回归验证

**目标**：验证修复不是 case patch，而是系统生效。

**验证方式**：
- 编译项目
- 运行对比用例
- 检查日志与对比报告

**重点场景**：
1. `span + span`
2. `text + span + text`
3. `inline-block + inline-block`
4. 行首空白
5. 行尾空白
6. block / inline 交界空白
7. `white-space: nowrap`
8. `pre / pre-wrap / pre-line`

## 7. 风险控制

### 低风险修正
- inline 元素之间应有源码空白的场景恢复正常间隔
- anonymous block 路径与主 IFC 路径收敛

### 中风险点
- run 开头 / 结尾空白如果 trimming 不完整，可能影响 mixed content 排版

### 高风险点
- `pre / pre-wrap / pre-line` 与 `normal / nowrap` 的模式边界处理

因此必须按分阶段推进，不能只改 tree building，不改 line breaker。

## 8. 结论

本问题的第一根因是：

> **inline whitespace 在 anonymous block 路径上被过早过滤，导致后续 whitespace 语义与布局都无法正确执行。**

因此完整方案必须同时修正：

1. anonymous block 输入路径
2. IFC 文本规范化
3. line breaker 的 collapsible whitespace 规则

只有这样，才能避免“修完一个 case 又出现新的 whitespace 补丁”的循环。

