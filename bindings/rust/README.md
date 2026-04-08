# MBink Rust Bindings

Rust bindings for MBink live under this workspace:

- `mbink-sys`: raw FFI bindings to `mbink.h`
- `mbink`: safe Rust wrapper with RAII, `Result`, JSON helpers, and callbacks

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

## Current safe wrapper coverage

- App creation and window lifecycle
- HTML / JS loading and evaluation
- state read/write helpers
- shared object read/write helpers
- `bind` / `unbind`
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
