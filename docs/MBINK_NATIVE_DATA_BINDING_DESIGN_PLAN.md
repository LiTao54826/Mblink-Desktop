# MBink 原生数据绑定 Stage 1 MVP 技术规格

## 1. 结论
Stage 1 MVP 只做一条最小可验证链路：

`StateGraph mutation -> path 命中 -> binding 入队 -> batch flush -> 定点更新 DOM`

本阶段必须保留：
- `StateGraph`
- `ElementScope`（只做就近继承）
- `TextBinding`
- `AttrBinding`
- `VisibleBinding`
- `ModelBinding`（仅 `input.value` / `checked`）
- `batch` 更新

本阶段明确不做：
- `ForBinding`
- `IfBinding`
- `ClassBinding` / `StyleBinding` 的复杂表达式
- 通用模板编译器
- 任意 JS 表达式求值
- Preact 替换

## 2. 当前收敛点
原设计文档的问题不是方向错误，而是过于偏“预案”：
- 有总体目标，但缺少可直接实现的数据结构
- 提到 binding / scope / scheduler，但缺少字段级规格
- API 只列了名字，没定义返回值、side effects、batch 行为
- 生命周期、清理责任、错误策略不明确

本版只保留 Stage 1 需要的技术规格，删除不影响 prototype 的宏观愿景描述。

## 3. Stage 1 MVP 边界

### 3.1 In Scope
1. `StateGraph` 作为唯一真源（single source of truth）
2. binding 仅依赖 **单一路径 path**，不支持表达式求值
3. 子元素通过 `ElementScope` 做就近继承解析
4. 数据变更时只更新命中的 binding，不触发整棵树 rerender
5. 所有变更统一进入 `set/delete` 与 `flush` 调度链路

### 3.2 Out of Scope
1. 列表渲染与 diff
2. 条件创建/销毁节点
3. 多路径依赖收集
4. 通用表达式解析器
5. 与现有 Preact 树做互斥或替换方案
6. 调试面板、性能可视化、开发时 DSL

## 4. 基础约定

### 4.1 Path 约定
- 外部 API 使用 `string path`
- canonical 格式为 `a.b.c`
- path segment 以 `.` 分隔
- Stage 1 不定义数组语义；`items.0.name` 仅按普通 segment 处理
- 空 path 非法

### 4.2 Scope 解析约定
- binding 在挂载时拿到一个 `sourcePath`
- 若 `sourcePath` 首段命中当前元素最近的 `ElementScope.aliases`，则替换为该 alias 对应的 graph path
- 若当前 scope 未命中，则向父元素最近 scope 继续查找
- 只做“首段 alias 替换 + 就近继承”，不做多段表达式改写

示例：
- 当前元素最近 scope: `user -> page.form.user`
- binding path: `user.name`
- 解析结果: `page.form.user.name`

## 5. Binding 类型定义

### 5.1 TextBinding
- 目标：文本节点或元素文本内容
- 输入：单一路径
- 更新规则：读取值后转字符串写入
- `null` / `undefined` -> 空字符串

### 5.2 AttrBinding
- 目标：元素 attribute
- 输入：单一路径 + `attrName`
- 更新规则：
  - `null` / `undefined` -> remove attribute
  - 其他值 -> `stringify(value)` 后写入

### 5.3 VisibleBinding
- 目标：元素可见状态
- 输入：单一路径
- 更新规则：
  - `false` / `null` / `undefined` -> hidden
  - 其他值 -> visible
- Stage 1 只切换显示状态，不创建/销毁节点

### 5.4 ModelBinding
- 目标：`input.value` 或 `input.checked`
- 输入：单一路径 + `modelKind`
- `modelKind = value`：
  - graph -> DOM: 标量值写入 `input.value`
  - DOM -> graph: 用户输入触发回写
- `modelKind = checked`：
  - graph -> DOM: 布尔语义写入 `input.checked`
  - DOM -> graph: 用户交互回写布尔值

## 6. 核心数据结构

### 6.1 StateGraphNode
职责：表示一个 canonical path 对应的状态节点，并承载订阅索引。

| 字段 | 类型 | 说明 |
|---|---|---|
| `path` | `string` | 该节点的 canonical path |
| `key` | `string` | 相对父节点的最后一段 key |
| `parentPath` | `string?` | 父节点 path，根节点为空 |
| `exists` | `bool` | 节点当前是否存在 |
| `value` | `Value` | 当前值 |
| `valueKind` | `enum` | `Null/Bool/Number/String/Object/Array/Undefined` |
| `revision` | `uint64` | 节点最后一次有效变更版本号 |
| `children` | `Map<string, string>` | `segment -> childPath` |
| `exactSubscriptions` | `Set<subscriptionId>` | 订阅该节点 exact path 的记录 |
| `subtreeSubscriptions` | `Set<subscriptionId>` | 订阅该节点 subtree 的记录 |

### 6.2 BindingRecord
职责：表示一个已挂载的 binding 实例及其运行时状态。

| 字段 | 类型 | 说明 |
|---|---|---|
| `bindingId` | `uint64` | 唯一 ID |
| `kind` | `enum` | `Text/Attr/Visible/ModelValue/ModelChecked` |
| `targetNodeId` | `NodeHandle` | DOM/原生节点句柄；允许 weak 引用 |
| `ownerElementId` | `ElementId` | 归属元素 |
| `scopeChainId` | `uint64` | 绑定挂载时绑定的 scope chain |
| `sourcePath` | `string` | 声明时路径 |
| `resolvedPath` | `string` | 经 scope 解析后的 graph path |
| `attrName` | `string?` | 仅 `AttrBinding` 使用 |
| `mounted` | `bool` | 是否仍处于挂载状态 |
| `dirty` | `bool` | 是否已进入 dirty 队列 |
| `orderIndex` | `uint64` | 创建顺序，用于稳定 flush 顺序 |
| `lastAppliedRevision` | `uint64` | 上次成功应用时的 graph revision |
| `lastRenderedValue` | `Value` | 上次写入目标节点的值 |
| `subscriptionIds` | `Vector<subscriptionId>` | 当前 binding 建立的订阅 |
| `writeBackGuard` | `bool` | `ModelBinding` 防止程序性写回形成回环 |

### 6.3 ScopeRef
职责：把元素局部命名空间映射到 graph path。

| 字段 | 类型 | 说明 |
|---|---|---|
| `scopeId` | `uint64` | 唯一 ID |
| `ownerElementId` | `ElementId` | 所属元素 |
| `parentScopeId` | `uint64?` | 最近父 scope |
| `aliases` | `Map<string, ScopeSlot>` | 本地 alias 表 |
| `readOnly` | `bool` | 整个 scope 是否只读 |

`ScopeSlot` 字段：
- `graphPath: string`
- `readOnly: bool`

### 6.4 ScopeChain
职责：缓存当前元素可见的 scope 查找链，按“近 -> 远”顺序解析 path。

| 字段 | 类型 | 说明 |
|---|---|---|
| `scopeChainId` | `uint64` | 唯一 ID |
| `scopeIds` | `Vector<uint64>` | 从最近 scope 到根 scope |
| `ownerElementId` | `ElementId` | 该链对应的元素 |

### 6.5 SubscriptionRecord
职责：描述一个 path 订阅与 binding 的关联。

| 字段 | 类型 | 说明 |
|---|---|---|
| `subscriptionId` | `uint64` | 唯一 ID |
| `bindingId` | `uint64` | 归属 binding |
| `path` | `string` | 订阅 path |
| `mode` | `enum` | `Exact/Subtree` |
| `active` | `bool` | 是否有效 |
| `lastDeliveredRevision` | `uint64` | 上次触发时的 revision |

### 6.6 DirtyBindingQueue
职责：维护待刷新的 binding 集合，并保证同一 binding 在同一轮 flush 中只出现一次。

| 字段 | 类型 | 说明 |
|---|---|---|
| `pendingBindingIds` | `OrderedSet<uint64>` | 待 flush binding，按 `orderIndex` 排序 |
| `scheduled` | `bool` | 是否已请求过一次 flush |
| `batchDepth` | `uint32` | 当前 batch 嵌套层级 |
| `pendingRevisionMax` | `uint64` | 当前队列内最大 revision |

### 6.7 FlushContext
职责：描述当前 flush 过程，处理 re-entrant mutation。

| 字段 | 类型 | 说明 |
|---|---|---|
| `isFlushing` | `bool` | 当前是否正在 flush |
| `flushEpoch` | `uint64` | flush 轮次 |
| `currentBindingId` | `uint64?` | 当前正在应用的 binding |
| `deferredBindingIds` | `OrderedSet<uint64>` | flush 期间新增的 dirty binding |
| `reentrantMutationCount` | `uint32` | flush 期间再次触发 `set/delete` 次数 |

## 7. 核心 API 规格

### 7.1 `get(path) -> Value`
- 输入：canonical path
- 输出：节点当前值；不存在时返回 `undefined`
- side effects：无
- 说明：不建立隐式订阅；Stage 1 的依赖关系只通过显式 `subscribe()` 建立

### 7.2 `set(path, value) -> bool`
- 输入：canonical path，任意 `Value`
- 输出：`true` 表示发生有效变更，`false` 表示 no-op
- side effects：
  - 必要时创建缺失节点
  - 更新节点值与 `revision`
  - 命中订阅并将 binding 入队
- 无变化处理：
  - 标量按值比较
  - `Object/Array` 按引用/实例比较
  - 相同则不递增 revision、不入队
- 重复 `set`：
  - 在同一 batch 内允许多次设置同一路径
  - 仅最终值参与本轮 flush
  - 命中过的 binding 仍只保留一份 dirty 记录

### 7.3 `delete(path) -> bool`
- 输入：canonical path
- 输出：`true` 表示删除成功，`false` 表示 path 原本不存在
- side effects：
  - 将目标节点标记为不存在
  - 相关 exact/subtree 订阅按“值已移除”触发
  - 命中 binding 入队
- 说明：对子树删除时，后代 path 视为不存在

### 7.4 `subscribe(path, mode = Exact, bindingId) -> subscriptionId`
- 输入：path、订阅模式、bindingId
- 输出：订阅 ID
- side effects：
  - 创建 `SubscriptionRecord`
  - 将其挂到 `StateGraphNode.exactSubscriptions` 或 `subtreeSubscriptions`
- 说明：Stage 1 只允许 binding 持有订阅，不开放任意匿名观察者语义

### 7.5 `unsubscribe(subscriptionId) -> bool`
- 输入：订阅 ID
- 输出：`true/false`
- side effects：
  - 将订阅从 graph node 索引移除
  - 标记 `SubscriptionRecord.active = false`

### 7.6 `beginBatch() -> uint32`
- 输出：进入后的 `batchDepth`
- side effects：`DirtyBindingQueue.batchDepth += 1`
- 说明：支持嵌套 batch

### 7.7 `endBatch() -> uint32`
- 输出：退出后的 `batchDepth`
- side effects：
  - `batchDepth -= 1`
  - 当最外层 batch 结束且队列非空时，立即触发一次 `flush()`
- 说明：不会多次 flush；只在最外层归零时触发

### 7.8 `flush() -> uint32`
- 输出：本轮实际刷新的 binding 数量
- side effects：
  - 按顺序应用 dirty binding
  - 清空当前 dirty 队列
  - 若 flush 期间发生新的 mutation，则追加一轮 flush
- 说明：
  - `batchDepth > 0` 时，除显式强制调用外，不自动执行
  - `flush()` 需要具备防重入保护

## 8. 绑定生命周期

### 8.1 创建
1. 宿主/解析层创建 `BindingRecord`
2. 记录 `kind`、`sourcePath`、目标节点句柄、owner element
3. 初始状态：`mounted = false`，`dirty = false`

### 8.2 挂载
1. 根据 `ownerElementId` 生成或获取 `ScopeChain`
2. 用 `ScopeChain` 将 `sourcePath` 解析为 `resolvedPath`
3. 创建对应 `SubscriptionRecord`
4. `BindingRecord.subscriptionIds` 保存订阅 ID
5. 标记 `mounted = true`
6. 立即入队一次，确保首次渲染通过同一条 flush 链路完成

### 8.3 订阅建立
- `TextBinding` / `AttrBinding` / `VisibleBinding` / `ModelBinding` 默认对 `resolvedPath` 建立 `Exact` 订阅
- 若后续某类 binding 需要感知子树变更，才允许使用 `Subtree`；Stage 1 默认不新增此类 binding

### 8.4 数据变更
1. `set/delete` 更新 `StateGraph`
2. graph 根据变更 path 找到命中的 `SubscriptionRecord`
3. 命中的 `bindingId` 进入 `DirtyBindingQueue`

### 8.5 入队
1. 若 binding 已 `dirty = true`，则跳过重复入队
2. 若 binding 未挂载，跳过
3. 将 `bindingId` 放入 `pendingBindingIds`
4. 标记 `dirty = true`

### 8.6 Flush
1. 取出当前 `pendingBindingIds`
2. 按 `orderIndex` 升序执行
3. 对每个 binding 重新读取 `resolvedPath` 当前值
4. 若目标节点已卸载，直接进入清理分支
5. 执行最小 DOM 写入
6. 更新 `lastRenderedValue` / `lastAppliedRevision`
7. 清除 `dirty`

### 8.7 卸载
1. 元素销毁或 binding 所属节点被移除时，由 **宿主 DOM/binding owner** 主动触发卸载
2. 将 `mounted = false`
3. 遍历 `subscriptionIds` 执行 `unsubscribe`
4. 从 dirty 队列移除该 binding（若存在）

### 8.8 释放订阅
- 责任方：binding owner，不是 `StateGraph`
- `StateGraph` 不做 GC 式隐式清理
- 任何未卸载但目标节点已失效的 binding，在 flush 时必须进入安全清理路径

## 9. 调度与更新规则

### 9.1 Path 级命中规则
- `Exact`：仅当 `mutationPath == subscription.path` 时命中
- `Subtree`：当以下任一条件成立时命中：
  - `mutationPath == subscription.path`
  - `mutationPath` 以 `subscription.path + "."` 为前缀

### 9.2 Subtree 订阅处理
- 命中算法从 `mutationPath` 向根路径逐段回溯，收集祖先节点上的 `subtreeSubscriptions`
- 例如变更 `a.b.c` 时，需要检查：`a.b.c`、`a.b`、`a`、根
- Stage 1 可只为内部索引保留该机制，即使默认 binding 不使用 `Subtree`

### 9.3 同一 binding 多次命中如何去重
- `DirtyBindingQueue.pendingBindingIds` 必须是 `OrderedSet`
- 同一轮内无论命中多少次，同一 `bindingId` 只保留一份
- 以最后一次 mutation 后的 graph 值作为 flush 输入

### 9.4 Batch 内多次 `set` 如何只 flush 一次
- `beginBatch` 到最外层 `endBatch` 之间不自动 flush
- 所有命中的 binding 仅入队一次
- `endBatch` 归零时统一 flush 一次

### 9.5 Flush 顺序
- 以 `BindingRecord.orderIndex` 升序为准，保证稳定、可预测
- 不按 mutation 发生顺序执行单独刷新
- 同一元素上的多个 binding 也遵守全局稳定顺序

### 9.6 Flush 中再次 `set` 如何处理
- 若 `flush()` 期间调用 `set/delete`：
  - 不立即递归执行 flush
  - 新命中的 binding 进入 `FlushContext.deferredBindingIds`
  - 当前轮结束后，如果 deferred 非空，追加下一轮 flush
- 若连续 re-entrant mutation 超过实现上限，必须报错并中止后续自动 flush

## 10. 错误与边界策略

### 10.1 Path 不存在
- `get(path)`：返回 `undefined`
- `TextBinding`：渲染为空字符串
- `AttrBinding`：移除 attribute
- `VisibleBinding`：按 hidden 处理
- `ModelBinding.value`：写入空字符串
- `ModelBinding.checked`：写入 `false`

### 10.2 类型不匹配
- `TextBinding`：标量转字符串；`Object/Array` 视为不支持，记录错误并渲染空字符串
- `AttrBinding`：标量转字符串；`Object/Array` 记录错误并移除 attribute
- `VisibleBinding`：使用 truthy/falsy 规则
- `ModelBinding.value`：仅接受标量；复杂类型记录错误并写空字符串
- `ModelBinding.checked`：DOM 回写永远写布尔值；graph 读取时按布尔语义处理

### 10.3 读写只读 scope
- 读：允许
- 写：若 path 首段经 scope 解析后命中只读 alias 或只读 scope，`set/delete` 必须失败并记录错误
- Stage 1 不做 silent fallback

### 10.4 绑定目标节点已卸载
- flush 前检查 `targetNodeId` 是否有效
- 无效则：
  1. 不做 DOM 写入
  2. 自动走卸载清理流程
  3. 返回安全失败，不崩溃

### 10.5 循环写入 / re-entrant mutation
- `ModelBinding` 在 graph -> DOM 写入时设置 `writeBackGuard = true`
- guard 生效期间，由程序性 DOM 赋值产生的回调不得再次 `set`
- flush 中发生再次写图时，通过 `deferredBindingIds` 延后到下一轮
- 必须设置最大 re-entrant 轮次，超限视为循环写入错误

### 10.6 空值 / `null` / `undefined`
- `null` 与 `undefined` 在 Stage 1 不区分 UI 行为：都按“无值”处理
- 但 `StateGraph` 内部仍保留原始值类型，便于调试与后续扩展

## 11. 最小 prototype 验收标准

### 场景 1：文本绑定
**初始状态**
- graph: `profile.name = "Alice"`
- 一个 `TextBinding` 绑定到 `profile.name`

**mutation**
- 调用 `set("profile.name", "Bob")`
- 调用一次 `flush()` 或由 batch 结束触发自动 flush

**预期 DOM 变化**
- 仅目标文本节点从 `Alice` 变为 `Bob`

**不应该发生什么**
- 不应触发整棵页面 rerender
- 不应重建父元素或兄弟节点

### 场景 2：VisibleBinding
**初始状态**
- graph: `panel.visible = true`
- 一个 `VisibleBinding` 绑定到 `panel.visible`

**mutation**
- `set("panel.visible", false)`
- `flush()`

**预期 DOM 变化**
- 目标元素变为 hidden
- 再次 `set("panel.visible", true)` 并 `flush()` 后恢复 visible

**不应该发生什么**
- 不应销毁并重建该元素
- 不应影响该元素下未绑定节点的结构

### 场景 3：input 双向绑定
**初始状态**
- graph: `form.username = "tom"`
- 一个 `ModelBinding(value)` 绑定到输入框

**mutation**
1. 首次挂载后 flush，输入框显示 `tom`
2. 用户输入 `jerry`，触发回写 `set("form.username", "jerry")`
3. flush 后 graph 与 DOM 保持一致

**预期 DOM 变化**
- 用户输入后，graph 值更新为 `jerry`
- 后续再次从 graph `set("form.username", "rose")` 时，输入框同步显示 `rose`

**不应该发生什么**
- 不应因为程序性写入再次触发无限回写
- 不应导致整个表单 rerender

## 12. 实现约束
- Stage 1 的 binding 输入必须是“单一路径”，不是表达式 AST
- binding 首次渲染与后续更新必须走同一套 flush 机制
- `StateGraph` 是唯一真源；DOM 只是投影，不反向持有权威状态
- 所有清理责任都要显式落在 binding owner 上，不能依赖隐式 GC 推断生命周期

## 13. 本文档刻意不展开的内容
- 模板编译器设计
- `ForBinding` / `IfBinding`
- 通用表达式系统
- 样式和类名复杂绑定
- 与 Preact 的迁移/替换方案

以上内容不是 Stage 1 阻塞项，prototype 不需要等待它们定义完成。
