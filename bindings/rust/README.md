# MBink Rust Bindings

Rust bindings for MBink live under this workspace:

- `mbink-sys`: raw FFI bindings to `mbink.h`
- `mbink`: safe Rust wrapper with RAII, `Result`, JSON helpers, callbacks, and resource package helpers

Runtime behavior follows the shared C API contract documented in `docs/C_API_RUNTIME_PARITY.md`.

## Layout

```text
bindings/rust/
├─ Cargo.toml
├─ mbink-sys/
└─ mbink/
```

## Dynamic library lookup

`mbink-sys` links against the MBink dynamic library.

Supported environment variables:

- `MBINK_LIB_DIR`: directory containing the native MBink library
- `MBINK_LIB_NAME`: library name, defaults to `mbink`
- `MBINK_DLL_PATH`: explicit DLL path used by runtime dynamic loading on Windows

If `MBINK_LIB_DIR` is not set, the build script falls back to:

```text
bindings/rust/mbink-sys/runtime
```

On Windows runtime dynamic loading looks for the DLL in this order:

1. `MBINK_DLL_PATH`
2. `bindings/rust/mbink-sys/runtime/mbink.dll`
3. `mbink.dll` from the process working directory / system search path

`mbink_devtools.dll` is development-only and is not vendored by the Rust binding package. When `App::enable_devtools_http`, `App::devtools_http_session`, or UI-dev snapshot/control helpers are used, the Rust devtools runtime loader resolves it from `MBINK_DEVTOOLS_PATH`, then from the loaded `mbink.dll` directory, then from the process/runtime search path.

## Build

From `bindings/rust/`:

```bash
cargo check
cargo check -p mbink --examples
```

## Hello example

```bash
cargo run -p mbink --example hello
```

This creates a window and loads inline HTML.

## Bind example

```bash
cargo run -p mbink --example bind
```

This registers a Rust callback:

- JS calls `backend.greet({...})`
- Rust receives `serde_json::Value`
- Rust returns JSON back to JS

## Events example

```bash
cargo run -p mbink --example events
```

This demonstrates `on_resize`, `on_focus`, `on_blur`, `on_close`, and `on_close_request`.

## Tray example

```bash
cargo run -p mbink --example tray
```

This demonstrates a borderless window, tray creation, click-to-restore, hide-to-tray on close, and quit from the tray menu.

## Window controls example

```bash
cargo run -p mbink --example window
```

This demonstrates window positioning, min/max size, and DevTools helpers.

## Shared example

```bash
cargo run -p mbink --example shared
```

This populates a shared object from Rust and reads it in the page.

## State example

```bash
cargo run -p mbink --example state
```

This initializes window state from Rust and renders it in the page.

## Resource mount example

```bash
cargo run -p mbink --example resources_mount
```

This compiles a temporary directory into a resource package, mounts it on the app, and loads `/input/index.html` from the package.

## Resource read example

```bash
cargo run -p mbink --example resources_read
```

This compiles a small package and reads a file back through `load_resource_file`.

## Minimal usage

```rust
use mbink::App;

fn main() -> mbink::Result<()> {
    let mut app = App::builder()
        .title("MBink Rust Hello")
        .size(800, 600)
        .build()?;

    app.load_html("<html><body><h1>Hello</h1></body></html>")?;
    app.run();
    Ok(())
}
```

## Resource helpers

```rust
use mbink::{App, compile_resources, load_resource_file};

fn demo() -> mbink::Result<()> {
    compile_resources("assets", "assets.mbk", "")?;

    let file = load_resource_file("assets.mbk", "assets/index.html", "")?;
    let html = file.into_utf8_string()?;

    let app = App::new("MBink", 800, 600)?;
    app.mount_resource_package("assets.mbk", "", "/")?;
    app.load_html_file("/assets/index.html")?;
    Ok(())
}
```

`load_resource_file` returns `ResourceFile`, which owns the returned bytes and automatically frees MBink-allocated memory in the wrapper layer.

`ResourceFile` currently exposes:

- `data()`
- `into_bytes()`
- `flags()`
- `is_bytecode()`
- `into_utf8_string()`

Use `RESOURCE_FLAG_BYTECODE` or `ResourceFile::is_bytecode()` to detect whether a loaded `.js/.mjs` resource was compiled to QuickJS bytecode.

## Current safe wrapper coverage

- App creation and window lifecycle
- window helpers:
  - `set_position` / `position`
  - `set_min_size` / `set_max_size`
  - `minimize` / `maximize` / `restore`
  - `set_fullscreen`
  - `set_resizable`
  - `set_borderless`
  - `set_always_on_top`
- HTML / JS loading and evaluation
- tray helpers:
  - `create_tray`
  - `destroy_tray`
  - `set_tray_tooltip`
  - `set_tray_menu` / `set_tray_menu_json`
  - `on_tray_click`
  - `on_tray_menu`
- resource package helpers:
  - `compile_resources`
  - `App::mount_resource_package`
  - `load_resource_file`
- state read/write helpers
  - `create_*` helpers
  - `get_length` / `get_at` / `get_key`
  - `array_*` helpers
  - `object_*` helpers
  - `increment` / `multiply`
  - `string_append` / `string_prepend`
  - `set_merge_mode` / `process_queue` / `queue_size`
- state watch helpers:
  - `App::watch_state`
  - `App::unwatch_state`
- shared object read/write helpers
- control helpers:
  - `App::logview` with `LogView::{append, clear, export}`
  - `App::terminal` with `Terminal::{write, clear, execute, start_shell, send_input, resize, serialize}`
- `bind` / `bind_async` / `unbind`
- DevTools helpers:
  - `devtools_open`
  - `devtools_close`
  - `enable_devtools_http` / `devtools_http_session`
- basic event callbacks:
  - `on_resize`
  - `on_close`
  - `on_close_request`
  - `on_focus`
  - `on_blur`
  - `on_update`

## Notes

Current callback behavior is intentionally conservative:

- one callback per event in the safe layer
- callback lifetime is owned by `App`
- bind arguments and return values use `serde_json::Value`
