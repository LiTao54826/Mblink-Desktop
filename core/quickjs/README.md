# QuickJS Module | QuickJS 模块

## Overview | 概览

`core/quickjs/` contains the repository-visible JavaScript runtime integration built around QuickJS.  
`core/quickjs/` 包含基于 QuickJS 的 JavaScript 运行时接入层。

## Main Areas | 核心区域

- script evaluation  
  脚本执行
- DOM and Window bindings  
  DOM 与 Window 绑定
- console and timer bindings  
  console 与 timer 绑定
- event-facing JavaScript integration  
  面向事件系统的 JavaScript 接入
- module loading  
  模块加载
- Preact-related runtime glue  
  Preact 相关运行时胶水层

## Repository-Visible Files | 仓库可见文件

- `quickjs_runtime.h` / `.cpp` — runtime core / 运行时核心
- `window_bindings.h` / `.cpp` — Window API bindings / Window API 绑定
- `dom_bindings.h` / `.cpp` — DOM API bindings / DOM API 绑定
- `event_bindings.h` / `.cpp` — event bindings / 事件绑定
- `console_bindings.h` / `.cpp` — console bindings / console 绑定
- `timer_bindings.h` / `.cpp` — timer bindings / 定时器绑定
- `preact_renderer.h` / `.cpp` — Preact renderer glue / Preact 渲染桥接
- `preact_bindings.h` / `.cpp` — Preact API bindings / Preact API 绑定
- `module_loader.h` / `.cpp` — module loading / 模块加载
- `js_utils.h` / `.cpp` — JS helpers / JS 工具

## Dependencies | 依赖关系

QuickJS-facing code visibly interacts with:  
从仓库可见，QuickJS 相关代码与以下模块存在交互：

- `third_party/quickjs`
- `core/dom`
- `core/window`
- `core/event`
- `core/network`
- `core/utils`

## Notes | 说明

- exact JavaScript feature coverage must be verified from implementation and tests  
  具体 JavaScript 能力覆盖应结合实现与测试验证
- module semantics and browser-compatibility claims should stay conservative  
  模块语义与浏览器兼容性表述应保持保守
- Preact support exists in the repository, but end-to-end behavior still depends on runtime validation  
  仓库中存在 Preact 支持，但端到端表现仍依赖运行时验证

## Related Docs | 相关文档

- `docs/ARCHITECTURE.md`
- `core/dom/README.md`
- `core/event/README.md`
- `docs/BINDINGS.md`

