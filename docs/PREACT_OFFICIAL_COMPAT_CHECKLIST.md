# MBlink 兼容官方 Preact 宿主环境补齐清单

## 目标

让 MBlink 从“可运行自实现 preact”提升到“可直接运行官方 `preact` / `preact/hooks` / JSX automatic runtime”，并尽量减少 render、hydrate、SVG、事件、生态库兼容问题。

---

## 当前已具备

- `preact`、`preact/hooks` 模块注册
- `document.createElement()`
- `document.createElementNS()`
- `document.createTextNode()`
- `document.createDocumentFragment()`
- `document.body/head/documentElement/activeElement`
- `document.createRange()`
- `window.getSelection()/document.getSelection()`
- `appendChild/removeChild/insertBefore/replaceChild`
- `querySelector/querySelectorAll/getElementById/getElementsByTagName/getElementsByClassName`
- `className/classList/style`
- `innerHTML/textContent`
- `Element.addEventListener/removeEventListener/dispatchEvent`
- `requestAnimationFrame/cancelAnimationFrame`
- `focus/blur/selectionStart/selectionEnd`
- `MutationObserver`
- `getComputedStyle`

---

## P0：必须先补

### 1. 模块入口
- [ ] `preact/jsx-runtime`
- [ ] `preact/jsx-dev-runtime`

说明：缺这两个时，官方 JSX automatic runtime 无法直接工作。

### 2. 官方源码/测试已确认的核心宿主缺口
- [ ] `Element.localName`
- [ ] `Element.namespaceURI`
- [ ] `document.createComment()`
- [ ] 文本/注释节点 `data`
- [ ] `<template>.content`

说明：这些不是经验项，而是 `参考/preact/src/diff/index.js` 与 `render.test.jsx` / `svg.test.jsx` / `mathml.test.jsx` 中直接依赖的能力；缺任一项都会影响 render/hydrate/SVG。

### 3. hydrate / diff 必需节点语义
- [ ] `Node.isConnected`
- [ ] `Node.compareDocumentPosition()`
- [ ] `Node.nodeType == 8` 的注释节点暴露一致
- [ ] `firstChild/nextSibling/childNodes` 顺序语义与浏览器一致
- [ ] `cloneNode(true)` 在 `template/content/普通节点` 上行为一致

说明：官方 hydrate 会读取 `$s` / `/$s` 注释 marker，并依赖已有 DOM 复用、兄弟节点遍历和节点克隆结果。

### 4. 事件系统最小闭环
- [ ] `window.addEventListener/removeEventListener/dispatchEvent` 真正实现
- [ ] `document.addEventListener/removeEventListener` 冒泡/捕获链一致性
- [ ] `document.createEvent('Event')`
- [ ] `Event.initEvent()`
- [ ] `eventPhase`
- [ ] `CustomEvent`

说明：`events.test.jsx` 和 `hydrate.test.jsx` 直接覆盖 `dispatchEvent(createEvent('click'))`、捕获阶段、`PointerCapture` 后缀规则与事件重绑定。

### 5. 调度与时间 API
- [ ] `queueMicrotask`
- [ ] `performance.now()`
- [ ] 校验 Promise microtask flush 与宿主事件循环时序一致

说明：官方 preact core 不是每处都直接调用，但 hooks、调度、调试和外围生态会依赖。

---

## P1：建议尽快补

### 6. SVG / MathML / namespace 细节
- [ ] `Element.ownerSVGElement`
- [ ] `Element.hasAttributeNS()`
- [ ] `Element.removeAttributeNS()` 行为校验
- [ ] `tagName/localName/namespaceURI` 在 SVG / MathML / `foreignObject` 下保持浏览器一致

说明：`svg.test.jsx` 与 `mathml.test.jsx` 明确覆盖 `foreignObject`、`xlinkHref`、SVG/MathML 到 HTML 的命名空间切换。

### 7. DOM 树与节点补完
- [ ] `Node.contains()`
- [ ] `Node.ownerDocument` 统一性校验
- [ ] `firstElementChild/lastElementChild`
- [ ] `nextElementSibling/previousElementSibling`
- [ ] `childElementCount`

说明：这组能力更多影响遍历、工具链与外围生态；对官方 preact core 不是最先阻塞，但建议尽快补齐。

### 8. focus / selection / editable 一致性
- [ ] `activeElement + focus()` 在重排/hydrate 后保持稳定
- [ ] `selectionStart/selectionEnd/setSelectionRange` 回归校验
- [ ] `document.createRange()` 与 `window.getSelection()` 联动校验
- [ ] `contentEditable + dangerouslySetInnerHTML` 场景下 caret 不漂移

说明：这些能力仓库里已部分存在，但官方 `focus.test.jsx`、`render.test.jsx` 对焦点保持和选区恢复要求比最初判断更高。

---

## P2：生态兼容建议

### 9. 常见事件类型
- [ ] `MouseEvent`
- [ ] `KeyboardEvent`
- [ ] `InputEvent`
- [ ] `FocusEvent`
- [ ] `PointerEvent`（可选）
- [ ] `WheelEvent`（可选）
- [ ] `CompositionEvent`（可选）

### 10. 生态模块
- [ ] `preact/compat`
- [ ] `preact/debug`
- [ ] `preact/devtools`

### 11. 常见宿主能力
- [ ] `Element.dataset`
- [ ] `Element.toggleAttribute()`
- [ ] `document.baseURI`
- [ ] `location`
- [ ] `navigator.userAgent`
- [ ] `customElements` / customized built-in elements（`is="..."`）
- [ ] `ResizeObserver`（可选）
- [ ] `IntersectionObserver`（可选）
- [ ] `Element.getRootNode()`（可选）

---

## hydrate 专项检查

如果目标包含官方 `hydrate()`，需要额外验证：

- [ ] `firstChild/nextSibling/childNodes` 顺序与浏览器一致
- [ ] 注释节点 `nodeType == 8`、`data == '$s'/'/$s'` 可正确暴露
- [ ] 文本节点 `nodeType/nodeValue/data/textContent` 一致
- [ ] `<template>.content` 与 `cloneNode(true)` 一致
- [ ] `dispatchEvent(createEvent('click'))` 可重绑到 hydration 后节点
- [ ] `eventPhase` / capture / `PointerCapture` 后缀规则一致
- [ ] `activeElement + focus()` 恢复逻辑稳定
- [ ] SVG / MathML 节点与属性在 hydration 下不漂移

---

## 建议改动入口

### 模块注册
- `core/api/mbink.cpp`
- `tools/esm_loader/main.cpp`

### Document / Window 绑定
- `core/quickjs/window_bindings.cpp`
- `core/quickjs/document_bindings_impl.cpp`

### Node / Element / Event 绑定
- `core/quickjs/bindings/js_node.cpp`
- `core/quickjs/bindings/js_element.cpp`
- `core/quickjs/bindings/js_event.cpp`

### 运行时引导
- `js/runtime/bootstrap.js`

### 对照验证
- `tests/unit/quickjs/test_dom_bindings.cpp`
- `examples/preact_demo`

---

## 最小落地顺序

1. 先补 `preact/jsx-runtime` / `preact/jsx-dev-runtime`
2. 再补 `localName/namespaceURI/createComment/data/template.content`
3. 再补 hydration 关键节点语义：注释节点、`isConnected`、`compareDocumentPosition`、`cloneNode(true)`
4. 再补事件最小闭环：`window/document` 事件系统、`document.createEvent()`、`initEvent()`、`eventPhase`、`CustomEvent`
5. 最后补 `queueMicrotask` / `performance.now()` 与 focus/selection 细节回归
