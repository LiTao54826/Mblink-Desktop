# core/dom/observers/

## 概述

观察者模式实现子模块，提供 DOM 变化监听和追踪功能。

## 模块列表

- `dom_observer` - DOM 观察器基类，定义观察接口
- `mutation_observer` - MutationObserver 实现，符合 W3C 标准
- `dirty_node_tracker` - 脏节点追踪器，用于增量更新优化

## 依赖关系

- 依赖: core/dom (核心类)
- 被依赖: core/render, core/layout

## 使用示例

```cpp
#include "core/dom/observers/mutation_observer.h"

// 创建 MutationObserver
auto observer = std::make_shared<MutationObserver>(callback);
observer->Observe(target_node, options);
```
