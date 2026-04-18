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
mbink-ui-dev init <path> [--template <preact-jsx|preact-ts|vanilla-js|python|go|rust>]

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
mbink-ui-dev init /abs/path/my-app --template preact-jsx
mbink-ui-dev init /abs/path/my-py-app --template python
mbink-ui-dev init /abs/path/my-go-app --template go
mbink-ui-dev init /abs/path/my-rust-app --template rust
mbink-ui-dev open /abs/path/my-app
mbink-ui-dev build
mbink-ui-dev snapshot
```

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

## Snapshot Shape

`snapshot_ui` returns a JSON object that includes:

- `timestamp`
- `viewport`
- `tree`

Current documented notes:

- `include_screenshot` exists as an input field but is currently a reserved flag and does not return a screenshot payload.
- `max_depth` and `root_selector` are also reserved fields in the current implementation.

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

1. Snapshot before acting.
2. Prefer dedicated UI control commands such as `query_element`, `inspect`, `click`, `input_text`, `scroll`, and `highlight` over general-purpose escape hatches.
3. Query first and inspect second when the exact element is unclear.
4. Prefer source edits over `eval_js`.
5. After interactions, re-check with query or snapshot immediately.
6. Stop on build failures before reloading.
7. Use logs and JS errors to explain what snapshots cannot.

## Templates

| Template | Use when | Host |
|---|---|---|
| `preact-jsx` | JSX plus Preact with direct official `preact` / `preact/hooks` imports | None |
| `preact-ts` | TypeScript plus JSX plus Preact | None |
| `vanilla-js` | Simple UI without framework dependency | None |
| `python` | Need a Python host integration starter with a multi-file frontend scaffold | Python |
| `go` | Need a Go host integration starter with a multi-file frontend scaffold | Go |
| `rust` | Need a Rust host integration starter with a multi-file frontend scaffold | Rust |

Aliases kept for compatibility: `python-host` → `python`, `rust-host` → `rust`.

`preact-jsx` scaffold shape:

```text
my-app/
  mbink.config.json
  src/
    App.jsx
    index.html
    components/
  mock/
    host.js
    state.js
  .dist/
```

Host starter scaffold shape (`python` / `go` / `rust`):

```text
my-host-app/
  mbink.config.json
  src/
    app.js
    components/
      AppShell.js
      InfoCard.js
      ActionList.js
  host/main.py          # python
  host/main.go          # go
  rust_host/src/main.rs # rust
```

The host starters use real `import ... from` composition in `src/app.js` and keep the default window size at `800x600`.

In `preact-jsx` and `preact-ts`, the generated entries import official `preact` / `preact/hooks` modules directly instead of relying on global `Preact` / `PreactHooks` injection.

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
