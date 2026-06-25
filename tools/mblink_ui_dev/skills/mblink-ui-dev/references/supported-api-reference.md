# MBlink UI Supported API Reference

Use this file when an agent is unsure whether a JavaScript, DOM, Preact, host bridge, or MBlink native-element API is supported by the current MBlink UI runtime.

This is a conservative compatibility list. Prefer APIs listed as supported. If an API is not listed here or in the linked references, add it only in a small isolated slice and verify it with `mblink-ui-dev build`, `snapshot`, `query_element`, `inspect`, `logs`, and `errors`.

## Runtime Globals

| API | Status | Notes |
|---|---|---|
| `globalThis` | supported | Runtime global object. |
| `window` | supported alias | Alias to the global object, not a full browser window. |
| `self` | supported alias | Alias to `window`. |
| `console.log(...)` | supported | Use for normal diagnostics. |
| `console.info(...)` | supported | Use for informational diagnostics. |
| `console.warn(...)` | supported | Use for warnings. |
| `console.error(...)` | supported | Also surfaces through JS error/log tooling. |
| `console.debug(...)` | supported | Lightweight debug output. |
| `queueMicrotask(callback)` | supported | Prefer for microtask scheduling when available. |
| `performance.now()` | supported | Use for timing. |
| `setTimeout(callback, delay, ...args)` | supported | Timer IDs can be passed to `clearTimeout`. |
| `clearTimeout(id)` | supported | Clears timeout timers. |
| `setInterval(callback, delay, ...args)` | supported | Timer IDs can be passed to `clearInterval`. |
| `clearInterval(id)` | supported | Clears interval timers. |
| `requestAnimationFrame(callback)` | supported | Backed by runtime scheduling; do not assume browser rendering semantics beyond frame callbacks. |
| `cancelAnimationFrame(id)` | supported | Cancels RAF callbacks. |

## Preact Modules

Prefer explicit ESM imports.

| Import | Status | Notes |
|---|---|---|
| `import { h, render } from 'preact'` | supported | Template baseline. |
| `import { useState } from 'preact/hooks'` | supported | Used by templates and bundled components. |
| `import { useEffect } from 'preact/hooks'` | supported | Used by templates and bundled components. |
| `import { useMemo } from 'preact/hooks'` | supported | Used by templates. |
| `import { useRef } from 'preact/hooks'` | supported | Used by bundled components and native element refs. |
| `import { useCallback } from 'preact/hooks'` | supported | Used by bundled shared-state helper. |
| `import { useLayoutEffect } from 'preact/hooks'` | supported | Used by bundled select component; verify if used heavily. |
| `preact/jsx-runtime` | supported module | Use only when project JSX settings require it and build is verified. |
| `preact/jsx-dev-runtime` | supported module | Dev JSX runtime module; verify build config. |

Avoid `preact-lite.js`, `globalThis.Preact`, `globalThis.PreactHooks`, legacy `js/preact/*`, React, React DOM, and `preact/compat` unless a project vendors and verifies them.

## Document APIs

| API | Status | Notes |
|---|---|---|
| `document.body` | supported | Dynamic getter for current body. |
| `document.documentElement` | supported | Dynamic getter for current root element. |
| `document.getElementById(id)` | supported | Prefer stable IDs for automation. |
| `document.getElementsByTagName(tag)` | supported | Returns array-like results. |
| `document.getElementsByClassName(className)` | supported | Returns array-like results. |
| `document.querySelector(selector)` | supported | Use simple selectors and validate. |
| `document.querySelectorAll(selector)` | supported | Use simple selectors and validate. |
| `document.createElement(tag)` | supported | Includes standard tags plus MBlink native tags such as `terminal` and `logview`. |
| `document.createElementNS(ns, qualifiedName)` | supported | Useful for SVG-like creation; verify rendering. |
| `document.createTextNode(text)` | supported | Safe for text nodes. |
| `document.createComment(text)` | supported | Safe for comments. |
| `document.createRange()` | supported | See Range APIs below. |
| `document.getSelection()` | supported | Alias to window/global selection. |
| `document.addEventListener(type, handler, options?)` | supported | Delegates through body. |
| `document.removeEventListener(type, handler, options?)` | supported | Delegates through body. |
| `document.dispatchEvent(event)` | supported | Delegates through body. |
| `window.getSelection()` / `getSelection()` | supported | Returns Selection object. |

## Node APIs

| API | Status | Notes |
|---|---|---|
| `node.parentNode` | supported | Read-only getter. |
| `node.firstChild` | supported | Read-only getter. |
| `node.lastChild` | supported | Read-only getter. |
| `node.nextSibling` | supported | Read-only getter. |
| `node.previousSibling` | supported | Read-only getter. |
| `node.childNodes` | supported | Read-only list. |
| `node.nodeName` | supported | Read-only getter. |
| `node.nodeType` | supported | Read-only getter. |
| `node.isConnected` | supported | Read-only getter. |
| `node.textContent` | supported | Get/set. |
| `node.nodeValue` | supported | Get/set. |
| `node.data` | supported | Get/set for text-like nodes. |
| `node.appendChild(child)` | supported | Use for direct DOM composition. |
| `node.removeChild(child)` | supported | Use for direct DOM composition. |
| `node.insertBefore(newNode, referenceNode)` | supported | Use for direct DOM composition. |
| `node.replaceChild(newChild, oldChild)` | supported | Use for direct DOM composition. |
| `node.cloneNode(deep?)` | supported | Verify deep clone behavior on complex trees. |
| `node.remove()` | supported | Removes node from parent. |
| `node.compareDocumentPosition(other)` | supported | Advanced DOM positioning. |

## Element APIs

| API | Status | Notes |
|---|---|---|
| `element.tagName` | supported | Read-only getter. |
| `element.localName` | supported | Read-only getter. |
| `element.namespaceURI` | supported | Read-only getter. |
| `element.id` | supported | Get/set; prefer stable IDs for automation. |
| `element.className` | supported | Get/set. |
| `element.classList.add(...)` | supported | Token class API. |
| `element.classList.remove(...)` | supported | Token class API. |
| `element.classList.toggle(token)` | supported | Token class API. |
| `element.classList.contains(token)` | supported | Token class API. |
| `element.children` | supported | Read-only child element list. |
| `element.attributes` | supported | Provides `length`, `item(index)`, and `getNamedItem(name)`. |
| `element.style` | supported | CSSStyleDeclaration subset. |
| `element.setAttribute(name, value)` | supported | Attribute mutation. |
| `element.setAttributeNS(ns, name, value)` | supported | Namespace attribute mutation. |
| `element.getAttribute(name)` | supported | Attribute lookup. |
| `element.getAttributeNS(ns, name)` | supported | Namespace attribute lookup. |
| `element.hasAttribute(name)` | supported | Attribute existence. |
| `element.removeAttribute(name)` | supported | Attribute removal. |
| `element.removeAttributeNS(ns, name)` | supported | Namespace attribute removal. |
| `element.querySelector(selector)` | supported | Use simple selectors and validate. |
| `element.querySelectorAll(selector)` | supported | Use simple selectors and validate. |
| `element.matches(selector)` | supported | Selector match. |
| `element.closest(selector)` | supported | Ancestor lookup. |
| `element.contains(other)` | supported | Containment check. |
| `element.addEventListener(type, handler, options?)` | supported | Prefer this over inline handler properties for non-trivial components. |
| `element.removeEventListener(type, handler, options?)` | supported | Remove registered listeners. |
| `element.dispatchEvent(event)` | supported | Dispatch synthetic events. |
| `element.click()` | supported | Synthetic click. |
| `element.focus()` | supported | Focus element. |
| `element.blur()` | supported | Blur element. |
| `element.getBoundingClientRect()` | supported | Returns `x`, `y`, `width`, `height`, `top`, `right`, `bottom`, `left`. |
| `element.getClientRects()` | supported | Layout rect list. |
| `element.scrollIntoView(options?)` | supported | Verify scroll container behavior. |
| `element.remove()` | supported | Remove element from parent. |
| `element.cloneNode(deep?)` | supported | Verify complex clones. |

## Form and Editable Element APIs

| API | Status | Notes |
|---|---|---|
| `element.value` | supported | Get/set for input-like controls. |
| `element.defaultValue` | supported | Get/set. |
| `element.checked` | supported | Get/set for checkbox/radio-like controls. |
| `element.defaultChecked` | supported | Get/set. |
| `element.selected` | supported | Get/set for option-like controls. |
| `element.selectionStart` | supported | Get/set for text inputs. |
| `element.selectionEnd` | supported | Get/set for text inputs. |
| `element.setSelectionRange(start, end)` | supported | Text selection. |
| `element.select()` | supported | Select text. |
| `element.isContentEditable` | supported | Read-only getter. |
| `element.contentEditable` | supported | Get/set. |
| `element.oninput` | supported | Inline property exists; prefer Preact `onInput` or `addEventListener`. |
| `element.onchange` | supported | Inline property exists; prefer Preact `onChange` or `addEventListener`. |
| `element.onsubmit` | supported property | Do not rely on native browser form submission; handle submit in JS. |

## HTML, Image, Canvas, and Style APIs

| API | Status | Notes |
|---|---|---|
| `element.innerHTML` | supported | Get/set; prefer Preact rendering for app UI. |
| `element.outerHTML` | supported | Read-only getter. |
| `template.content` | supported | Template content getter exists. |
| `img.src` | supported | Get/set; verify resource paths. |
| `img.alt` | supported | Get/set. |
| `img.naturalWidth` | supported | Read-only getter. |
| `img.naturalHeight` | supported | Read-only getter. |
| `img.complete` | supported | Read-only getter. |
| `img.crossOrigin` | supported | Get/set; verify network/resource use. |
| `canvas.width` | supported | Get/set. |
| `canvas.height` | supported | Get/set. |
| `canvas.getContext('2d')` | supported subset | Verify any advanced Canvas API before relying on it. |
| `style.cssText` | supported | Get/set. |
| `style.length` | supported | Read-only getter. |
| `style.setProperty(name, value, priority?)` | supported | CSS property mutation. |
| `style.getPropertyValue(name)` | supported | CSS property lookup. |
| `style.removeProperty(name)` | supported | CSS property removal. |

## Event APIs

| API | Status | Notes |
|---|---|---|
| `new Event(type, options?)` | supported | Supports `bubbles` and `cancelable`. |
| `new CustomEvent(type, options?)` | supported | Supports `detail`, `bubbles`, and `cancelable`. |
| `event.type` | supported | Read-only getter. |
| `event.target` | supported | Read-only getter. |
| `event.currentTarget` | supported | Read-only getter. |
| `event.bubbles` | supported | Read-only getter. |
| `event.cancelable` | supported | Read-only getter. |
| `event.defaultPrevented` | supported | Read-only getter. |
| `event.timeStamp` | supported | Read-only getter. |
| `event.eventPhase` | supported | Read-only getter. |
| `event.stopPropagation()` | supported | Event propagation control. |
| `event.stopImmediatePropagation()` | supported | Event propagation control. |
| `event.preventDefault()` | supported | Default prevention. |
| `event.initEvent(type, bubbles, cancelable)` | supported | Legacy initializer. |
| `event.initCustomEvent(type, bubbles, cancelable, detail)` | supported | Legacy initializer for CustomEvent. |
| `event.clientX`, `event.clientY` | supported | Pointer/mouse event coordinates. |
| `event.screenX`, `event.screenY` | supported | Pointer/mouse event coordinates. |
| `event.button`, `event.buttons` | supported | Pointer/mouse button state. |
| `event.key`, `event.code` | supported | Keyboard event fields. |
| `event.keyCode`, `event.charCode` | supported | Legacy keyboard fields. |
| `event.repeat` | supported | Keyboard repeat field. |
| `event.ctrlKey`, `event.shiftKey`, `event.altKey`, `event.metaKey` | supported | Modifier state. |
| `event.inputType`, `event.data`, `event.isComposing` | supported | Input event fields. |
| `event.clipboardData.getData(type)` | supported | Clipboard event data only. |
| `event.clipboardData.setData(type, value)` | supported | Clipboard event data only. |
| `event.clipboardData.clearData()` | supported | Clipboard event data only. |
| `event.dataTransfer.getData(type)` | supported | Drag/drop data object exists; full file drag/drop is not a safe contract. |
| `event.dataTransfer.setData(type, value)` | supported | String data only. |
| `event.dataTransfer.clearData()` | supported | Clears transfer data. |

Supported event handler properties on elements include `onclick`, `ondblclick`, `onmousedown`, `onmouseup`, `onmousemove`, `onmouseenter`, `onmouseleave`, `oninput`, `onchange`, `onkeydown`, `onkeyup`, `onfocus`, `onblur`, `onsubmit`, `onload`, and `onerror`.

## MutationObserver

| API | Status | Notes |
|---|---|---|
| `new MutationObserver(callback)` | supported | Callback receives mutation records and observer. |
| `observer.observe(target, options)` | supported | Use basic `childList`, `attributes`, `characterData`, and subtree options. |
| `observer.disconnect()` | supported | Stop observing. |
| `observer.takeRecords()` | supported | Returns queued records. |

Mutation records expose `type`, `target`, `addedNodes`, `removedNodes`, `previousSibling`, `nextSibling`, `attributeName`, `attributeNamespace`, and `oldValue`.

## Selection and Range

| API | Status | Notes |
|---|---|---|
| `getSelection()` | supported | Also available as `window.getSelection()` and `document.getSelection()`. |
| `selection.anchorNode` | supported | Read-only getter. |
| `selection.focusNode` | supported | Read-only getter. |
| `selection.anchorOffset` | supported | Read-only getter. |
| `selection.focusOffset` | supported | Read-only getter. |
| `selection.isCollapsed` | supported | Read-only getter. |
| `selection.rangeCount` | supported | Read-only getter. |
| `selection.type` | supported | Read-only getter. |
| `selection.direction` | supported | Read-only getter. |
| `selection.setBaseAndExtent(anchorNode, anchorOffset, focusNode, focusOffset)` | supported | Selection mutation. |
| `selection.collapse(node, offset)` | supported | Selection mutation. |
| `selection.extend(node, offset)` | supported | Selection mutation. |
| `selection.selectAllChildren(node)` | supported | Selection mutation. |
| `selection.collapseToStart()` | supported | Selection mutation. |
| `selection.collapseToEnd()` | supported | Selection mutation. |
| `selection.removeAllRanges()` / `selection.empty()` | supported | Clear selection. |
| `selection.addRange(range)` | supported | Range selection. |
| `selection.removeRange(range)` | supported | Range removal. |
| `selection.getRangeAt(index)` | supported | Range lookup. |
| `selection.containsNode(node, allowPartial)` | supported | Containment check. |
| `selection.deleteFromDocument()` | supported | Deletes selection contents. |
| `selection.toString()` | supported | Selected text. |
| `range.setStart(node, offset)` | supported | Range mutation. |
| `range.setEnd(node, offset)` | supported | Range mutation. |
| `range.setStartBefore(node)` / `range.setStartAfter(node)` | supported | Range mutation. |
| `range.setEndBefore(node)` / `range.setEndAfter(node)` | supported | Range mutation. |
| `range.selectNode(node)` | supported | Range mutation. |
| `range.selectNodeContents(node)` | supported | Range mutation. |
| `range.collapse(toStart?)` | supported | Range mutation. |
| `range.cloneRange()` | supported | Returns a cloned range. |
| `range.toString()` | supported | Range text. |
| `range.getBoundingClientRect()` | supported | Range rect. |
| `range.getClientRects()` | supported | Range rect list. |

## Fetch and Response

| API | Status | Notes |
|---|---|---|
| `fetch(url, options?)` | supported | Supports network URLs, data URLs, and local/resource reads. Local/resource fetch supports only `GET` and `HEAD`. |
| `options.method` | supported | Defaults to `GET`. |
| `options.headers` | supported | Plain object headers. |
| `options.body` | supported | String body. |
| `Response.ok` | supported | Boolean. |
| `Response.status` | supported | Numeric status. |
| `Response.statusText` | supported | Status text. |
| `Response.headers` | supported | `Headers` object. |
| `Response.bodyUsed` | supported | Boolean body consumption state. |
| `Response.text()` | supported | Consumes body. |
| `Response.json()` | supported | Consumes body and parses JSON. |
| `Response.blob()` | supported | Exists, but verify `Blob` availability before relying on it. |
| `Response.arrayBuffer()` | supported | Consumes body and returns ArrayBuffer. |
| `Response.clone()` | supported | Only before body is consumed. |
| `new Headers(init?)` | supported | Plain object init. |
| `headers.get(name)` | supported | Case-insensitive. |
| `headers.set(name, value)` | supported | Case-insensitive. |
| `headers.has(name)` | supported | Case-insensitive. |
| `headers.delete(name)` | supported | Case-insensitive. |
| `headers.forEach(callback)` | supported | Iterates headers. |
| `headers.entries()` / `keys()` / `values()` | supported | Returns iterators. |

## MBlink Native Elements

Create these with Preact `h('terminal', props)` / `h('logview', props)` or `document.createElement('terminal')` / `document.createElement('logview')`. Give them stable IDs and explicit dimensions.

### `<terminal>`

| Method | Status | Notes |
|---|---|---|
| `terminal.write(data)` | supported | Append text or ANSI output. |
| `terminal.clear()` | supported | Clear terminal buffer. |
| `terminal.scrollTo(line)` | supported | Scroll to a buffer line. |
| `terminal.focus()` | supported | Focus terminal input. |
| `terminal.execute(command)` | supported | Run one-shot command. |
| `terminal.startShell(shell?)` | supported | Start interactive shell. |
| `terminal.sendInput(input)` | supported | Send input to running shell. |
| `terminal.resize(rows, cols)` | supported | Resize terminal rows/columns. |
| `terminal.serialize()` | supported | Return plain text buffer content. |

### `<logview>`

| Method | Status | Notes |
|---|---|---|
| `logview.append(level, source, message)` | supported | Levels should be `DEBUG`, `INFO`, `WARN`, `ERROR`, or `FATAL`. |
| `logview.clear()` | supported | Clear logs, search, filters, and selection. |
| `logview.scrollTo(line)` | supported | Scroll to displayed line. |
| `logview.search(query, useRegex?)` | supported | Returns match count. |
| `logview.clearSearch()` | supported | Clears search highlighting. |
| `logview.export(format?)` | supported | Use `text` or `json`; default is text. |
| `logview.setLevelFilter(levels)` | supported | Pass an array such as `['ERROR', 'WARN']`. |
| `logview.clearFilter()` | supported | Clears filters. |
| `logview.nextMatch()` | supported | Move to next search match. |
| `logview.prevMatch()` | supported | Move to previous search match. |

Do not assume JavaScript methods such as `setSourceFilter`, `scrollToTop`, `scrollToBottom`, `getMatchCount`, `getCurrentMatch`, `selectAll`, or display-option setters. See [native-elements.md](native-elements.md) for details.

## Host Bridge APIs

Use the generated `ui/bridge.js` and `hostApi`. Keep real host APIs and dev mocks aligned.

| API | Status | Notes |
|---|---|---|
| `hostApi.getTemplateInfo(defaults?)` | scaffolded baseline | Returns host/runtime/purpose/capability metadata. |
| `hostApi.incrementCounter(payload?)` | scaffolded baseline | Counter example; safe pattern for request/response host calls. |
| `hostApi.submitValidation(payload?)` | scaffolded baseline | Form submission example. |
| `hostApi.trayAction(payload?)` | scaffolded baseline | Host-only behavior; `tool` runtime returns dev mock data. |
| `globalThis.backend.<name>(payload)` | underlying bridge | Use through `hostApi`, not directly from visual components. |

## Host Observation and Devtools APIs

Use these host-side APIs when validating real C API hosts after the UI has passed the `mblink-ui-dev` mock/dev gate. Names are idiomatic per binding, but behavior should remain aligned with the C APIs in `core/api/mblink.h` and `core/devtools/mblink_devtools.h`.

| Capability | C API source | Python | Rust | Go | Notes |
|---|---|---|---|---|---|
| Runtime epoch | `mblink_runtime_epoch` | `App.runtime_epoch()` | `App::runtime_epoch()` | `App.RuntimeEpoch()` | Use to confirm the observed runtime matches the loaded UI. |
| Console observation | `mblink_observe_console_json` | `App.observe_console()` | `App::observe_console()` / `observe_console_json()` | `App.ObserveConsole()` / `ObserveConsoleJSON()` | Compare with `mblink-ui-dev logs` when checking parity. |
| Error observation | `mblink_observe_errors_json` | `App.observe_errors()` | `App::observe_errors()` / `observe_errors_json()` | `App.ObserveErrors()` / `ObserveErrorsJSON()` | Use after every host interaction path. |
| Lifecycle observation | `mblink_observe_lifecycle_json` | `App.observe_lifecycle()` | `App::observe_lifecycle()` / `observe_lifecycle_json()` | `App.ObserveLifecycle()` / `ObserveLifecycleJSON()` | Useful for close/load/runtime-state parity. |
| Deterministic render flush | `mblink_render_frame` | C API only unless wrapped | `App::render_frame(passes)` | `App.RenderFrame(passes)` | Use before snapshot when deterministic observation matters. |
| UI-dev snapshot JSON/file | `mblink_ui_dev_snapshot_json` / `mblink_ui_dev_snapshot_file` | `App.ui_dev_snapshot(...)` / `ui_dev_snapshot_file(...)` | `App::ui_dev_snapshot(...)` / `ui_dev_snapshot_file(...)` | `App.UiDevSnapshot(...)` / `UiDevSnapshotFile(...)` | Requires optional `mblink_devtools.dll`; prefer this over OS screenshots. |
| UI-dev command | `mblink_ui_dev_command_json` | `App.ui_dev_command(...)` | `App::ui_dev_command(...)` | `App.UiDevCommand(...)` | Supports live `query_element`, `inspect`, `click`, `input_text`, `scroll`, and `highlight` command semantics. |
| Runtime HTTP MCP | `mblink_devtools_http_start` / `mblink_devtools_http_stop` | `App.devtools_http_session(...)` | `App::devtools_http_session(...)` | `App.DevtoolsHttpSession(...)` | Local-only development endpoint for live UI analysis/control in an already-running host. |

`mblink_devtools.dll` is optional and development-only. Binding packages should not vendor it; load it on demand from `MBLINK_DEVTOOLS_PATH`, the adjacent `mblink.dll` directory, or the process/runtime search path.

## Explicitly Unsupported or Not a Current Contract

Do not use these in MBlink UI code unless the project adds a verified adapter and the change passes incremental runtime validation.

| API or Pattern | Status | Reason |
|---|---|---|
| `require`, `module.exports` | unsupported | UI code is ESM, not CommonJS. |
| Node built-ins such as `fs`, `path`, `process`, `Buffer` | unsupported | UI runtime is not Node.js. |
| `localStorage`, `sessionStorage`, `indexedDB` | unsupported contract | Not part of verified runtime surface. |
| cookies and cache APIs | unsupported contract | Not part of verified runtime surface. |
| WebSocket, WebRTC | unsupported contract | Not part of verified runtime surface. |
| Web Workers, Service Workers, BroadcastChannel | unsupported contract | Not part of verified runtime surface. |
| `navigator.clipboard` | unsupported contract | Clipboard event data may exist; navigator clipboard API is not a usable contract. |
| `File`, `FileReader`, full drag-and-drop file workflows, `URL.createObjectURL` | unsupported contract | Native file workflows must be host-backed and verified. |
| full browser navigation, history, downloads, `target="_blank"` | unsupported contract | MBlink UI is a desktop runtime, not a browser. |
| native form submission | unsupported contract | Handle form submission in JS and through `hostApi`. |
| Shadow DOM, custom elements | unsupported contract | MBlink native tags are not Web Components. |
| `ResizeObserver`, `IntersectionObserver` | unsupported contract | Not in the verified API list. |
| WebGL, WebGPU, audio/video media APIs | unsupported contract | Not in the verified API list. |
| `preact-lite.js`, `globalThis.Preact`, `globalThis.PreactHooks`, legacy `js/preact/*` | unsupported pattern | Use official embedded Preact ESM modules. |

## Verification Rule for New APIs

When an API is not listed as supported:

1. Add the smallest possible isolated component or helper that uses it.
2. Build with `mblink-ui-dev build`.
3. Open or reload the runtime.
4. Confirm `snapshot` shows real UI nodes.
5. Query and inspect the component by stable selector.
6. Exercise the interaction path if relevant.
7. Check `logs` and `errors`.
8. Only then expand usage across the app.
