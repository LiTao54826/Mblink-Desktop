# MBink UI Compatibility Guidelines

Use this reference when writing MBink UI code, choosing dependencies, changing scaffold templates, or diagnosing code that works in a browser but fails or renders blank in MBink.

## Runtime Model

- MBink UI runs on QuickJS plus MBink DOM, layout, render, networking, and host bindings. It is not Chromium, WebView2, Node.js, or a full browser.
- `mbink.dll` is the runtime core. `mbink-ui-dev`, `esm_loader`, Python, Rust, and Go should share observable behavior through `core/api/mbink.h`; development-only snapshot/control and HTTP MCP behavior should flow through the optional `core/devtools/mbink_devtools.h` plugin API.
- Use ESM imports. CommonJS (`require`, `module.exports`) and Node built-ins (`fs`, `path`, `process`, `Buffer`, etc.) are not a supported UI runtime contract.
- Prefer deterministic local UI code plus explicit host calls through `hostApi`. Do not hide business behavior behind implicit browser globals.
- Verify behavior with `mbink-ui-dev open`, `build`, `snapshot`, `query_element`, `inspect`, `logs`, and `errors`. Do not assume browser-tested code is compatible.
- When unsure whether a specific API exists or is safe, read [supported-api-reference.md](supported-api-reference.md) before using it.

## UI-First Workflow Gate

- Build the UI first with `mbink-ui-dev`, shared `ui/bridge.js`, and realistic mock data. Do not begin Python, Rust, or Go feature implementation until the UI has passed dev validation.
- Cover normal, loading, empty, error, and representative interaction states with mock data before host integration.
- The required validation path is: `build`, open or reload, `snapshot`, `query_element` for stable controls, `inspect` for layout-sensitive nodes, representative `click`, `input_text`, and `scroll`, then `logs` and `errors`.
- Treat a blank UI, placeholder-only snapshot, failed selector query, failed interaction, build error, or unexplained JS error as a blocker for host-language work.
- After the UI gate passes, wire the real host through the same `hostApi` names used by the mock bridge, validate the host runtime, then use `mbink-ui-dev build` for the final artifact.
- Do not treat a passing `tool` runtime as proof for Python, Rust, Go, tray/native controls, resources, or packaged host behavior. Re-run the same important snapshot/query/interaction checks in the real host using UI-dev snapshot/control helpers or the runtime HTTP MCP endpoint.

## C API Parity Rules

- If a feature must behave the same in `esm_loader`, Python, Rust, Go, or future bindings, implement the runtime capability in the shared C API or in the optional devtools plugin C API first.
- Do not add binding-only shortcuts for parity-critical behavior. Wrapper names can be idiomatic, but JSON fields, snapshot shape, command semantics, lifecycle/console/error observation, and interaction effects should remain aligned.
- Use `mbink_devtools.dll` as a development companion only. It may sit next to `mbink-ui-dev.exe`, `esm_loader.exe`, and `mbink.dll`; generated host projects and binding packages should not vendor it.
- Bindings should load `mbink_devtools.dll` only when UI-dev snapshot/control, DevTools panel, or HTTP MCP features are used. Resolution should prefer explicit path or `MBINK_DEVTOOLS_PATH`, then the runtime DLL directory or process search path.
- When a parity issue appears, compare `mbink-ui-dev` snapshot/control output, `esm_loader` UI-dev files, and the target binding's UI-dev helper before changing app CSS or host-specific wrapper code.

## Snapshot Completion Gate

- A UI task is not complete until a live `snapshot` or `snapshot_ui` proves the rendered tree contains the intended UI.
- Do not accept source code review, successful build output, clean logs, or "looks correct by reasoning" as completion evidence without a snapshot.
- The final snapshot must show real rendered nodes, expected key text or controls, nonzero rectangles for important regions, and no empty root, `stub-root`, placeholder-only shell, or hidden-only content.
- After the snapshot, use `query_element` for the key selectors the user cares about. For layout-sensitive work, use `inspect` on those nodes and verify visibility, geometry, and relevant style facts.
- If snapshot evidence is missing, empty, stale, or inconsistent with the expected UI, report the task as not fully verified and continue diagnosing instead of claiming completion.
- Final responses for UI work must mention the snapshot or query checks that passed. Build output alone is never enough.

## Incremental Development Rules

- Build UI progressively. Add one component, visual section, data state, or interaction path, validate it, and only then add the next slice.
- Do not write a full complex screen or large component before the first runtime check. In MBink, delayed validation makes framework, DOM, CSS, and runtime compatibility issues much harder to isolate.
- Stop immediately when the last slice causes a blank UI, missing node, failed interaction, layout breakage, build error, or JS error. Fix that slice before continuing.
- Introduce new syntax, dependencies, native elements, browser APIs, CSS features, and host bridge calls one at a time so compatibility failures have a narrow cause.
- Prefer simple placeholder data and stable selectors for early slices, then expand to realistic mock data after the component renders and validates.

## Preact Rules

- Use official embedded Preact modules:
  - `preact`
  - `preact/hooks`
  - `preact/jsx-runtime`
  - `preact/jsx-dev-runtime`
- Do not use `preact-lite.js`, custom `js/preact/*`, `globalThis.Preact`, or `globalThis.PreactHooks` for runtime behavior.
- The scaffold templates intentionally use:

```js
import { h, render } from 'preact';
import { useEffect, useMemo, useState } from 'preact/hooks';
```

- The default build uses classic JSX settings: `jsx_factory = "h"` and `jsx_fragment = "Fragment"`. If authoring JSX, keep those identifiers in scope or explicitly change and verify `mbink.config.json`.
- Prefer `h(...)` for generated template code because it avoids JSX transform ambiguity and keeps `.js` entries runnable through the current toolchain.
- Do not assume `preact/compat`, `preact/debug`, React, React DOM, React Router, or browser-devtools integration is available unless the project explicitly vendors and verifies it.

## Componentization Rules

- Prefer componentized UI development unless the project is truly a tiny single-screen utility.
- Split non-trivial UIs into local Preact components for the app shell, navigation, panels, forms, list rows, dialogs, status blocks, and native wrappers such as terminal or log views.
- Keep component boundaries UI-oriented. Put host access behind `ui/bridge.js` or thin adapter helpers, then pass data and callbacks into components through explicit props.
- Keep mock data close to the bridge or a dedicated mock module so the same component tree can run in `tool`, Python, Rust, and Go runtimes.
- Add stable IDs or data attributes on important component roots and controls so `query_element`, `inspect`, `click`, and `input_text` can validate them reliably.

## Build and Syntax Boundaries

- `mbink-ui-dev build` is the release path. It compiles the UI to `.dist/App.js`, packages `.dist/app.mbrp`, and then builds the selected host artifact.
- Current UI bundling is esbuild-based and emits ESM. `.js` and `.mjs` are treated with the JSX loader, and `.jsx`, `.ts`, and `.tsx` are also accepted as transpiled input.
- TypeScript is transpiled only. Do not rely on `mbink-ui-dev build` for type checking, declaration generation, decorators, custom transformers, or path aliases that are not already handled by esbuild and the project configuration.
- Avoid dynamic imports, import assertions, CSS imports, JSON imports, asset imports, and package-manager-specific module resolution unless verified in the generated project.
- Keep imports explicit and local where possible. Bare imports are safest for the embedded official Preact modules; other third-party packages must be bundled and runtime-tested.

## Browser API Boundaries

The runtime includes a practical subset of browser APIs for desktop UI. Treat these as limited, not browser-complete.

Prefer:
- `document.createElement`, `createElementNS`, `createTextNode`, `createComment`
- `querySelector`, `querySelectorAll`, `getElementById`, `matches`, `closest`
- `append`, `prepend`, `remove`, `appendChild`, `removeChild`, `classList`
- `addEventListener`, `removeEventListener`, `dispatchEvent`, `Event`, `CustomEvent`
- `MutationObserver` for basic DOM mutation observation
- `setTimeout`, `setInterval`, `queueMicrotask`, `requestAnimationFrame`, `performance.now`
- `fetch` for verified HTTP/resource cases only

Avoid or gate behind verification:
- `localStorage`, `sessionStorage`, `indexedDB`, cookies, cache APIs
- WebSocket, WebRTC, workers, service workers, broadcast channels
- native file APIs such as `File`, `FileReader`, drag-and-drop file workflows, and `URL.createObjectURL`
- full browser navigation, `target="_blank"`, download behavior, history semantics, and automatic form submission
- `navigator.clipboard`; clipboard event data may exist, but the navigator clipboard API is not a usable contract
- `ResizeObserver`, `IntersectionObserver`, Shadow DOM, custom elements, full accessibility tree APIs
- WebGL, WebGPU, audio/video media APIs, and advanced canvas image-pattern behavior unless specifically verified

## MBink Native Elements

Use [native-elements.md](native-elements.md) when a UI needs terminal output, an interactive shell, command execution, or high-volume logs. MBink provides two proprietary native-rendered tags:

- `<terminal>` for ANSI terminal emulation, command output, and interactive shell sessions.
- `<logview>` for efficient structured log display, filtering, search, and export.

Do not replace these with browser or npm widgets such as xterm.js-style terminal canvases or virtualized log viewers unless the project has a verified reason. These tags are MBink runtime elements, not standard HTML and not Web Components; their supported JavaScript methods are the methods documented in the native-elements reference.

## CSS and Layout Rules

- Prefer normal block, flex, grid, inline, positioning, transforms, clipping, and stacking behavior that has been validated by snapshot and visual checks.
- Avoid browser-specific layout assumptions and advanced CSS features unless verified in MBink: container queries, complex filters/backdrop filters, advanced animations, sticky edge cases, custom scrollbars, complex writing modes, and print/media-query workflows.
- Keep UI dimensions explicit enough for snapshot inspection. Use stable IDs on important controls so `query_element`, `inspect`, `click`, and `input_text` can target them reliably.
- Treat text overflow as a layout defect in compact controls: buttons, tabs, badges, and similar elements must keep labels inside their bounds, and any longer copy must have an explicit wrap, truncate, or resize strategy before approval.
- For frameless desktop templates, treat CSS window-control and drag-region properties as host-specific integration points and verify them in a host runtime, not just `tool`.

## Host Runtime Boundaries

- `tool` is frontend plus dev mock bridge. It does not provide real tray integration, real native host data, or final desktop packaging.
- `python`, `rust`, and `go` templates provide host adapters and receive a copied MBink runtime library during init on Windows.
- Keep the UI-to-host contract narrow and named through `ui/bridge.js`: `getTemplateInfo`, `incrementCounter`, `submitValidation`, and `trayAction` are the scaffolded baseline.
- After host wiring, validate through the host runtime itself. Prefer `App.ui_dev_snapshot`, `App::ui_dev_snapshot`, `App.UiDevSnapshot`, or the language's `devtools_http_session` helper so evidence comes from the shared C API/devtools path.
- Prefer `mbink-ui-dev build` over manually running `cargo`, `go build`, or `PyInstaller` for release artifacts. Manual host commands are only for targeted binding validation.
- Final Windows output differs by runtime:
  - Rust and Go produce an exe plus `mbink.dll`.
  - Python uses PyInstaller one-file output and embeds the MBink DLL plus resources.
  - Tool runtime produces the UI resource package and is not a standalone host app.

## Resource Package Rules

- Treat `app.mbrp` as the shared compiled UI resource package.
- Host templates should prefer loading and mounting `app.mbrp`; direct filesystem loading is for dev fallback only.
- Rust and Go host builds copy `app.mbrp` into a host resources directory before compiling. Rust can embed the package through the generated build script path.
- Do not hard-code absolute repository paths, local machine paths, or `MBINK_REPO_ROOT` in generated projects.
- Do not duplicate `mbink.dll` inside each embedded template. The CLI distribution should include one adjacent runtime DLL, and host init copies it into the generated project. `mbink_devtools.dll` is a development companion shipped next to `mbink-ui-dev.exe`, `esm_loader.exe`, and `mbink.dll`; generated Python, Go, and Rust host packages should not vendor it.

## CLI and MCP Feature Boundaries

- `reload` currently restarts the runtime. Do not pass or assume `css`, `remount`, or `restart` modes.
- `snapshot_ui` returns structured DOM snapshot data. `include_screenshot` is available as an opt-in PNG capture path; default snapshots remain DOM-only. Prefer file-mode screenshot metadata over inline base64. `max_depth` and `root_selector` are supported snapshot-bounding options for callers that need smaller DOM payloads.
- CLI stdout is JSON and stderr is human-readable logs.
- `mbink-ui-dev serve` uses stdio MCP for project-level work. The optional `mbink_devtools.dll` runtime HTTP MCP endpoint is local-only and limited to live UI analysis/control for an already-running C API host. It does not provide init, build, watch, files, logs, errors, daemon state, or project resources. Push notifications and VS Code side-panel preview are future work.
- If multiple projects are present, always pass `--project <abs-path>` or call `open_project` for the intended project.

## Compatibility Checklist Before Finishing

1. Build with `mbink-ui-dev build`.
2. Open the project and confirm `snapshot` returns real UI nodes, not an empty tree or placeholder root.
3. Query important controls by stable selectors.
4. Exercise at least one click/input path for interactive work.
5. Check `logs` and `errors`.
6. For host runtimes, verify the real host with UI-dev snapshot/control helpers or runtime HTTP MCP, then verify the final artifact path and copied or embedded resource package.
