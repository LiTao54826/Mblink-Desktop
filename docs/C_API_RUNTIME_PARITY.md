# C API Runtime Parity

MBink keeps one public runtime contract for native hosts and tools: the C API in `core/api/mbink.h`.
`esm_loader`, Python, Go, Rust, and future bindings should route observable runtime behavior through that contract.

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
`mbink_ui_dev` can keep its own developer CLI and daemon surface, but runtime behavior that must also work in bindings belongs in the C API.

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

The following API families are parity-critical:

- lifecycle: `mbink_lifecycle_state`, `mbink_lifecycle_reason`, `mbink_observe_lifecycle_json`
- console/errors: `mbink_observe_console_json`, `mbink_observe_errors_json`, `mbink_observe_clear`, `mbink_observe_set_callback`
- UI dev snapshot/control: `mbink_ui_dev_snapshot_json`, `mbink_ui_dev_snapshot_file`, `mbink_ui_dev_command_json`
- loading/rendering: `mbink_load_entry_file`, `mbink_load_module_file`, `mbink_render_frame`, `mbink_runtime_epoch`

Bindings should expose these shapes without inventing language-specific semantics.
Wrapper names can be idiomatic, but the behavior and JSON fields should remain aligned.

## Regression Gates

Run these checks after changing the C API runtime path, `esm_loader`, `mbink-ui-dev` runtime control, or a binding wrapper:

```powershell
cmake --build build --config Release --target mbink_api esm_loader mbink_ui_dev
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
