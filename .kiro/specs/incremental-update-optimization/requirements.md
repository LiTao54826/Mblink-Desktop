# 需求文档：增量更新系统优化

## 简介

本项目的 UI 框架采用双路径增量更新架构：
- **即时路径**：ObserverManager → WindowDOMObserver（立即触发样式重解析）
- **延迟路径**：DirtyNodeTracker → RenderTreeSynchronizer（批量处理结构变化）

当前，结构性 DOM 操作（appendChild/removeChild/insertBefore/replaceChild）和文本变化（Text::SetData）已正确使用双路径。但属性变化、样式变化、部分批量操作以及特定元素的值更新存在路径缺失或实现缺陷，导致：
- 同帧内多次样式/属性变化无法合并优化
- 部分操作未触发脏标记导致渲染不更新
- JS 绑定层的 classList 存在子串误匹配

本需求旨在修复已确认的 Bug、补齐延迟路径覆盖、统一双路径架构一致性。

## 术语表

- **DirtyNodeTracker**：脏节点追踪器，收集 DOM 变化并延迟到渲染前统一处理，支持变化合并优化
- **ObserverManager**：观察者管理器，即时通知 DOM 变化的观察者（如 WindowDOMObserver）
- **RenderTreeSynchronizer**：渲染树同步器，在渲染前将 DirtyNodeTracker 中的变化同步到渲染树
- **MarkDirty**：脏标记函数，标记节点需要重新布局/绘制，接受 DirtyType 参数（LAYOUT/PAINT/STYLE）
- **即时路径**：通过 ObserverManager 立即通知变化的处理路径
- **延迟路径**：通过 DirtyNodeTracker 收集变化、渲染前批量处理的路径
- **DOMTokenList**：W3C 标准的 classList 接口，用于操作元素的 class 列表
- **HTMLInputElement**：HTML input 元素类，支持文本输入、复选框、单选按钮等多种类型

## 需求

### 需求 1：HTMLInputElement::SetValue() 脏标记修复

**用户故事：** 作为开发者，我希望通过 JS 编程方式设置 input.value 时能正确触发重绘，以便用户能看到更新后的值。

#### 验收标准

1. WHEN HTMLInputElement::SetValue() 被调用且新值与旧值不同, THE HTMLInputElement SHALL 调用 MarkDirty(DirtyType::PAINT) 标记节点需要重绘
2. WHEN HTMLInputElement::SetValue() 被调用且新值与旧值不同, THE HTMLInputElement SHALL 通知所属 Window 需要重绘
3. WHEN HTMLInputElement::SetValue() 被调用且新值与旧值相同, THE HTMLInputElement SHALL 跳过脏标记和重绘通知以避免不必要的开销
4. WHEN HTMLInputElement::SetValue() 被调用且 trigger_events 为 false, THE HTMLInputElement SHALL 仍然执行脏标记和重绘通知（脏标记与事件触发独立）

### 需求 2：Node::RemoveAllChildren() DirtyNodeTracker 记录

**用户故事：** 作为框架使用者，我希望 SetTextContent() 和 SetInnerHTML() 清空子节点时，被移除的子节点能被 DirtyNodeTracker 正确追踪，以便渲染树同步器能正确清理对应的渲染对象。

#### 验收标准

1. WHEN RemoveAllChildren() 被调用, THE Node SHALL 将每个被移除的子节点逐一记录到 DirtyNodeTracker 的 RecordNodeRemoved 中
2. WHEN RemoveAllChildren() 被调用且节点不属于任何 Document, THE Node SHALL 跳过 DirtyNodeTracker 记录并正常完成子节点移除
3. WHEN SetTextContent() 调用 RemoveAllChildren() 后再 AppendChild() 新文本节点, THE DirtyNodeTracker SHALL 包含所有旧子节点的 Removed 记录和新文本节点的 Added 记录
4. WHEN RemoveAllChildren() 记录的 Removed 变化经过 DirtyNodeTracker::Optimize(), THE DirtyNodeTracker SHALL 正确合并与同一节点的 Added 记录（如先 Add 后 Remove 则取消）

### 需求 3：Element::SetStyle() DirtyNodeTracker 记录

**用户故事：** 作为开发者，我希望同一帧内对同一元素的多次样式变化能被合并优化，以避免每次 SetStyle 都触发一次完整的 StyleResolver::ResolveStyle()。

#### 验收标准

1. WHEN Element::SetStyle() 被调用, THE Element SHALL 将样式变化记录到 DirtyNodeTracker 的 RecordStyleChanged 中（包含元素引用、属性名、旧值、新值）
2. WHEN 同一元素在同一帧内多次调用 SetStyle() 修改同一属性, THE DirtyNodeTracker::Optimize() SHALL 合并为一条记录，仅保留最初的旧值和最终的新值
3. WHEN 同一元素在同一帧内多次调用 SetStyle() 修改不同属性, THE DirtyNodeTracker SHALL 为每个属性保留独立的变化记录
4. WHEN Element::SetStyle() 被调用, THE Element SHALL 同时保留对 ObserverManager 的即时通知（保持向后兼容）

### 需求 4：Element::SetAttribute() 和 RemoveAttribute() DirtyNodeTracker 记录

**用户故事：** 作为开发者，我希望属性变化（class、id 等）也能通过延迟路径被追踪，以便与结构变化保持一致的处理方式，并支持同帧合并优化。

#### 验收标准

1. WHEN Element::SetAttribute() 被调用且属性值发生变化, THE Element SHALL 将属性变化记录到 DirtyNodeTracker（通过 RecordStyleChanged 记录，复用样式变化通道）
2. WHEN Element::RemoveAttribute() 被调用且属性存在, THE Element SHALL 将属性移除记录到 DirtyNodeTracker（新值为空字符串）
3. WHEN 同一元素在同一帧内多次修改同一属性, THE DirtyNodeTracker::Optimize() SHALL 合并为一条记录
4. WHEN Element::SetAttribute() 或 RemoveAttribute() 被调用, THE Element SHALL 同时保留对 ObserverManager 的即时通知（保持向后兼容）

### 需求 5：JS 绑定层 classList 子串误匹配修复

**用户故事：** 作为 JS 开发者，我希望 classList.contains/add/remove/toggle 能精确匹配完整的 class 名称，以避免 "btn" 错误匹配 "btn-primary" 等子串误匹配问题。

#### 验收标准

1. WHEN classList.contains() 被调用, THE JS 绑定 SHALL 使用按空格分词后的精确匹配（而非 string::find 子串匹配）来判断 class 是否存在
2. WHEN classList.add() 被调用, THE JS 绑定 SHALL 使用精确匹配检查 class 是否已存在，避免子串误判导致重复添加失败
3. WHEN classList.remove() 被调用, THE JS 绑定 SHALL 使用精确匹配定位要移除的 class，避免子串误匹配导致移除错误的 class
4. WHEN classList.toggle() 被调用, THE JS 绑定 SHALL 使用精确匹配判断 class 存在性，确保切换行为正确
5. WHEN classList 操作处理包含 "btn" 和 "btn-primary" 的 class 列表时, THE JS 绑定 SHALL 将 "btn" 和 "btn-primary" 视为两个独立的 class（不产生子串误匹配）

### 需求 6：RenderTreeSynchronizer 样式变化处理集成

**用户故事：** 作为框架维护者，我希望 RenderTreeSynchronizer::ProcessStyleChanges() 能正确处理来自 DirtyNodeTracker 的样式变化记录，以完成延迟路径的端到端闭环。

#### 验收标准

1. WHEN RenderTreeSynchronizer::Synchronize() 处理包含样式变化的 DirtyNodeTracker, THE RenderTreeSynchronizer SHALL 调用 ProcessStyleChanges() 对每个样式变化触发对应元素的样式重解析
2. WHEN ProcessStyleChanges() 处理样式变化, THE RenderTreeSynchronizer SHALL 根据变化的属性类型设置正确的脏标记（布局属性设置 LAYOUT|PAINT，纯外观属性仅设置 PAINT）
3. WHEN DirtyNodeTracker 同时包含结构变化和样式变化, THE RenderTreeSynchronizer SHALL 先处理结构变化再处理样式变化，确保新添加的节点能正确接收样式更新
