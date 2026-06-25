# MBlink UI Dev Reference

Read this file when exact command names, MCP mappings, templates, routing rules, reload semantics, C API host parity, or ecosystem integrations matter. Current project work is split between `mblink-ui-dev` for project development, `mblink.dll` for runtime behavior, and optional `mblink_devtools.dll` for development-only live UI analysis/control.

## Output Rules

- Treat CLI stdout as machine-readable JSON.
- Treat CLI stderr as human-readable logs.
- Treat exit code `0` as success.
- Prefer parsing tool output instead of scraping stderr text.

## Architecture and Distribution Boundaries

- `mblink-ui-dev` is the primary development surface for scaffold/init, project config, daemon lifecycle, build/watch, file IO, logs/errors, snapshots, precision UI control, and stdio MCP.
- `mblink.dll` is the runtime core. Runtime behavior that must work in `esm_loader`, Python, Rust, Go, or future bindings belongs in the public C API in `core/api/mblink.h`.
- `mblink_devtools.dll` is an optional development plugin. It provides DevTools panel integration, UI-dev snapshot/control C APIs, and local Streamable HTTP MCP for already-running C API hosts.
- Development distributions should place `mblink_devtools.dll` next to `mblink-ui-dev.exe`, `esm_loader.exe`, and `mblink.dll`. Generated host projects and binding packages should copy/vendor `mblink.dll` as needed, but should not vendor `mblink_devtools.dll`.
- Bindings should load `mblink_devtools.dll` on demand from `MBLINK_DEVTOOLS_PATH`, the loaded `mblink.dll` directory, or the process/runtime search path.
- Do not use the frontend-only `tool` runtime as final evidence for Python, Rust, Go, tray, native controls, or resource-package behavior. Use `tool` to shape the UI quickly, then verify through the real host binding.

## CLI Surface

```bash
# Project initialization
mblink-ui-dev init [path] [--purpose <minimal|showcase|desktop-app> --runtime <tool|python|rust|go>] [--template <legacy-name>]

# Daemon management
mblink-ui-dev daemon start [--project <path>]
mblink-ui-dev daemon run [--project <path>]
mblink-ui-dev daemon stop [--project <path>]
mblink-ui-dev daemon status [--project <path>]
mblink-ui-dev stop [--project <path>]

# MCP adapter
mblink-ui-dev serve [--project <path>]

# Project and runtime control
mblink-ui-dev open [<project-path>]
mblink-ui-dev info [--project <path>]
mblink-ui-dev reload [--project <path>]
mblink-ui-dev eval "<js code>" [--project <path>]

# File operations
mblink-ui-dev read <path> [--encoding utf8|base64] [--project <path>]
mblink-ui-dev write <path> [--content <text> | --from <file>] [--project <path>]

# Build and observation
mblink-ui-dev build [--watch] [--project <path>]
mblink-ui-dev build-status [--project <path>]
mblink-ui-dev snapshot [--project <path>]
mblink-ui-dev snapshot [--response auto|inline|file] [--project <path>]
mblink-ui-dev snapshot --include-screenshot [--inline-screenshot] [--response auto|inline|file] [--project <path>]
mblink-ui-dev logs [--project <path>]
mblink-ui-dev errors [--project <path>]

# Precision control and inspection
mblink-ui-dev query ...
mblink-ui-dev query-element ...
mblink-ui-dev inspect ...
mblink-ui-dev click ...
mblink-ui-dev input-text ...
mblink-ui-dev scroll ...
mblink-ui-dev highlight ...
```

For precision-control work, prefer MCP tool schemas when exact argument structure matters.

## `esm_loader` Development Hooks

Use `esm_loader` when verifying the thin C API runtime path directly. The development hooks are opt-in and require the adjacent or resolvable `mblink_devtools.dll`.

```bash
esm_loader app.js --ui-dev-snapshot-file snapshot.json
esm_loader app.js --ui-dev-snapshot-file snapshot.json --ui-dev-snapshot-include-screenshot --ui-dev-screenshot-file snapshot.png
esm_loader app.js --ui-dev-command-file command.json --ui-dev-response-file response.json
esm_loader app.js --ui-dev-console-file console.json --ui-dev-errors-file errors.json --ui-dev-lifecycle-file lifecycle.json
esm_loader app.js --devtools-http-mcp --devtools-http-port 0
```

Useful snapshot options include `--ui-dev-runtime-epoch`, `--ui-dev-snapshot-max-nodes`, `--ui-dev-snapshot-max-depth`, `--ui-dev-snapshot-root-selector`, `--ui-dev-snapshot-include-screenshot`, `--ui-dev-snapshot-inline-screenshot`, and `--ui-dev-screenshot-file`.

## Recommended CLI Sequences

### UI-First Then Host Integration

Build and prove the UI before writing host-language code:

```bash
mblink-ui-dev init /abs/path/my-app --purpose desktop-app --runtime tool
mblink-ui-dev open /abs/path/my-app
mblink-ui-dev build
mblink-ui-dev snapshot
mblink-ui-dev query-element ...
mblink-ui-dev inspect ...
mblink-ui-dev click ...
mblink-ui-dev input-text ...
mblink-ui-dev scroll ...
mblink-ui-dev logs
mblink-ui-dev errors
```

Use `ui/bridge.js` mock data for this stage. The UI stage is not complete until build succeeds, the snapshot contains real UI nodes, important controls can be queried by stable selectors, representative interactions work, and logs/errors are clean enough to explain.

Develop this sequence incrementally. Add one component, section, mock state, or interaction path, then run the relevant build, snapshot, query, inspect, interaction, log, and error checks before adding the next part. If the last addition causes a blank UI, missing selector, layout issue, build error, or JS error, fix that addition before continuing.

After that gate passes, add or modify the real Python, Rust, or Go host adapter behind the same `hostApi` names, then run the host integration and final release build:

```bash
mblink-ui-dev build --project /abs/path/my-app
```

Manual host commands are only for targeted binding checks after UI validation has passed. When validating a host runtime, use the binding's UI-dev snapshot/control helpers or `devtools_http_session` so the evidence comes from the same C API/devtools path that `mblink-ui-dev` uses.

### C API Host Parity Check

Use this sequence when a change touches `esm_loader`, C API runtime behavior, `mblink_devtools.dll`, or Python/Rust/Go bindings:

1. Verify the same UI through `mblink-ui-dev open`, `build`, `snapshot`, `query_element`, `inspect`, representative interactions, `logs`, and `errors`.
2. Verify `esm_loader` with UI-dev snapshot/control files and observation files.
3. Verify the target binding through its UI-dev snapshot/control helper:
   - Python: `App.ui_dev_snapshot(...)`, `App.ui_dev_command(...)`, or `App.devtools_http_session(...)`
   - Rust: `App::ui_dev_snapshot(...)`, `App::ui_dev_command(...)`, or `App::devtools_http_session(...)`
   - Go: `App.UiDevSnapshot(...)`, `App.UiDevCommand(...)`, or `App.DevtoolsHttpSession(...)`
4. Compare snapshot shape, key text/control presence, geometry, console/error observation, lifecycle state, and interaction results. Wrapper names may differ by language; behavior and JSON fields should not.
5. If a capability is missing from a binding, add it through the shared C API or devtools plugin API first, then expose the idiomatic wrapper.

### Python, Go, and Rust Write-Debug Loop

Use this loop for normal host-language app development:

1. Keep `mblink-ui-dev build --watch --project /abs/path/my-app` running when iterating on shared UI files.
2. Make one UI or host change at a time. For UI changes, reload or reopen with `mblink-ui-dev`, then verify with snapshot/query/inspect/interactions.
3. For host changes, run the real host entrypoint:
   - Python: `python host/main.py`
   - Go: `go run ./host`
   - Rust: `cargo run --manifest-path rust_host/Cargo.toml`
4. In that host process, enable development observation through the binding when needed:
   - Python: `app.ui_dev_snapshot(...)`, `app.ui_dev_command(...)`, or `app.devtools_http_session(...)`
   - Go: `app.UiDevSnapshot(...)`, `app.UiDevCommand(...)`, or `app.DevtoolsHttpSession(...)`
   - Rust: `app.ui_dev_snapshot(...)`, `app.ui_dev_command(...)`, or `app.devtools_http_session(...)`
5. Reuse the same stable selectors and interaction commands from the `tool` runtime so differences identify a real parity issue.
6. Stop and fix the first mismatch between `tool`, `esm_loader`, and the host binding before adding the next slice.

For long-running manual debugging, start the host with HTTP MCP enabled and connect the agent to that endpoint for live `snapshot_ui`, `query_element`, `inspect`, `click`, `input_text`, `scroll`, and `highlight`. Use `mblink-ui-dev serve` only for project-level operations such as files, build/watch, logs/resources, and daemon state.

### Existing Project Iteration

```bash
mblink-ui-dev open
mblink-ui-dev info
mblink-ui-dev snapshot
mblink-ui-dev query ...
mblink-ui-dev inspect ...
mblink-ui-dev read src/app.js
mblink-ui-dev write src/app.js --from /tmp/new-app.js
mblink-ui-dev build --watch
mblink-ui-dev build-status
mblink-ui-dev click ...
mblink-ui-dev input-text ...
mblink-ui-dev scroll ...
mblink-ui-dev snapshot
mblink-ui-dev logs
mblink-ui-dev errors
mblink-ui-dev stop
```

### New Project Bootstrap

```bash
# initialize the default minimal/tool project into the current directory
mblink-ui-dev init

# initialize purpose-first projects into explicit directories
mblink-ui-dev init /abs/path/my-minimal-app --purpose minimal --runtime tool
mblink-ui-dev init /abs/path/my-showcase-py --purpose showcase --runtime python
mblink-ui-dev init /abs/path/my-desktop-rust --purpose desktop-app --runtime rust
mblink-ui-dev open /abs/path/my-minimal-app
mblink-ui-dev build
mblink-ui-dev snapshot
```

Host-runtime projects can still be launched from the generated project root for local checks, but the primary release path is `mblink-ui-dev build`:

```bash
python host/main.py
go run ./host
cargo run --manifest-path rust_host/Cargo.toml
```

Use `mblink-ui-dev open .` for UI iteration and mock data. Use `mblink-ui-dev build` to compile the UI resource package and the final host artifact in one step. Use the host command only when validating the real Python, Rust, or Go binding integration.
Distribute `mblink-ui-dev.exe` with adjacent `mblink.dll` and `mblink_devtools.dll` on Windows. Host-runtime init copies only `mblink.dll` into the generated Python, Go, or Rust project; `mblink_devtools.dll` remains a development companion resolved from `MBLINK_DEVTOOLS_PATH`, the runtime DLL directory, or the process search path when explicitly enabled.

## Authoring and Compatibility Rules

Read [compatibility-guidelines.md](compatibility-guidelines.md) before adding new dependencies, browser APIs, host features, or non-template syntax. The short version:

- MBlink UI is QuickJS plus MBlink DOM, not Chromium, WebView2, Node.js, or a complete Web platform.
- Use ESM and official embedded Preact modules. Do not use `preact-lite.js`, `globalThis.Preact`, `globalThis.PreactHooks`, or legacy custom `js/preact/*` APIs.
- When uncertain about a JavaScript, DOM, Preact, fetch, native element, or host bridge function, read [supported-api-reference.md](supported-api-reference.md) before using it.
- The current scaffold pattern is `import { h, render } from 'preact'` plus `h(...)`. JSX is supported by esbuild, but default JSX settings require `h` and `Fragment` in scope unless `mblink.config.json` changes them.
- TypeScript and TSX are transpiled only; `mblink-ui-dev build` is not a type checker.
- Do not use Node built-ins, CommonJS, storage APIs, workers, WebSocket, full browser navigation/download/form-submit behavior, `navigator.clipboard`, Shadow DOM, custom elements, or unverified browser-only libraries without explicit runtime verification.
- In `tool` runtime, host APIs are dev mocks. Real tray/native/resource behavior requires `python`, `rust`, or `go` host runtimes and should be verified through the same UI-dev snapshot/control path when possible.
- Use MBlink native `<terminal>` and `<logview>` for terminal and log UI. Read [native-elements.md](native-elements.md) for supported methods and caveats.

## Write Command Guidance

- Prefer stdin or `--from` for multi-line content.
- Use `--content` only when quoting is simple.
- Expect `write` to trigger file watchers when watch mode is active.

## MCP Mapping

The latest documented mapping is:

| MCP tool | CLI equivalent | Use |
|---|---|---|
| `open_project` | `open` | Open a project and bind session state |
| `init_project` | `init` | Scaffold a new project |
| `get_project_info` | `info` | Inspect current project state |
| `build` | `build` | Run single build or watch |
| `get_build_status` | `build-status` | Inspect latest build result |
| `reload` | `reload` | Restart current runtime |
| `eval_js` | `eval` | Run one-off JS in QuickJS |
| `snapshot_ui` | `snapshot` | Read live UI state |
| `get_console_logs` | `logs` | Read runtime console output |
| `get_js_errors` | `errors` | Read runtime JS errors |
| `query_element` | `query` or `query-element` | Locate elements by selector or node id |
| `inspect` | `inspect` | Diagnose layout, styles, and component state |
| `click` | `click` | Trigger interaction verification |
| `input_text` | `input-text` | Fill inputs and optionally submit |
| `scroll` | `scroll` | Move container or page scroll position |
| `highlight` | `highlight` | Visually mark target elements for debugging |
| `read_file` | `read` | Read project-relative file content |
| `write_file` | `write` | Write project-relative file content |

## Precision Tool Notes

- `query_element` returns matching elements, geometry, attributes, visibility, and interactivity.
- `inspect` returns computed style, layout, and component state when available.
- `click` supports button selection and double-click behavior.
- `input_text` supports clearing existing content and optional submit behavior.
- `scroll` targets either the root scroll area or a specific container, with `instant` or `smooth` behavior.
- `highlight` is for visual debugging and element disambiguation.

## MCP Resources

Resources are read-only views over current project state:

| URI | Meaning |
|---|---|
| `ui://snapshot` | Latest UI snapshot JSON |
| `ui://console_logs` | Latest console log JSON |
| `ui://build_status` | Latest build result JSON |
| `project://file_tree` | Project file tree |
| `project://config` | Parsed `mblink.config.json` |
| `project://file/{path}` | File content for a URL-encoded project-relative path |

## Reload Semantics

The document keeps long-term reload semantics as design guidance, but current documented behavior is simpler:

- `reload` currently maps to `restart_runtime`.
- Do not assume a public `--mode` switch exists yet.

Design-intent semantics for future expansion:

| Mode | Use when | Effect |
|---|---|---|
| `css` | Only CSS changed | Reinject styles and preserve state |
| `remount` | JS or JSX logic changed | Re-import the entry and remount the root while preserving HostBridge |
| `restart` | Config, mock wiring, or runtime health changed | Recreate the QuickJS runtime completely |

## Project Resolution Rules

Project resolution order is:

1. `--project`
2. Positional project path
3. Current directory or parents containing `.devui`
4. Current directory or parents containing `mblink.config.json`
5. Auto-fallback only when exactly one managed project exists

If more than one managed project exists and resolution is ambiguous, pass an explicit project path.

## Runtime and Build Semantics

- `snapshot` is the main observation surface.
- `logs` and `errors` are supplemental diagnostics.
- `build --watch` is daemon-held watch mode.
- Successful watch rebuilds trigger automatic runtime restart.
- If the user closes the runtime window manually, treat the project as stopped. Watch, reload, or fallback logic should not silently relaunch it.
- The first `snapshot_ui` after `open` should already be real DOM data rather than `stub-root`, due to the documented cold-start snapshot export and daemon short-poll fix.

## Snapshot Completion Evidence

- A UI task is not complete until `snapshot` or `snapshot_ui` has been run against the live runtime after the latest build or reload.
- The completion snapshot must contain real rendered UI nodes, expected key text or controls, nonzero rects for important regions, and no empty root, `stub-root`, placeholder-only shell, or hidden-only content.
- Use `query_element` after the snapshot for the key selectors that prove the intended controls are present and targetable.
- For visual, layout, or style-sensitive changes, use `inspect` on the relevant nodes and verify visibility, geometry, and style facts.
- If snapshot cannot run, fails, is stale, or does not show the expected UI, the correct status is not fully verified; keep diagnosing instead of claiming the UI work is done.
- Final user-facing summaries for UI work must name the snapshot or query checks that passed. Build success and clean logs are supporting evidence only.

## Snapshot Shape

`snapshot_ui` returns a JSON object that includes:

- `timestamp`
- `viewport`
- `tree`
- `screenshot` metadata when `include_screenshot` is true

Current documented notes:

- `include_screenshot` is opt-in. Default snapshots do not include screenshot bytes or write a PNG.
- Screenshot-enabled file responses include `screenshot.included: true`, `screenshot.mime_type: "image/png"`, `screenshot.path`, `screenshot.bytes`, dimensions, and `dpr`.
- `screenshot_base64` is empty unless `--inline-screenshot` or MCP `inline_screenshot: true` is explicitly requested.
- `snapshot` defaults to `--response auto`: small snapshots are returned inline; large snapshots return `response_mode: "file"` with `snapshot.path` and `snapshot.bytes` so callers can read the JSON from disk without moving a large payload through daemon IPC, stdout, or MCP text.
- Use `snapshot --response file --include-screenshot` or MCP `snapshot_ui({ "response_mode": "file", "include_screenshot": true })` when the caller can read files directly and wants the most reliable path for DOM JSON plus PNG output.

Important node fields inside `tree`:

- `node_id`
- `tag`
- `id`
- `class`
- `text`
- `rect`
- `attrs`
- `interactive`
- `visible`
- `scroll`
- `children`

## Error Shape

Build and runtime errors follow a common structure with fields such as:

- `type`
- `severity`
- `message`
- `file`
- `line`
- `column`
- `stack`
- `context`

## Tool Priority Rules

1. Snapshot before acting and before completion.
2. Prefer dedicated UI control commands such as `query_element`, `inspect`, `click`, `input_text`, `scroll`, and `highlight` over general-purpose escape hatches.
3. Query first and inspect second when the exact element is unclear.
4. Prefer source edits over `eval_js`.
5. After interactions, re-check with query or snapshot immediately.
6. Stop on build failures before reloading.
7. Use logs and JS errors to explain what snapshots cannot; never use them as a replacement for snapshot evidence.

## Templates

The canonical scaffold model is purpose-first, runtime-second:

| Purpose | Use when |
|---|---|
| `minimal` | Smallest runnable MBlink UI Dev project and smoke-test target |
| `showcase` | Capability gallery for MBlink-specific features such as borderless-friendly layout, tray/native host hooks, resources, and state |
| `desktop-app` | Production-like desktop app skeleton with app shell, theme, and runtime adapter boundary |

| Runtime | Host role |
|---|---|
| `tool` | Frontend-only project loaded directly by `mblink-ui-dev` |
| `python` | Runnable Python host adapter plus shared UI |
| `rust` | Runnable Rust host adapter plus shared UI |
| `go` | Runnable Go host adapter plus shared UI |

Initial supported combinations are the full 12-pair matrix: `minimal`, `showcase`, and `desktop-app` crossed with `tool`, `python`, `rust`, and `go`. Bare CLI `init` and MCP `init_project({ "path": ... })` default to `minimal/tool`. Partial canonical input is invalid: provide both `purpose` and `runtime`, or provide neither.

### Showcase Capability Matrix

The `showcase` purpose is the canonical feature-discovery template. Runtime support is explicit so agents do not assume host-only behavior exists in the frontend-only tool runtime.

| Capability | `tool` | `python` | `rust` | `go` |
|---|---|---|---|---|
| Shared UI capability cards/state interaction | supported | supported | supported | supported |
| Borderless/frameless-friendly layout | supported in UI | supported via host adapter target | supported via host adapter target | supported via host adapter target |
| Tray integration | documented caveat only | supported target from binding examples | supported target from binding examples | supported target from binding examples |
| Native controls/window actions | documented caveat only | supported target from binding examples | supported target from binding examples | supported target from binding examples |
| Resource loading/mounting | frontend docs only | supported target from binding examples | supported target from binding examples | supported target from binding examples |

`showcase/tool` is intentionally shipped as `supported-with-caveat`: it teaches the UI shape and feature vocabulary, but tray/native/resource behavior requires a host runtime.

Legacy `--template` names are accepted during the transition and return warnings in JSON output:

| Legacy name | Canonical target |
|---|---|
| `preact-jsx` | `minimal/tool` |
| `preact-ts` | `minimal/tool` |
| `vanilla-js` | `minimal/tool` |
| `python` | `showcase/python` |
| `go` | `showcase/go` |
| `rust` | `showcase/rust` |
| `python-host` | `showcase/python` |
| `rust-host` | `showcase/rust` |

Templates are embedded into the built `mblink-ui-dev.exe`, so init does not rely on shipping a separate external templates directory at distribution time. The MBlink runtime library is not duplicated inside those templates; it is copied from the `mblink-ui-dev.exe` directory during host-runtime init.

Layering is intentionally slim:

1. `base/common` provides shared docs, `ui/bridge.js`, and theme primitives.
2. `runtime/<python|rust|go>` provides the real host executable starter.
3. `purpose/<minimal|showcase|desktop-app>/shared` provides the purpose-specific UI.
4. `purpose/<purpose>/runtime/<runtime>` provides the resolved `mblink.config.json`.

The shared UI calls `getTemplateInfo`, `incrementCounter`, `submitValidation`, and `trayAction`. In `tool` runtime those calls are handled by the dev mock bridge. In host runtimes the generated Python, Rust, or Go program binds the same API names and loads `ui/app.js` directly.
The generated Rust host vendors the MBlink Rust crates and receives the runtime DLL in `rust_host/vendor/...` during init, so it does not require a checkout at the original repository path.
The build command compiles `ui/app.js` into a shared `app.mbrp` resource package first, then invokes the selected host packager (`cargo`, `go build`, or `PyInstaller`) to produce the final artifact.

Canonical scaffold shape:

```text
my-app/
  README.md
  mblink.config.json
  ui/
    app.js
    bridge.js
    styles/theme.js
  host/main.py              # python runtime only
  host/main.go              # go runtime only
  go.mod                    # go runtime only
  .mblink/mblink-go/          # go runtime only, includes copied mblink.dll on Windows
  requirements.txt          # python runtime only
  vendor/mblink/bin/mblink.dll # python runtime only on Windows
  rust_host/Cargo.toml      # rust runtime only
  rust_host/src/main.rs     # rust runtime only
  rust_host/vendor/mblink/   # rust runtime only
  rust_host/vendor/mblink-sys/runtime/mblink.dll # rust runtime only on Windows
```

Generated result JSON includes `purpose`, `runtime`, `canonical_key`, `supported_combinations`, `legacy_templates`, `layers`, `warnings`, and `used_default` so agents can verify the resolved scaffold contract without guessing.

## `mblink.config.json` Notes

Expect these top-level areas:

- `name`
- `template`
- `entry`
- `src_dir`
- `out_dir`
- `mock_dir`
- `window`
- `build`
- `dev`
- `mcp`

Default dev-oriented expectations include:

- `build.builder = "esbuild"`
- `build.icon = "assets/app.ico"` for host-runtime Windows exe icon packaging; only `.ico` is accepted, and missing or invalid icons are reported as skipped instead of failing the build.
- `dev.watch = true`
- `dev.debounce_ms = 100`
- `dev.auto_reload = true`
- `mcp.transport = "stdio"`
- `mcp.snapshot_include_screenshot = true`
- `mcp.snapshot_max_depth = 20`

## MCP Transport and Configuration Snippets

`mblink-ui-dev serve` is the default stdio MCP transport for project management, build/watch, files, logs, and UI control through the managed daemon:

Generic server:

```json
{
  "mcpServers": {
    "mblink-ui-dev": {
      "command": "mblink-ui-dev",
      "args": ["serve"],
      "env": {}
    }
  }
}
```

Single fixed project:

```json
{
  "mcpServers": {
    "mblink-ui-dev": {
      "command": "mblink-ui-dev",
      "args": ["serve", "--project", "/path/to/project"],
      "env": {}
    }
  }
}
```

Runtime HTTP MCP:

- `mblink_devtools.dll` can expose a local Streamable HTTP MCP endpoint from a C API host when that host explicitly starts devtools HTTP.
- Start it through `esm_loader --devtools-http-mcp`, Python `App.devtools_http_session(...)`, Rust `App::devtools_http_session(...)`, Go `App.DevtoolsHttpSession(...)`, or the underlying `mblink_devtools_http_start` C API.
- The endpoint is localhost-only, validates local `Origin`, and requires `Authorization: Bearer <token>` or `X-MBLINK-DevTools-Token`.
- Its scope is live UI analysis/control: `snapshot_ui`, `query_element`, `inspect`, `click`, `input_text`, `scroll`, and `highlight`.
- `snapshot_ui` over HTTP supports `max_nodes`, `max_depth`, `root_selector`, and `include_screenshot`; inline screenshots are not supported over HTTP MCP.
- It does not replace `mblink-ui-dev serve` for init, build, watch, file, log, error, daemon, or project-resource operations.

Future ecosystem direction:

- MCP notifications for build status and file change events
- Additional project templates and host integration variants
- Snapshot lazy-loading and depth controls for large trees
- VS Code side-panel MBlink UI preview

## Relationship to Existing MBlink Tools

- `tools/esm_loader` provides the core runtime behavior that `mblink-ui-dev` builds on.
- `tools/app_bundler` remains the production bundling path.
- `core/devtools/` provides the DOM access foundation for snapshot and inspection.
- `js/runtime/bootstrap.js` remains the root-registration bootstrap dependency.
