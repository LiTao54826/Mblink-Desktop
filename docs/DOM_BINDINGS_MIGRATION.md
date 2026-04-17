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

当前真实状态：

#### 运行时仍保留的 legacy 入口

- `core/api/mbink.cpp`
  - 销毁时仅调用 `DOMBindings::Cleanup(nullptr)`
  - 只保留 legacy 兼容/兜底语义
- `tools/esm_loader/main.cpp`
  - 退出流程中仅调用 `DOMBindings::Cleanup(nullptr)`
  - quickjs 主线路径清理由 `WindowBindings::Cleanup()` 负责

#### 主线路径已收口到 quickjs

- `core/quickjs/window_bindings.h/.cpp`
  - quickjs 主线已接管 `EventLoop` bridge
  - 统一通过 `WindowBindings::SetActiveEventLoop()/GetActiveEventLoop()` 访问
- `core/quickjs/document_bindings_impl.cpp`
  - `document` 查询与注入走 quickjs 主线
- `tests/unit/quickjs/test_dom_bindings.cpp`
  - 回归已迁移到 `WindowBindings` / quickjs 绑定

#### 已删除的 legacy public surface / retired 实现

- `DOMBindings::Init`
- `DOMBindings::SetGlobalDocument`
- `DOMBindings::SetGlobalTaskScheduler`
- `DOMBindings::SetGlobalEventLoop`
- `DOMBindings::GetGlobalEventLoop`
- 旧 `Document / Element / Event` JSClass 注册
- 旧 `Wrap* / Unwrap*` 实现
- legacy cache / finalizer：`element_cache_ / text_cache_ / document_cache_`
- legacy `g_event_loop`
- legacy timer bridge dead code

#### 当前分类结论

- `core/quickjs/*` 已是唯一 DOM 绑定主线
- `WindowBindings::Cleanup()` 负责 quickjs 主线 cleanup
- `DOMBindings::Cleanup(nullptr)` 仅保留兼容/兜底语义

### Phase 2: Single source of truth | 收口单一 document 路径

- [x] 主路径已移除 `DOMBindings::SetGlobalDocument()`
- [x] 全局 `document` 主路径统一由 quickjs 绑定注入
- [x] 已验证 `window.document === document`

### Phase 3: API parity | 补齐 API 一致性

当前已补齐并验证：

- [x] `Element.prototype.click()`
- [x] `Element.prototype.hasAttribute()`
- [x] `Element.prototype.dispatchEvent()`
- [x] `document.getElementsByTagName()`
- [x] `document.getElementsByClassName()`
- [x] 全局 `Event` constructor
- [x] `Event.timeStamp`
- [x] `Event.stopImmediatePropagation()`
- [x] `diagnostics` 通过
- [x] `DOMBindingsTest.*` 37/37 通过

### Phase 4: Cleanup migration | 迁移清理逻辑

- [x] 已给 `WindowBindings` 新增并启用 `Cleanup()`
- [x] quickjs 主线路径退出时已优先调用 `WindowBindings::Cleanup()`
- [x] `DOMBindings::Cleanup()` 已收窄为兼容层
- [x] `Cleanup(nullptr)` 不再参与 quickjs 主线路径清理
- [x] 已验证 `WindowBindings::Cleanup()` 与 `DOMBindings::Cleanup(nullptr)` 路径

### Phase 5: Remove legacy wrappers | 删除旧 wrapper

- [x] 已删除旧 `Document / Element / Event` JSClass 注册
- [x] 已删除旧 `Wrap* / Unwrap*` 实现
- [x] 已删除 legacy cache / finalizer 残留
- [x] 已删除 legacy `g_event_loop` 残留
- [x] `core/dom/bindings/dom_bindings.cpp` 仅保留兼容 cleanup 与 `Image` constructor

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

当前 DOM 绑定迁移已完成主线路径收口。

因此：

- `core/quickjs/*` 是唯一 DOM 绑定主线
- `core/dom/bindings/dom_bindings.cpp` 只保留兼容 cleanup 与 `Image` constructor
- 不再维护双套 `Element / Document / Event` 绑定实现
