# MBlink Go Binding

Windows-oriented Go binding for MBlink, implemented with `cgo + core/api/mblink.h + mblink.lib`.
Runtime behavior follows the shared C API contract documented in `docs/C_API_RUNTIME_PARITY.md`.

## Requirements

- Windows
- Go 1.23+
- `CGO_ENABLED=1`
- a C compiler on `PATH`
- built native library available at `bindings/go/mblink.lib`
  - CMake now copies it there automatically after building `mblink_api`
- matching runtime DLL available for the process loader
  - CMake now also copies `mblink.dll` to `bindings/go/`

## Quick check

From `bindings/go/`:

```bash
set CGO_ENABLED=1
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

    "mblink-go/mblink"
)

func main() {
    app, err := mblink.New("MBlink Go Hello", 800, 600)
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
- Windows builds with `CGO_ENABLED=0` also use unsupported stubs
- examples are also marked with `//go:build windows`
- build-time linking now prefers `bindings/go`, with `build/lib/Release` kept as fallback
- runtime DLL loading is handled by the Windows loader, not by a custom Go `_find_dll()` helper
- CMake copies `mblink.dll` and `mblink.lib` directly to `bindings/go/`
- this lets `go run ./examples/...` started from `bindings/go/` find the DLL from the working directory more directly
- for other executables, `mblink.dll` still needs to be reachable at run time, typically via the executable directory or `PATH`
- development-only runtime UI analysis can be enabled with `App.EnableDevtoolsHttp`; the returned session exposes `URL`, `Port`, `AuthToken`, and `Request`
- Go binding packages do not vendor `mblink_devtools.dll`; development tooling should provide it next to `mblink.dll` or set `MBLINK_DEVTOOLS_PATH`, and the Go devtools wrapper loads it on demand
- wrapper runtime, lifecycle, observation, snapshot, and UI-dev command methods should stay aligned with `docs/C_API_RUNTIME_PARITY.md`
