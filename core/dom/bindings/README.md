# core/dom/bindings/

## 概述

JavaScript 绑定子模块，负责将 DOM API 暴露给 QuickJS 运行时。

## 模块列表

- `dom_bindings` - DOM API 绑定，包括 Node、Element、Document 等核心类的 JS 接口
- `canvas_bindings` - Canvas 2D API 绑定，提供 CanvasRenderingContext2D 接口

## 依赖关系

- 依赖: core/dom (核心类), core/dom/elements, core/dom/selection, core/quickjs
- 被依赖: core/bridge

## 使用示例

```cpp
#include "core/dom/bindings/dom_bindings.h"

// 注册 DOM 绑定到 JS 运行时
DOMBindings::Register(js_context);
```
