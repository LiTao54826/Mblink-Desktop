# MBlink 直接运行官方 Preact 任务指导

## 目标

唯一目标：**让 MBlink 直接运行官方版本的 `preact`、`preact/hooks`、`preact/jsx-runtime`、`preact/jsx-dev-runtime`。**

本任务**不再考虑兼容仓库当前自定义 `js/preact/*` 实现**，不再以它为语义基准，也不再为了兼容它而设计桥接层。

官方基准目录：
- `third_party/preact`
- `third_party/preact/hooks`
- `third_party/preact/jsx-runtime`
- `third_party/preact/jsx-dev-runtime`

`参考/preact` 仅作参考来源，不再直接接入运行链路。

---

## 总原则

1. **以官方源码/官方包结构为准**
   - `preact`
   - `preact/hooks`
   - `preact/jsx-runtime`
   - `preact/jsx-dev-runtime`
2. **优先跑通官方包加载链路**，其次再补宿主 DOM 缺口。
3. **不再维护“自定义 preact 兼容层”**。
4. **不做语义近似桥接**，要么直接运行官方模块，要么视为未完成。

---

## 当前目标拆解

### Phase 1：跑通官方包加载

必须完成：
- [x] `import 'preact'` 可直接工作
- [x] `import 'preact/hooks'` 可直接工作
- [x] `import 'preact/jsx-runtime'` 可直接工作
- [x] `import 'preact/jsx-dev-runtime'` 可直接工作

实施要求：
- 运行时必须解析官方包，而不是映射到 `globalThis.Preact` 的自定义实现
- 优先利用已有 QuickJS 包解析能力：`package.json` / `exports` / `main` / `node_modules`
- 如果现有解析链路不足，就补解析能力或补官方包接入方式

完成判定：
- 单测中能直接从官方包路径导入上述 4 个模块
- 不依赖 `js/preact/preact.js`
- 不依赖 `js/preact/hooks.js`

---

### Phase 2：跑通官方 JSX automatic runtime

必须完成：
- [x] 官方 `jsx-runtime` 可创建 VNode
- [x] 官方 `jsx-dev-runtime` 可创建 VNode
- [x] 使用 automatic runtime 的最小 render 用例能执行

最小验证建议：
- `import { jsx } from 'preact/jsx-runtime'`
- `import { jsxDEV } from 'preact/jsx-dev-runtime'`
- `import { render } from 'preact'`
- `render(jsx('div', { children: 'hello' }), document.body)`

完成判定：
- 不再手写 `jsx/jsxs/jsxDEV` 桥接实现
- 直接使用官方实现

---

### Phase 3：补官方 render 首批宿主缺口

这些缺口是当前第一优先级：
- [x] `Element.localName`
- [x] `Element.namespaceURI`
- [x] `document.createComment()`
- [x] 文本/注释节点 `data`
- [x] `<template>.content`

依据：
- 实际运行基准：`third_party/preact/src/diff/index.js`
- 参考来源（不接入运行链路）：
  - `参考/preact/test/browser/render.test.jsx`
  - `参考/preact/test/browser/svg.test.jsx`
  - `参考/preact/test/browser/mathml.test.jsx`

完成判定：
- 官方 render 基础用例可运行
- SVG / MathML 基础节点创建不立即失败
- `select.value` / `option.selected` / `input.checked` / `input.defaultChecked` / `textarea.defaultValue` 最小受控 render 回归通过

---

### Phase 4：补 hydrate / diff 关键节点语义

必须完成：
- [x] `Node.isConnected`
- [x] `Node.compareDocumentPosition()`
- [x] 注释节点 `nodeType == 8`
- [x] `firstChild/nextSibling/childNodes` 顺序一致
- [x] `cloneNode(true)` 在普通节点和 `template.content` 上一致

依据：
- 官方 hydrate 会读取注释 marker：`$s` / `/$s`
- hydrate 依赖兄弟节点遍历、注释节点、节点克隆和已有 DOM 复用

完成判定：
- 官方 `hydrate()` 最小用例可执行
- 注释节点相关用例不再直接失败

---

### Phase 5：补事件与调度闭环

必须完成：
- [x] `window.addEventListener/removeEventListener/dispatchEvent`
- [x] `document.addEventListener/removeEventListener`
- [x] `document.createEvent('Event')`
- [x] `Event.initEvent()`
- [x] `eventPhase`
- [x] `CustomEvent`
- [x] `queueMicrotask`
- [x] `performance.now()`

完成判定：
- 官方事件基础用例能跑
- hooks / 调度相关最小回归不立即失败

---

## 非目标

以下内容在当前阶段**不要优先处理**：
- 为 `js/preact/*` 保持兼容
- 手写 `preact/jsx-runtime` 的伪实现
- 为现有自定义 Preact 行为做适配补丁
- 提前做 `preact/compat`、`preact/debug`、`preact/devtools`
- 先做大范围重构再验证

---

## 推荐改动入口

### 官方包接入 / 模块解析
- `core/api/mbink.cpp`
- `tools/esm_loader/main.cpp`
- `core/quickjs/quickjs_runtime.cpp`
- `core/quickjs/quickjs_runtime.h`

### 宿主 DOM / Event 绑定
- `core/quickjs/window_bindings.cpp`
- `core/quickjs/document_bindings_impl.cpp`
- `core/quickjs/bindings/js_node.cpp`
- `core/quickjs/bindings/js_element.cpp`
- `core/quickjs/bindings/js_event.cpp`

### 运行时与验证
- `js/runtime/bootstrap.js`
- `tests/unit/quickjs/test_dom_bindings.cpp`

---

## 验收顺序

1. 先让官方 `preact` 包链路可 import
2. 再让官方 `jsx-runtime` / `jsx-dev-runtime` 可 import
3. 再跑通官方最小 render
4. 再补 render 首批 DOM 缺口
5. 再补 hydrate 关键节点语义
6. 最后补事件与调度闭环

---

## 每阶段验收标准

### A. 包加载验收
- [x] 没有使用 `js/preact/preact.js`
- [x] 没有使用 `js/preact/hooks.js`
- [x] 4 个官方模块都能直接导入
- [x] `resource_package` 不再回退到 legacy Preact builtin
- [x] `app_bundler` 不再生成 `globalThis.Preact` / `globalThis.PreactHooks` 桥接模块
- [x] `mbink-ui-dev` 模板入口不再依赖 `Preact` / `PreactHooks` 全局对象

### B. render 验收
- [x] 官方 `render()` 可挂载文本节点
- [x] 官方 `render()` 可挂载普通元素
- [x] 官方 `render()` 在 SVG 节点创建上不立即失败

### C. hydrate 验收
- [x] 注释节点可见
- [x] hydration marker 可读取
- [x] 兄弟节点遍历一致

### D. 事件验收
- [x] `createEvent + dispatchEvent` 可工作
- [x] capture / bubble 基础流程可工作

---

### E. 真实示例 / UI Dev 验收
- [x] `examples/todo_app_js/app.js` 可通过 `--ui-dev-snapshot-file` 产出非空 snapshot，并稳定运行 30s+ 直到主动停止
- [x] `examples/official_preact_jsx_dev/app.js`（`.js` 内直接写 JSX）可在 `open/build/reload/snapshot/query/inspect/input-text/click` 下走官方 `preact` / `preact/hooks` 链路并完成状态更新
- [x] `examples/component_demo/app.js` 现已可稳定启动；此前崩溃根因是 `clearTimeout/cancelAnimationFrame` 清理路径里的定时器回调释放逻辑导致 iterator 失效并触发 native crash
- [x] snapshot 可见真实 UI 节点（如 `Todo List (Pure JS)`、`#todo-input`、按钮）

---


## 最终判定标准

只有满足以下条件，才算目标达成：

- MBlink 运行的是**官方版 Preact**，不是仓库自定义实现
- 官方 `preact` / `preact/hooks` / `preact/jsx-runtime` / `preact/jsx-dev-runtime` 可以直接运行
- `resource_package` / `app_bundler` / `mbink-ui-dev` 模板入口也走官方模块链路
- 代码运行链路里不再依赖 `js/preact/preact.js` / `js/preact/hooks.js`
- 后续兼容问题按官方测试和官方源码依赖逐项补齐

如果仍然需要靠自定义 `Preact` 全局对象桥接核心行为，则视为**目标未达成**；仅保留 shutdown 阶段对历史全局键名的清理，不视为运行依赖。
