# C API Runtime Parity

MBlink keeps one public runtime contract for native hosts and tools: the core C API in `core/api/mblink.h`.
Development-only UI analysis and control live in the optional plugin C API in `core/devtools/mblink_devtools.h`.
`esm_loader`, Python, Go, Rust, and future bindings should route observable runtime behavior through these two C ABI surfaces instead of language-specific shortcuts.

The project is not released yet, so compatibility with older wrapper behavior is not a goal. Consistent behavior is the goal.

## Shared Runtime Path

The thin loader target is `tools/esm_loader`.
It links to `mblink_api` and loads the adjacent runtime library at process start.
On Windows, the expected dependent set for `esm_loader.exe` is intentionally small:

```text
mblink.dll
KERNEL32.dll
```

The large embedded runtime resources live in `mblink.dll`, not duplicated into `esm_loader.exe`.
`mblink_devtools.dll` is a development companion used only when snapshot/control/HTTP MCP is explicitly enabled.
`mblink_ui_dev` can keep its own developer CLI and daemon surface, but runtime behavior that must also work in bindings belongs in the core C API or the devtools plugin C API.

## Runtime Setup Contract

Every binding or tool that creates an app should use this sequence:

1. `mblink_init`
2. `mblink_create` or `mblink_create_ex`
3. `mblink_default_runtime_options`
4. `mblink_configure_runtime`
5. one load call, usually `mblink_load_entry_file`
6. `mblink_render_frame` when deterministic observation is needed
7. `mblink_poll_events` or `mblink_run`
8. `mblink_destroy`
9. `mblink_cleanup`

`mblink_configure_runtime` is the canonical place to set `runtime_epoch` and decide whether embedded runtime scripts and official Preact modules are loaded.

## Shared Observation Contract

The following core API families are parity-critical:

- lifecycle: `mblink_lifecycle_state`, `mblink_lifecycle_reason`, `mblink_observe_lifecycle_json`
- console/errors: `mblink_observe_console_json`, `mblink_observe_errors_json`, `mblink_observe_clear`, `mblink_observe_set_callback`
- loading/rendering: `mblink_load_entry_file`, `mblink_load_module_file`, `mblink_render_frame`, `mblink_runtime_epoch`

The following development plugin API families are also parity-critical when the plugin is present:

- UI dev snapshot/control: `mblink_ui_dev_snapshot_json`, `mblink_ui_dev_snapshot_file`, `mblink_ui_dev_command_json`
- DevTools/MCP transport: `mblink_devtools_open`, `mblink_devtools_close`, `mblink_devtools_http_start`, `mblink_devtools_http_stop`

Bindings should expose these shapes without inventing language-specific semantics.
Wrapper names can be idiomatic, but the behavior and JSON fields should remain aligned.
Binding packages should not vendor `mblink_devtools.dll`; development tooling should provide it next to `mblink.dll` or through `MBLINK_DEVTOOLS_PATH`, and bindings should load it only when devtools features are used.

## Regression Gates

Run these checks after changing the C API runtime path, `esm_loader`, `mblink-ui-dev` runtime control, or a binding wrapper:

```powershell
cmake --build build --config Release --target mblink_api mblink_devtools esm_loader mblink_ui_dev
powershell -NoProfile -ExecutionPolicy Bypass -File tests\regression\test_esm_loader_c_api_parity.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File tests\regression\test_mblink_ui_dev_p0_regression.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File tests\regression\test_mblink_ui_dev_responsiveness.ps1
py -3 -m py_compile bindings\python\mblink\_ffi.py bindings\python\mblink\app.py
```

When Go and Rust toolchains are available, also run:

```powershell
Push-Location bindings\go
go test ./...
Pop-Location

Push-Location bindings\rust
cargo check -p mblink-sys -p mblink
Pop-Location
```

The regression script `test_esm_loader_c_api_parity.ps1` checks the thin dynamic-library dependency, DOM snapshot, console/error/lifecycle JSON, `runtime_epoch`, UI-dev command handling, and `--no-scripts` behavior.
