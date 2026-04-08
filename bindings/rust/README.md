# MBink Rust Bindings

Rust bindings for MBink live under this workspace:

- `mbink-sys`: raw FFI bindings to `mbink.h`
- `mbink`: safe Rust wrapper with RAII, `Result`, JSON helpers, callbacks, and resource package helpers

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

If `MBINK_LIB_DIR` is not set, the build script falls back to:

```text
bindings/python/mbink/bin
```

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

This compiles a temporary directory into a resource package, mounts it on the app, and loads `/index.html` from the package.

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

    let file = load_resource_file("assets.mbk", "index.html", "")?;
    let html = file.into_utf8_string()?;

    let app = App::new("MBink", 800, 600)?;
    app.mount_resource_package("assets.mbk", "", "/")?;
    app.load_html_file("/index.html")?;
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
- shared object read/write helpers
- `bind` / `bind_async` / `unbind`
- DevTools helpers:
  - `devtools_open`
  - `devtools_close`
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
