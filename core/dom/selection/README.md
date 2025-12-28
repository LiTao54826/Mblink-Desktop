# core/dom/selection/

## 概述

文本选择和 CSS 选择器子模块，提供文本范围管理和元素查询功能。

## 模块列表

- `selection` - Selection API 实现，管理用户文本选择
- `range` - Range 对象实现，表示文档片段
- `selector_engine` - CSS 选择器引擎，支持 querySelector/querySelectorAll

## 依赖关系

- 依赖: core/dom (核心类)
- 被依赖: core/dom/bindings, core/editing

## 使用示例

```cpp
#include "core/dom/selection/selector_engine.h"

// 使用选择器查询元素
auto elements = SelectorEngine::QueryAll(document, ".my-class");
```
