# MBink UI Dev Reference

Read this file when exact command names, MCP mappings, templates, routing rules, reload semantics, or ecosystem integrations matter. This reference follows the latest documented progress: P2 is complete, P3 precision UI control is available, and P4 ecosystem work remains future-facing.

## Output Rules

- Treat CLI stdout as machine-readable JSON.
- Treat CLI stderr as human-readable logs.
- Treat exit code `0` as success.
- Prefer parsing tool output instead of scraping stderr text.

## CLI Surface

```bash
# Project initialization
mbink-ui-dev init [path] [--purpose <minimal|showcase|desktop-app> --runtime <tool|python|rust|go>] [--template <legacy-name>]

# Daemon management
mbink-ui-dev daemon start [--project <path>]
mbink-ui-dev daemon run [--project <path>]
mbink-ui-dev daemon stop [--project <path>]
mbink-ui-dev daemon status [--project <path>]
mbink-ui-dev stop [--project <path>]

# MCP adapter
mbink-ui-dev serve [--project <path>]

# Project and runtime control
mbink-ui-dev open [<project-path>]
mbink-ui-dev info [--project <path>]
mbink-ui-dev reload [--project <path>]
mbink-ui-dev eval "<js code>" [--project <path>]

# File operations
mbink-ui-dev read <path> [--encoding utf8|base64] [--project <path>]
mbink-ui-dev write <path> [--content <text> | --from <file>] [--project <path>]

# Build and observation
mbink-ui-dev build [--watch] [--project <path>]
mbink-ui-dev build-status [--project <path>]
mbink-ui-dev snapshot [--project <path>]
mbink-ui-dev snapshot [--response auto|inline|file] [--project <path>]
mbink-ui-dev snapshot --include-screenshot [--inline-screenshot] [--response auto|inline|file] [--project <path>]
mbink-ui-dev logs [--project <path>]
mbink-ui-dev errors [--project <path>]

# Precision control and inspection
mbink-ui-dev query ...
mbink-ui-dev query-element ...
mbink-ui-dev inspect ...
mbink-ui-dev click ...
mbink-ui-dev input-text ...
mbink-ui-dev scroll ...
mbink-ui-dev highlight ...
```

For precision-control work, prefer MCP tool schemas when exact argument structure matters.

## Recommended CLI Sequences

### UI-First Then Host Integration

Build and prove the UI before writing host-language code:

```bash
mbink-ui-dev init /abs/path/my-app --purpose desktop-app --runtime tool
mbink-ui-dev open /abs/path/my-app
mbink-ui-dev build
mbink-ui-dev snapshot
mbink-ui-dev query-element ...
mbink-ui-dev inspect ...
mbink-ui-dev click ...
mbink-ui-dev input-text ...
mbink-ui-dev scroll ...
mbink-ui-dev logs
mbink-ui-dev errors
```

Use `ui/bridge.js` mock data for this stage. The UI stage is not complete until build succeeds, the snapshot contains real UI nodes, important controls can be queried by stable selectors, representative interactions work, and logs/errors are clean enough to explain.

Develop this sequence incrementally. Add one component, section, mock state, or interaction path, then run the relevant build, snapshot, query, inspect, interaction, log, and error checks before adding the next part. If the last addition causes a blank UI, missing selector, layout issue, build error, or JS error, fix that addition before continuing.

After that gate passes, add or modify the real Python, Rust, or Go host adapter behind the same `hostApi` names, then run the host integration and final release build:

```bash
mbink-ui-dev build --project /abs/path/my-app
```

Manual host commands are only for targeted binding checks after UI validation has passed.

### Existing Project Iteration

```bash
mbink-ui-dev open
mbink-ui-dev info
mbink-ui-dev snapshot
mbink-ui-dev query ...
mbink-ui-dev inspect ...
mbink-ui-dev read src/app.js
mbink-ui-dev write src/app.js --from /tmp/new-app.js
mbink-ui-dev build --watch
mbink-ui-dev build-status
mbink-ui-dev click ...
mbink-ui-dev input-text ...
mbink-ui-dev scroll ...
mbink-ui-dev snapshot
mbink-ui-dev logs
mbink-ui-dev errors
mbink-ui-dev stop
```

### New Project Bootstrap

```bash
# initialize the default minimal/tool project into the current directory
mbink-ui-dev init

# initialize purpose-first projects into explicit directories
mbink-ui-dev init /abs/path/my-minimal-app --purpose minimal --runtime tool
mbink-ui-dev init /abs/path/my-showcase-py --purpose showcase --runtime python
mbink-ui-dev init /abs/path/my-desktop-rust --purpose desktop-app --runtime rust
mbink-ui-dev open /abs/path/my-minimal-app
mbink-ui-dev build
mbink-ui-dev snapshot
```

Host-runtime projects can still be launched from the generated project root for local checks, but the primary release path is `mbink-ui-dev build`:

```bash
python host/main.py
go run ./host
cargo run --manifest-path rust_host/Cargo.toml
```

Use `mbink-ui-dev open .` for UI iteration and mock data. Use `mbink-ui-dev build` to compile the UI resource package and the final host artifact in one step. Use the host command only when validating the real Python, Rust, or Go binding integration.
Distribute `mbink-ui-dev.exe` with the adjacent MBink runtime library (`mbink.dll` on Windows). Host-runtime init copies that runtime library into the generated Python, Go, or Rust project.

## Authoring and Compatibility Rules

Read [compatibility-guidelines.md](compatibility-guidelines.md) before adding new dependencies, browser APIs, host features, or non-template syntax. The short version:

- MBink UI is QuickJS plus MBink DOM, not Chromium, WebView2, Node.js, or a complete Web platform.
- Use ESM and official embedded Preact modules. Do not use `preact-lite.js`, `globalThis.Preact`, `globalThis.PreactHooks`, or legacy custom `js/preact/*` APIs.
- When uncertain about a JavaScript, DOM, Preact, fetch, native element, or host bridge function, read [supported-api-reference.md](supported-api-reference.md) before using it.
- The current scaffold pattern is `import { h, render } from 'preact'` plus `h(...)`. JSX is supported by esbuild, but default JSX settings require `h` and `Fragment` in scope unless `mbink.config.json` changes them.
- TypeScript and TSX are transpiled only; `mbink-ui-dev build` is not a type checker.
- Do not use Node built-ins, CommonJS, storage APIs, workers, WebSocket, full browser navigation/download/form-submit behavior, `navigator.clipboard`, Shadow DOM, custom elements, or unverified browser-only libraries without explicit runtime verification.
- In `tool` runtime, host APIs are dev mocks. Real tray/native/resource behavior requires `python`, `rust`, or `go` host runtimes.
- Use MBink native `<terminal>` and `<logview>` for terminal and log UI. Read [native-elements.md](native-elements.md) for supported methods and caveats.

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
| `project://config` | Parsed `mbink.config.json` |
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
4. Current directory or parents containing `mbink.config.json`
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
| `minimal` | Smallest runnable MBink UI Dev project and smoke-test target |
| `showcase` | Capability gallery for MBink-specific features such as borderless-friendly layout, tray/native host hooks, resources, and state |
| `desktop-app` | Production-like desktop app skeleton with app shell, theme, and runtime adapter boundary |

| Runtime | Host role |
|---|---|
| `tool` | Frontend-only project loaded directly by `mbink-ui-dev` |
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

Templates are embedded into the built `mbink-ui-dev.exe`, so init does not rely on shipping a separate external templates directory at distribution time. The MBink runtime library is not duplicated inside those templates; it is copied from the `mbink-ui-dev.exe` directory during host-runtime init.

Layering is intentionally slim:

1. `base/common` provides shared docs, `ui/bridge.js`, and theme primitives.
2. `runtime/<python|rust|go>` provides the real host executable starter.
3. `purpose/<minimal|showcase|desktop-app>/shared` provides the purpose-specific UI.
4. `purpose/<purpose>/runtime/<runtime>` provides the resolved `mbink.config.json`.

The shared UI calls `getTemplateInfo`, `incrementCounter`, `submitValidation`, and `trayAction`. In `tool` runtime those calls are handled by the dev mock bridge. In host runtimes the generated Python, Rust, or Go program binds the same API names and loads `ui/app.js` directly.
The generated Rust host vendors the MBink Rust crates and receives the runtime DLL in `rust_host/vendor/...` during init, so it does not require a checkout at the original repository path.
The build command compiles `ui/app.js` into a shared `app.mbrp` resource package first, then invokes the selected host packager (`cargo`, `go build`, or `PyInstaller`) to produce the final artifact.

Canonical scaffold shape:

```text
my-app/
  README.md
  mbink.config.json
  ui/
    app.js
    bridge.js
    styles/theme.js
  host/main.py              # python runtime only
  host/main.go              # go runtime only
  go.mod                    # go runtime only
  .mbink/mbink-go/          # go runtime only, includes copied mbink.dll on Windows
  requirements.txt          # python runtime only
  vendor/mbink/bin/mbink.dll # python runtime only on Windows
  rust_host/Cargo.toml      # rust runtime only
  rust_host/src/main.rs     # rust runtime only
  rust_host/vendor/mbink/   # rust runtime only
  rust_host/vendor/mbink-sys/runtime/mbink.dll # rust runtime only on Windows
```

Generated result JSON includes `purpose`, `runtime`, `canonical_key`, `supported_combinations`, `legacy_templates`, `layers`, `warnings`, and `used_default` so agents can verify the resolved scaffold contract without guessing.

## `mbink.config.json` Notes

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
- `dev.watch = true`
- `dev.debounce_ms = 100`
- `dev.auto_reload = true`
- `mcp.transport = "stdio"`
- `mcp.snapshot_include_screenshot = true`
- `mcp.snapshot_max_depth = 20`

## MCP Transport and Configuration Snippets

Generic server:

```json
{
  "mcpServers": {
    "mbink-ui-dev": {
      "command": "mbink-ui-dev",
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
    "mbink-ui-dev": {
      "command": "mbink-ui-dev",
      "args": ["serve", "--project", "/path/to/project"],
      "env": {}
    }
  }
}
```

Future ecosystem direction:

- HTTP plus SSE transport for IDE integrations
- MCP notifications for build status and file change events
- Additional project templates and host integration variants
- Snapshot lazy-loading and depth controls for large trees
- VS Code side-panel MBink UI preview

## Relationship to Existing MBink Tools

- `tools/esm_loader` provides the core runtime behavior that `mbink-ui-dev` builds on.
- `tools/app_bundler` remains the production bundling path.
- `core/devtools/` provides the DOM access foundation for snapshot and inspection.
- `js/runtime/bootstrap.js` remains the root-registration bootstrap dependency.
