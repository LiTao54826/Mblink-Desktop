# MBink 原生数据绑定系统设计预案

## 1. 背景
当前 MBink 主要有两类数据驱动方式：
- 宿主状态 / SharedObject 驱动 JS
- JS/Preact 通过 rerender + diff 更新 DOM

这套模式足够灵活，但如果未来希望获得：
- 更细粒度的原生节点更新
- 更统一的宿主 / JS 双向数据流
- 更少的组件级 rerender 成本

则需要考虑一套 **MBink 原生数据绑定系统**。

## 2. 目标
设计一套面向 MBink 自身 DOM/组件体系的原生绑定能力，使元素及其子树可以基于共享数据域进行直接绑定。

目标能力：
1. 元素拥有本地数据域（scope）
2. 子元素可继承访问父级数据域
3. text / attr / style / visible / list / model 支持绑定
4. 数据变更时只更新受影响节点，不必整棵树 rerender
5. 宿主语言与 JS 走统一 mutation 通道

## 3. 非目标
本预案默认不追求：
- 完全兼容 Vue 模板语法
- 直接替代全部 JS 框架生态
- 在第一阶段支持任意 JS 表达式求值

## 4. 核心判断
真正值钱的不是“元素挂一个 C++ data 对象”，而是下面四件事：
- 单一真源（single source of truth）
- path 级订阅
- binding 引擎
- 最小粒度更新调度

如果只有元素私有 data，但没有这四层，系统不会天然变成高性能响应式框架。

## 5. 推荐总体架构
### 5.1 StateGraph
新增原生状态图对象，作为统一权威状态层。

建议能力：
- `get(path)`
- `set(path, value)`
- `delete(path)`
- `subscribe(path or subtree)`
- `beginBatch()/endBatch()`
- revision/version

### 5.2 ElementScope
每个元素可选持有一个 scope。

scope 规则建议：
- 就近查找：子元素先查自身，再向父级查
- 可挂载本地字段
- 可引用全局状态/共享状态
- 可做只读映射或可写映射

### 5.3 Binding
绑定建议拆为显式对象，而不是散落在属性字符串里。

基础绑定类型：
- TextBinding：文本内容
- AttrBinding：属性
- StyleBinding：样式
- ClassBinding：类名
- VisibleBinding：显示/隐藏
- IfBinding：条件节点存在性
- ForBinding：列表渲染
- ModelBinding：表单值双向绑定

### 5.4 Scheduler
数据变更后不触发整棵 rerender，而是：
1. 找到受影响 binding
2. 将对应元素/节点加入更新队列
3. 统一批量刷新

## 6. 单一真源建议
推荐：
- 原生 StateGraph 为唯一真源
- JS 逻辑读取时通过受控代理访问
- 宿主更新、用户输入、内部控件状态变化，都统一发 mutation

不推荐：
- C++ 一份数据
- JS 再维护一份普通对象副本
- 两套数据并存再做松散同步

否则会重演 SharedObject 早期的双向覆盖问题。

## 7. 建议的数据流
### 7.1 宿主写入
Host → StateGraph.set(path, value) → 通知订阅 binding → 精确更新节点

### 7.2 JS 事件写入
Input/change/click handler → mutation → StateGraph.set(path, value) → 回写绑定节点

### 7.3 只读访问
绑定表达式求值时读取 StateGraph 的 path 值，并记录依赖。

## 8. 与现有 Preact 的关系
建议不要一开始强行替代 Preact，而是提供两种模式：

### 模式 A：Native Binding Mode
适合：
- 表单
- 设置页
- 面板
- 原生控件密集界面

特点：
- 不依赖 VDOM rerender
- 走原生 binding 更新

### 模式 B：Framework Mode
继续支持：
- Preact / React 风格 render
- SharedObject / host state

意义：
- 降低迁移风险
- 允许新旧模式并存

## 9. 推荐 MVP 范围
第一阶段不要做太大，建议只做：
1. StateGraph
2. TextBinding
3. AttrBinding
4. VisibleBinding
5. ModelBinding（input/value/checked）
6. scope 继承
7. batch 更新

先不做：
- 通用模板编译器
- 复杂列表 diff
- 高级表达式系统
- 完整组件化 DSL

## 10. 性能预期
该方案在以下场景有明显优势：
- 大量静态结构 + 少量动态文本/属性
- 高频局部更新
- 表单/配置面板/仪表盘
- 宿主频繁推送少量字段变化

优势来源：
- 不需要组件 rerender
- 不需要 VDOM diff
- 只更新真正受影响节点

## 11. 风险点
1. 需要额外维护一套 binding 生命周期
2. 与现有 DOM/组件模型的耦合要设计好
3. 表达式求值和依赖收集策略需谨慎
4. 列表与条件渲染一旦设计过早，复杂度会快速上升
5. 若与 Preact 混用不清晰，调试成本会增加

## 12. 分阶段实施建议
### 阶段 1：原型验证
- 做 StateGraph
- 做少量基础 binding
- 在几个原生控件上验证性能与可维护性

### 阶段 2：局部落地
- 先用于表单/设置页/宿主驱动面板
- 不要求替换现有 Preact 页面

### 阶段 3：能力扩展
- 列表绑定
- 条件渲染
- 更完整的模板/表达式能力
- 更细粒度调度统计与调试工具

## 13. 结论
如果 MBink 的长期方向是“原生高性能数据驱动 UI”，本预案很值得推进。

但它属于 **框架级能力建设**，不是 SharedObject 的小修补。更适合作为后续独立演进方向，在明确需要自有 binding engine 时启动，而不是与当前问题绑定一起实现。
