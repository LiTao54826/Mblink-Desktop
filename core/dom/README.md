# DOM Module | DOM 模块

## Overview | 概览

`core/dom/` contains the repository-visible DOM tree model and related helpers.  
`core/dom/` 包含仓库中可见的 DOM 树模型及相关辅助组件。

## Core Areas | 核心区域

- node tree types such as `Node`, `Element`, `Text`, and `Document`  
  `Node`、`Element`、`Text`、`Document` 等节点树类型
- attribute, style, and class-list handling  
  属性、样式与 class-list 处理
- event-facing DOM data structures  
  面向事件系统的 DOM 数据结构
- selector, range, and selection helpers  
  选择器、range 与 selection 辅助能力
- DOM observers and dirty-node tracking  
  DOM observer 与脏节点追踪
- HTML element specializations under `elements/`  
  `elements/` 下的 HTML 元素特化实现

## Repository-Visible Layout | 仓库可见目录

- `bindings/` — JavaScript-facing DOM bindings / 面向 JavaScript 的 DOM 绑定
- `observers/` — DOM observer support / DOM observer 支持
- `selection/` — selection and range support / selection 与 range 支持
- `style/` — style-related DOM helpers / DOM 样式相关辅助
- `utils/` — token list and string-map helpers / token list 与 string-map 工具
- `elements/` — specialized HTML element classes / 专用 HTML 元素类

## Dependencies | 依赖关系

Depends on | 依赖：

- `core/utils`
- `third_party/lexbor`

Used by | 被依赖：

- `core/quickjs`
- `core/event`
- `core/render`
- `core/layout`

## Notes | 说明

- DOM behavior claims should be limited to what the current source tree proves  
  DOM 能力表述应限定在当前源码树可证明的范围内
- exact API shape and element coverage must follow implementation files  
  具体 API 形态与元素覆盖范围应以实现文件为准
- DOM operations should generally be treated as main-thread runtime work unless documented otherwise  
  除非另有说明，DOM 操作一般应视为主线程运行时工作

## Related Docs | 相关文档

- `docs/ARCHITECTURE.md`
- `core/quickjs/README.md`
- `core/render/README.md`
- `core/layout/README.md`

