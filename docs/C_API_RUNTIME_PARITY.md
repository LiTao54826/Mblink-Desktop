# C API Runtime Parity

MBink keeps one public runtime contract for native hosts and tools: the core C API in `core/api/mbink.h`.
Development-only UI analysis and control live in the optional plugin C API in `core/devtools/mbink_devtools.h`.
`esm_loader`, Python, Go, Rust, and future bindings should route observable runtime behavior through these two C ABI surfaces instead of language-specific shortcuts.

The project is not released yet, so compatibility with older wrapper behavior is not a goal. Consistent behavior is the goal.

## Shared Runtime Path

The thin loader target is `tools/esm_loader`.
It links to `mbink_api` and loads the adjacent runtime library at process start.
On Windows, the expected dependent set for `esm_loader.exe` is intentionally small:

```text
mbink.dll
KERNEL32.dll
```

The large embedded runtime resources live in `mbink.dll`, not duplicated into `esm_loader.exe`.
`mbink_devtools.dll` is a development companion used only when snapshot/control/HTTP MCP is explicitly enabled.
`mbink_ui_dev` can keep its own developer CLI and daemon surface, but runtime behavior that must also work in bindings belongs in the core C API or the devtools plugin C API.

## Runtime Setup Contract

Every binding or tool that creates an app should use this sequence:

1. `mbink_init`
2. `mbink_create` or `mbink_create_ex`
3. `mbink_default_runtime_options`
4. `mbink_configure_runtime`
5. one load call, usually `mbink_load_entry_file`
6. `mbink_render_frame` when deterministic observation is needed
7. `mbink_poll_events` or `mbink_run`
8. `mbink_destroy`
9. `mbink_cleanup`

`mbink_configure_runtime` is the canonical place to set `runtime_epoch` and decide whether embedded runtime scripts and official Preact modules are loaded.

## Shared Observation Contract

The following core API families are parity-critical:

- lifecycle: `mbink_lifecycle_state`, `mbink_lifecycle_reason`, `mbink_observe_lifecycle_json`
- console/errors: `mbink_observe_console_json`, `mbink_observe_errors_json`, `mbink_observe_clear`, `mbink_observe_set_callback`
- loading/rendering: `mbink_load_entry_file`, `mbink_load_module_file`, `mbink_render_frame`, `mbink_runtime_epoch`

The following development plugin API families are also parity-critical when the plugin is present:

- UI dev snapshot/control: `mbink_ui_dev_snapshot_json`, `mbink_ui_dev_snapshot_file`, `mbink_ui_dev_command_json`
- DevTools/MCP transport: `mbink_devtools_open`, `mbink_devtools_close`, `mbink_devtools_http_start`, `mbink_devtools_http_stop`

Bindings should expose these shapes without inventing language-specific semantics.
Wrapper names can be idiomatic, but the behavior and JSON fields should remain aligned.
Binding packages should not vendor `mbink_devtools.dll`; development tooling should provide it next to `mbink.dll` or through `MBINK_DEVTOOLS_PATH`, and bindings should load it only when devtools features are used.

## Regression Gates

Run these checks after changing the C API runtime path, `esm_loader`, `mbink-ui-dev` runtime control, or a binding wrapper:

```powershell
cmake --build build --config Release --target mbink_api mbink_devtools esm_loader mbink_ui_dev
powershell -NoProfile -ExecutionPolicy Bypass -File tests\regression\test_esm_loader_c_api_parity.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File tests\regression\test_mbink_ui_dev_p0_regression.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File tests\regression\test_mbink_ui_dev_responsiveness.ps1
py -3 -m py_compile bindings\python\mbink\_ffi.py bindings\python\mbink\app.py
```

When Go and Rust toolchains are available, also run:

```powershell
Push-Location bindings\go
go test ./...
Pop-Location

Push-Location bindings\rust
cargo check -p mbink-sys -p mbink
Pop-Location
```

The regression script `test_esm_loader_c_api_parity.ps1` checks the thin dynamic-library dependency, DOM snapshot, console/error/lifecycle JSON, `runtime_epoch`, UI-dev command handling, and `--no-scripts` behavior.
