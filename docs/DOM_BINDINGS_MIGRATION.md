# DOM Bindings Migration Plan | DOM 绑定迁移方案

## 1. Purpose | 目标

本文档定义 `core/dom/bindings/dom_bindings.cpp` 与 `core/quickjs/*` 双套 DOM 绑定的统一方案，目标是：

- 明确当前真实运行路径
- 指定唯一主线绑定体系
- 给出分阶段迁移与删除策略
- 降低 API 不一致、双维护、行为漂移风险

## 2. Current State | 当前状态

仓库中仍存在两套 DOM 绑定体系：

1. 旧的集中式绑定：`core/dom/bindings/dom_bindings.cpp`
2. 新的模块化绑定：`core/quickjs/document_bindings_impl.cpp` + `core/quickjs/bindings/*`

其中 `core/quickjs/*` 已是运行时唯一主线；`DOMBindings` 仅剩 legacy/兼容与清理兜底职责。

## 3. Verified Facts | 已验证事实

### 3.1 Initialization order | 初始化顺序

`core/api/mbink.cpp` 当前执行：

- `windowBindings->InitBindings()`

随后 `WindowBindings::InitBindings()` 会调用：

- `CanvasBindings::Init(runtime_->GetContext())`
- `InitImageConstructor(runtime_->GetContext())`
- `BindDocumentAPIs(runtime_->GetContext(), window_.get())`

### 3.2 Actual example path | 示例实际命中路径

`core/quickjs/document_bindings_impl.cpp` 中：

- `JS_SetPropertyStr(ctx, global, "document", document)`
- `JS_SetPropertyStr(ctx, window_obj, "document", JS_DupValue(ctx, document))`

全局 `document` 由 quickjs 主线直接注入到 global / window。

结论：

- 示例里的 `document.body`
- `document.createElement()`
- `document.getElementById()`
- `document.head.appendChild(...)`

最终命中的都是 `core/quickjs/*` 这套模块化绑定。

### 3.3 Special elements | 特殊元素

`terminal` / `logview` 示例依赖的能力，如：

- `execute()`
- `write()`
- `clear()`
- `append()`
- `nextMatch()` / `prevMatch()`

由 `core/quickjs/bindings/js_element.cpp` 在包装对象上动态挂载。

### 3.4 Legacy coupling | 遗留耦合

`core/dom/bindings/dom_bindings.cpp` 已反向依赖 quickjs 新系统中的类型与清理逻辑，例如：

- `#include "quickjs/bindings/js_element.h"`
- `bindings::GetElementClassID()`
- `bindings::ClearElementListenerBindings(...)`

这说明旧系统已经不是独立主线，更像遗留兼容层。

## 4. Decision | 决策

### 4.1 Keep | 保留

保留 `core/quickjs/*` 模块化绑定作为唯一主线：

- `core/quickjs/window_bindings.cpp`
- `core/quickjs/document_bindings_impl.cpp`
- `core/quickjs/bindings/js_node.cpp`
- `core/quickjs/bindings/js_element.cpp`
- `core/quickjs/bindings/js_event.cpp`
- `core/quickjs/bindings/js_range.cpp`
- `core/quickjs/bindings/js_selection.cpp`
- `core/quickjs/bindings/js_mutation_observer.cpp`
- `core/quickjs/bindings/js_style_declaration.cpp`

### 4.2 Remove | 下线

逐步下线 `core/dom/bindings/dom_bindings.cpp` 及其对应的旧 `Document` / `Element` / `Event` wrapper 主路径。

## 5. Migration Principles | 迁移原则

- 不再给 `DOMBindings` 增加新 DOM API
- 所有新功能只补到 `core/quickjs/*`
- 先统一返回路径，再删旧实现
- 每个阶段都必须可回滚
- 优先保留行为稳定性，不做无关重构

## 6. Migration Phases | 迁移阶段

### Phase 0: Freeze legacy | 冻结旧系统

- 标记 `DOMBindings` 为 legacy
- 禁止继续向其补充新 API
- 新增缺失能力仅实现到 quickjs 主线

### Phase 1: Audit usage | 盘点旧系统剩余依赖

已确认的当前清单如下。

#### 仍在运行时主路径使用

- `core/api/mbink.cpp`
  - 销毁时仅调用 `DOMBindings::Cleanup(nullptr)`
  - 只保留 legacy EventLoop 状态兜底
- `tools/esm_loader/main.cpp`
  - 退出流程中仅调用 `DOMBindings::Cleanup(nullptr)`
  - quickjs 主线清理由 `WindowBindings::Cleanup()` 负责
- `core/quickjs/window_bindings.h/.cpp`
  - quickjs 主线已接管 `EventLoop` bridge
  - 运行时与工具路径统一通过 `WindowBindings::SetActiveEventLoop()/GetActiveEventLoop()` 访问

#### 已从 legacy public surface 移除

- `DOMBindings::Init`
  - 运行时主路径已移除
  - public 声明与实现已删除
- `DOMBindings::SetGlobalDocument`
  - 运行时主路径已移除
  - quickjs 单测主路径已迁移到 `WindowBindings`
  - public 声明与实现已删除
- `DOMBindings::SetGlobalEventLoop / GetGlobalEventLoop`
  - quickjs 主线路径已迁移到 `WindowBindings::SetActiveEventLoop()/GetActiveEventLoop()`
  - public 声明与实现已删除
- `DOMBindings::SetGlobalTaskScheduler`
  - legacy timer bridge 已无外部调用
  - public 声明与实现已删除
- `DOMBindings::Cleanup(ctx)`
  - 当前仍保留给 legacy wrapper 场景

#### 只剩旧系统内部自调用

- `WrapElement / WrapDocument / WrapNode / UnwrapElement / UnwrapDocument`
  - 当前检索结果主要集中在 `core/dom/bindings/dom_bindings.cpp` 内部实现相互调用
  - 暂未发现示例主路径直接依赖这些旧 wrapper 返回值
- `element_cache_ / text_cache_ / document_cache_`
  - 当前审计结果显示为 pure legacy cache
  - quickjs 主线使用 `DOMBindingMap`，未直接依赖这 3 个 cache

#### 当前分类结论

- `Init / SetGlobal*`
  - 已不再承担运行时主路径职责
  - 当前已从 legacy public surface 删除
- `Cleanup`
  - 主路径仅保留 `Cleanup(nullptr)` 状态兜底
  - `Cleanup(ctx)` 当前只为旧测试 / legacy wrapper 清理路径保留
- `Wrap* / Unwrap*`
  - 暂未发现运行时主路径外部调用证据，当前更像旧绑定体系内部实现细节

输出结果应区分：

- 运行时仍在使用
- 只剩兼容/清理用途
- 已无实际调用价值

### Phase 2: Single source of truth | 收口单一 document 路径

- [x] 主路径已移除 `DOMBindings::SetGlobalDocument()`
- [x] 全局 `document` 主路径统一由 `BindDocumentAPIs()` 注入
- [ ] 继续验证 `window.document === document` 与相关回归

### Phase 3: API parity | 补齐 API 一致性

以 quickjs 主线为标准，补齐并验证：

- `Element` 常用方法与属性
- `Document` 常用查询与创建 API
- `Event` / `Range` / `Selection` / `MutationObserver`
- 特殊元素能力：`terminal` / `logview`

当前进展：

- [x] 已补 `Element.prototype.click()` 到 `core/quickjs/bindings/js_element.cpp`
- [x] 已补 `document.getElementsByTagName()` 到 `core/quickjs/document_bindings_impl.cpp`
- [x] 已补 `document.getElementsByClassName()` 到 `core/quickjs/document_bindings_impl.cpp`
- [x] 已补 `Event.timeStamp` 到 `core/quickjs/bindings/js_event.cpp`
- [x] 已补 `Event.stopImmediatePropagation()` 到 `core/quickjs/bindings/js_event.cpp`
- [x] `diagnostics` 通过
- [x] `cmake --build build --config Debug --target mbink_api` 通过
- [ ] 继续盘点其余 API parity（含 `Event` / `Range` / `Selection` / `MutationObserver`）

### Phase 4: Cleanup migration | 迁移清理逻辑

把 `DOMBindings::Cleanup()` 中仍有价值的逻辑迁到 quickjs 主线，包括：

- listener cleanup
- cache release
- `DOMBindingMap` 清理
- 全局对象解绑

当前进展：

- [x] 已给 `core/quickjs/window_bindings.h/.cpp` 新增 `WindowBindings::Cleanup()`
- [x] quickjs 主线路径退出时已优先调用 `WindowBindings::Cleanup()`
- [x] `core/api/mbink.cpp` / `tools/esm_loader/main.cpp` 已改为只把 `DOMBindings::Cleanup(nullptr)` 作为 legacy EventLoop 状态兜底
- [x] quickjs 主线路径已接管 `EventLoop` bridge，不再通过 `DOMBindings::SetGlobalEventLoop()` / `GetGlobalEventLoop()` 访问
- [x] quickjs 单测 `tests/unit/quickjs/test_dom_bindings.cpp` 已迁移到 `WindowBindings`
- [x] `DOMBindings::Cleanup()` 已显式收窄：`nullptr` 路径不再参与 quickjs listener / global document / `DOMBindingMap` 清理
- [x] 已把 legacy cache 清理抽成独立 helper，`element_cache_ / text_cache_ / document_cache_` 边界更明确
- [x] `Cleanup(ctx)` 已继续收窄，不再清理 quickjs 主线注入的 `document` 或 `DOMBindingMap`

### Phase 5: Remove legacy wrappers | 删除旧 wrapper

在确认无真实运行时依赖后，删除：

- 旧 `Document` / `Element` / `Event` JSClass 注册
- 旧 `Wrap* / Unwrap*` 实现
- 已无外部调用的 legacy timer bridge / cache / finalizer

## 7. Risks | 风险

主要风险：

- JS 对象身份不一致
- 事件监听器与 `on*` 属性清理不完整
- 特殊元素能力丢失
- `document.body` / `head` / `documentElement` 在 load 后失效
- GC / cache / opaque class id 清理错误

## 8. Acceptance Criteria | 验收标准

迁移完成后应满足：

- 全局 `document` 只有一条注入路径
- 示例全部运行在 quickjs 主线 wrapper 上
- `window.document === document`
- `terminal` / `logview` 示例功能完整
- 不再出现双套 `Element` API 不一致
- 旧 `DOMBindings` 不再承担 DOM 主路径职责

## 9. Recommended Execution Order | 建议执行顺序

1. 冻结旧系统
2. 盘点真实依赖
3. 收口全局 `document`
4. 补齐 quickjs API 缺口
5. 迁移 cleanup / cache
6. 删除旧 wrapper
7. 回归验证 examples + tests

## 10. Short Conclusion | 简短结论

当前示例实际使用的是 `core/quickjs/*` 模块化绑定。

因此：

- `core/quickjs/*` 应保留并作为唯一主线
- `core/dom/bindings/dom_bindings.cpp` 应逐步退役
- 迁移重点不是“再维护两套”，而是“尽快收口为一套”
