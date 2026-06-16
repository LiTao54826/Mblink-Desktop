# MBink Go Binding

Windows-oriented Go binding for MBink, implemented with `cgo + core/api/mbink.h + mbink.lib`.
Runtime behavior follows the shared C API contract documented in `docs/C_API_RUNTIME_PARITY.md`.

## Requirements

- Windows
- Go 1.23+
- built native library available at `bindings/go/mbink.lib`
  - CMake now copies it there automatically after building `mbink_api`
- matching runtime DLL available for the process loader
  - CMake now also copies `mbink.dll` to `bindings/go/`

## Quick check

From `bindings/go/`:

```bash
go test ./...
```

## Run examples

From `bindings/go/`:

```bash
go run ./examples/hello
go run ./examples/bind
go run ./examples/events
go run ./examples/shared
go run ./examples/state
go run ./examples/window
go run ./examples/tray
go run ./examples/resources_read
go run ./examples/resources_mount
go run ./examples/native_controls
```

## Example summary

- `hello`: create a window and load inline HTML
- `bind`: expose `backend.greet()` to JavaScript
- `events`: register resize / focus / blur / close callbacks
- `shared`: populate shared object data from Go
- `state`: initialize window state from Go
- `window`: demonstrate position / size / DevTools helpers
- `tray`: hide to tray, restore on click, handle tray menu
- `resources_read`: compile and read back a resource package
- `resources_mount`: mount a compiled package and load `/index.html`
- `native_controls`: initialize `LogView` and `Terminal` from Go

## Minimal usage

```go
package main

import (
    "log"

    "mbink-go/mbink"
)

func main() {
    app, err := mbink.New("MBink Go Hello", 800, 600)
    if err != nil {
        log.Fatal(err)
    }
    defer app.Close()

    if err := app.LoadHTML(`<!doctype html><html><body><h1>Hello from Go</h1></body></html>`); err != nil {
        log.Fatal(err)
    }

    app.Run()
}
```

## Package surface

Current package includes:

- `App` / `AppHandle`
- `Config`
- `State`
- `Shared`
- `LogView`
- `Terminal`
- resource helpers: `CompileResources`, `LoadResourceFile`
- callback/event helpers: `Bind`, `BindAsync`, tray callbacks, window event callbacks, state watch

## Notes

- current implementation is Windows-oriented
- non-Windows builds use unsupported stubs
- examples are also marked with `//go:build windows`
- build-time linking now prefers `bindings/go`, with `build/lib/Release` kept as fallback
- runtime DLL loading is handled by the Windows loader, not by a custom Go `_find_dll()` helper
- CMake copies `mbink.dll` and `mbink.lib` directly to `bindings/go/`
- this lets `go run ./examples/...` started from `bindings/go/` find the DLL from the working directory more directly
- for other executables, `mbink.dll` still needs to be reachable at run time, typically via the executable directory or `PATH`
- wrapper runtime, lifecycle, observation, snapshot, and UI-dev command methods should stay aligned with `docs/C_API_RUNTIME_PARITY.md`
