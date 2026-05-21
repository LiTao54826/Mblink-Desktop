---
name: mbink-ui-dev
description: Operate the MBink UI development toolchain through `mbink-ui-dev` CLI or MCP to initialize projects, open runtimes, inspect live snapshots, query and inspect UI elements, click or input or scroll or highlight the live UI, read or write project files, build, watch, reload the runtime, diagnose console or JS errors, and read MCP resources for MBink UI automation. Use when Codex needs to create, debug, test, or iterate on an MBink UI project, when a user mentions `mbink-ui-dev`, MBink UI snapshots, daemon or watch workflows, selector-based UI diagnosis, or when Claude Desktop or Cursor needs MCP access to an MBink UI project.
---

# MBink UI Dev

## Overview

Use `mbink-ui-dev` as the primary control surface for MBink UI development. Treat the latest progress described in `MBINK_UI_DEV_TOOL_DESIGN.md` as authoritative: P2 is complete, the P3 precision-control loop is now available, and ecosystem items such as HTTP+SSE transport, MCP notifications, and VS Code preview are still future work.

## Layout and Sizing Defaults

- Treat all width and height values as logical pixels, including app window size, viewport size, element rects, spacing, and default template dimensions. Do not design against physical pixels or inflate layouts for high-DPI displays.
- MBink UI is primarily a desktop-app framework. Make the outermost `body` or root shell fill the full window by default, keep `body` non-scrollable unless page-level scrolling is genuinely required, and place scrollbars inside the content regions that need them.

## Choose the Control Surface

- Use the CLI when the agent has shell access and wants direct JSON responses.
- Use MCP when the agent benefits from structured tools, resources, session-scoped `active_project`, or IDE integration.
- Treat daemon state as project-scoped. Open the project first, then keep all observation, edit, build, and interaction steps routed through that same project.
- In multi-project situations, pass `--project <abs-path>` or call `open_project` explicitly instead of guessing.

## Follow the Standard Development Loop

1. Open the project through `mbink-ui-dev open` or MCP `open_project`.
2. Read project state through `info` or `get_project_info`.
3. Observe the live UI through `snapshot` or `snapshot_ui`.
4. Locate the exact target through `query_element` instead of guessing selectors.
5. Diagnose the target through `inspect` when layout, style, or component state matters.
6. Read or write source files through `read` and `write` or `read_file` and `write_file`.
7. Build through `build`, usually with watch enabled during tight iteration loops.
8. Reload through `reload` after builds. Current documented behavior is fixed `restart_runtime`, even though the design keeps `css` and `remount` and `restart` as future semantics.
9. Use `click`, `input_text`, `scroll`, and `highlight` to validate interactions or spotlight the exact element under review.
10. Re-run `snapshot_ui`, `query_element`, `get_console_logs`, and `get_js_errors` to verify the effect.

## Build UI First, Then Integrate Host Code

- Start new UI work in the `tool` runtime or through the shared `ui/bridge.js` mock layer before editing Python, Rust, or Go host code.
- Build the UI with realistic mock data first. Finish layout, state, loading, empty, error, and interaction behavior in `mbink-ui-dev` before wiring real host behavior.
- Prefer componentized UI development for anything beyond a very small screen. Split reusable shell, panels, forms, lists, dialogs, and native-element wrappers into local Preact components before host integration.
- Treat automated UI validation as a required gate. Run `build`, reload or reopen the runtime, take a `snapshot`, target important elements with `query_element`, inspect layout-sensitive nodes with `inspect`, exercise representative `click`, `input_text`, and `scroll` paths, then check `logs` and `errors`.
- Preserve this order even during incremental work: design and verify the UI with mock data first, then implement and connect the real host-language logic only after the UI slice passes its validation gate.
- Do not start or modify host-language implementation for a feature while the mock UI is blank, failing to build, missing important selectors, or producing runtime JS errors.
- After the UI validation gate passes, implement the narrow host API surface behind `ui/bridge.js`, then validate the real Python, Rust, or Go host runtime and finish with `mbink-ui-dev build`.
- Keep mock and host API names aligned so the same UI test path can run before and after host integration.

## Prefer Async Host Calls

- Treat JavaScript-to-host calls as asynchronous by default. UI code should call host behavior through `hostApi` methods that return promises and are safe to `await`.
- Use Python `@app.bind_async`, Rust `App::bind_async`, or Go `App.BindAsync` for host work that might block, including filesystem access, network calls, process execution, sleeps, database work, native control bulk updates, CPU-heavy processing, or any operation with user-visible latency.
- Use synchronous `bind` only for tiny, deterministic operations that return immediately and cannot block the UI, such as simple in-memory reads, validation of already-loaded data, or trivial window state helpers.
- Keep the async boundary behind `ui/bridge.js`: visual components should call `hostApi.someAction(payload)` and handle loading, success, empty, and error states instead of calling `globalThis.backend` directly.
- In the `tool` runtime, keep mocks promise-compatible even when they return immediately, so the same UI path works unchanged after wiring the real Python, Rust, or Go host runtime.
- If an existing host call starts doing heavier work, upgrade it from sync binding to async binding before adding UI around it.

## Develop Incrementally

- Add UI in small verified slices instead of writing the whole screen or a large component in one pass.
- Build one component, state branch, or interaction path at a time, then run the relevant `build`, reload or reopen, `snapshot`, `query_element`, `inspect`, interaction, `logs`, and `errors` checks before adding the next slice.
- For each UI slice, follow this verification order: write code, build and reload, capture a live `snapshot`, confirm the rendered structure and key rects are correct, capture a screenshot when screenshot output is available, compare the visual result against the intended CSS, then test interaction behavior if that slice has any interactions.
- During the visual check, verify layout fit, text size, text alignment, colors, element positions, element sizes, spacing, and state colors against the CSS intent. Do not continue to the next slice while the snapshot structure or screenshot appearance contradicts the expected CSS result.
- Treat text overflow as a validation failure in compact controls: buttons, tabs, badges, and similar elements must keep their labels inside their bounds, and any longer copy must have an explicit wrap, truncate, or resize strategy before approval.
- When a visual issue clearly contradicts valid CSS or expected MBink behavior, diagnose and fix the MBink framework first instead of hiding the problem with project-specific CSS workarounds. Temporary UI-side workarounds are acceptable only to isolate the failure, and should be removed after the framework fix lands.
- When a slice introduces a framework compatibility issue, blank UI, selector failure, layout regression, or JS error, stop at that slice and fix it before continuing.
- Introduce risky syntax, browser APIs, CSS features, native elements, third-party dependencies, or host bridge calls in the smallest isolated component that can prove compatibility.
- Keep each validation target stable with IDs or data attributes so the failure can be traced to the last added slice instead of a large unverified rewrite.

## Require Snapshot Evidence Before Completion

- Do not claim a UI task is complete from code review, build success, clean logs, or subjective judgment alone.
- Before reporting completion, run `snapshot` or `snapshot_ui` against the live runtime and confirm the tree contains real rendered UI nodes, expected key text or controls, nonzero rects for important regions, and no placeholder-only or empty root.
- Use `query_element` for key selectors after the snapshot so the exact controls that matter to the user are proven present and targetable.
- For visual or layout-sensitive work, also use `inspect` on the relevant nodes and summarize the verified geometry, visibility, or style facts.
- If `snapshot` cannot be run, fails, returns an empty tree, or does not show the expected UI, report the work as not fully verified and continue debugging instead of saying it is done.
- In the final response for UI work, include a short verification note naming the snapshot or query checks that passed. Build output alone is not enough evidence.

## Prefer These Tool Priorities

- Call `snapshot_ui` before acting and before completion so the UI state is grounded in real rendered data.
- Use `query_element` to locate and `inspect` to diagnose. Do not debug by selector guesswork alone.
- Prefer dedicated UI control tools over `eval_js` for locate, inspect, click, input, scroll, and highlight actions.
- Prefer source edits plus build plus reload over `eval_js` patches.
- Treat `eval_js` as an escape hatch for one-off checks, temporary instrumentation, or state probes that do not deserve a source edit.
- After interactions, re-check with `snapshot_ui` or `query_element`. The documented P3 path includes post-interaction resampling and snapshot timing fixes, so immediate verification is expected to work.
- Do not reload a failed build. Fix build errors first.
- Use logs and JS errors as supporting evidence, not as a replacement for snapshots.

## Author Within Current MBink Limits

- Treat MBink as a QuickJS plus MBink DOM desktop runtime, not as Chromium, Firefox, Safari, Node.js, or a complete Web platform.
- Write UI code as ESM. Prefer official `preact`, `preact/hooks`, `preact/jsx-runtime`, and `preact/jsx-dev-runtime` imports. Do not use `preact-lite.js`, `globalThis.Preact`, `globalThis.PreactHooks`, or legacy custom `js/preact/*` APIs.
- Prefer the template pattern: `import { h, render } from 'preact'` and author components with `h(...)`. If using JSX in `.js`, `.mjs`, `.jsx`, `.ts`, or `.tsx`, keep `h` and `Fragment` in scope unless `mbink.config.json` intentionally changes the JSX settings and the build is verified.
- Structure non-trivial UI as small Preact components with explicit props and stable selectors. Keep host calls in `ui/bridge.js` or thin adapter helpers instead of scattering host behavior across visual components.
- Do not rely on TypeScript type checking from `mbink-ui-dev build`; current TypeScript and TSX handling is esbuild transpilation only.
- Use `ui/bridge.js` and `hostApi` for Python, Rust, and Go host interaction. The `tool` runtime provides dev mocks only; tray, native window actions, and host resource behavior must not be treated as real in `tool` projects.
- Use MBink's native `<terminal>` and `<logview>` elements for terminal emulation and high-volume log display instead of browser or npm terminal/log widgets. Read [native-elements.md](references/native-elements.md) before using either element.
- When unsure whether a JavaScript, DOM, Preact, host bridge, or native-element API is compatible, read [supported-api-reference.md](references/supported-api-reference.md) before using it.
- Avoid browser or Node APIs that are not part of the verified runtime surface: `require`, `module.exports`, Node built-ins such as `fs` and `path`, `process`, `Buffer`, `localStorage`, `sessionStorage`, `indexedDB`, workers, WebSocket, full navigation/history/download behavior, native form submission, and `navigator.clipboard`.
- Treat `reload --mode`, HTTP plus SSE transport, MCP notifications, and VS Code preview as unavailable unless the reference says they have landed. `snapshot` supports opt-in screenshot output through `include_screenshot`; prefer file-mode PNG metadata over inline base64 unless the caller explicitly needs inline data.
- Read [compatibility-guidelines.md](references/compatibility-guidelines.md) before adding new framework dependencies, browser APIs, runtime-specific host features, or non-template syntax.

## Bootstrap Projects and Templates Deliberately

- Use `init` or `init_project` to scaffold new projects.
- CLI `mbink-ui-dev init` now accepts an optional target directory. `mbink-ui-dev init` scaffolds into the current directory, while `mbink-ui-dev init my-app` scaffolds into `my-app`.
- Choose a canonical purpose (`minimal`, `showcase`, `desktop-app`) and runtime (`tool`, `python`, `rust`, `go`). Bare `init` defaults to `minimal/tool`; legacy `--template` names are compatibility-only.
- Open the new project immediately after init and verify the initial runtime with a snapshot.
- Templates are embedded into `mbink-ui-dev.exe` at build time, so distribution does not depend on shipping a separate `tools/mbink_ui_dev/templates` directory.
- Distribute `mbink-ui-dev.exe` with the adjacent MBink runtime library (`mbink.dll` on Windows); host-runtime init copies that one runtime library into the generated Python, Go, or Rust project.
- Runtime templates vendor their MBink binding files, and init copies the runtime library, so generated Python, Go, and Rust projects are self-contained and do not depend on `MBINK_REPO_ROOT`.
- Use `mbink-ui-dev build` as the release path. It compiles the UI into `app.mbrp` and then builds the selected host artifact in one command.
- Runtime starters share the canonical UI under `ui/app.js`, add `ui/bridge.js` mock fallback, and generate runnable host adapters such as `host/main.py`, `host/main.go`, or `rust_host/src/main.rs` when a host runtime is selected.
- Read [cli-mcp-reference.md](references/cli-mcp-reference.md) when template choice, config layout, or generated structure matters.

## Use File and Interaction Operations Intentionally

- Prefer `write` with stdin or `--from` for substantial file content.
- Use `write_file` for project-relative MCP edits that should trigger the normal dev loop.
- Use `click`, `input_text`, and `scroll` to validate UI behavior after edits instead of assuming the component now works.
- Use `highlight` when the user or another agent needs visual disambiguation of the selected element.

## Use MCP Resources and Ecosystem Integrations

- Use `resources/read` for stable, read-only project state such as `ui://snapshot`, `ui://console_logs`, `ui://build_status`, `project://file_tree`, `project://config`, and `project://file/{path}`.
- Prefer session-scoped `active_project` in multi-step MCP work so later tool calls and resource reads stay aligned.
- Use stdio MCP by default. Treat HTTP plus SSE transport, MCP notifications, additional ecosystem integrations, and VS Code side-panel preview as future-stage items, not current assumptions.
- When current behavior and long-term design differ, prefer the document's explicit "当前实现" notes over the future-facing design direction.

## Use This Reference When Needed

- Read [cli-mcp-reference.md](references/cli-mcp-reference.md) for exact CLI commands, MCP tool mapping, resource URIs, project resolution rules, templates, and current versus future capability boundaries.
