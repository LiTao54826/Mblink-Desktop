# Bindings

English | [中文](BINDINGS.zh-CN.md)

MBlink is organized around one shared runtime and multiple host surfaces.

## The runtime model first

Before thinking in terms of language bindings, think in terms of runtime layers:

- `mblink.dll` is the shared runtime core
- `esm_loader.exe` is the clearest manual host in this repository
- `mblink-ui-dev.exe` is the project-development tool surface
- `mblink_devtools.dll` is a development-only companion for snapshot/control and runtime HTTP MCP

That means bindings are best understood as host adapters around the same runtime, not separate runtimes.

## Current binding status

| Binding | Current state | Notes |
|---|---|---|
| Python | Supported host binding | `ctypes + C ABI`; runtime copied into `bindings/python/mblink/bin/` |
| Rust | Supported host binding | Raw FFI plus safe wrapper; runtime copied into `bindings/rust/mblink-sys/runtime/` |
| Go | Supported host binding on Windows with cgo | `cgo + C ABI`; requires `CGO_ENABLED=1`, a C compiler, `mblink.dll`, and `mblink.lib` |
| Node.js | Placeholder only | Do not describe as supported |

All supported bindings must track the same observable runtime contract. If a C
API behavior is exposed in Python, release readiness should either expose it in
Rust and Go as well, or explicitly document and gate the gap before release.

## Recommended order for new readers

1. `mblink-ui-dev`
2. `esm_loader`
3. Python, Rust, or Go depending on the host language you plan to use

This order starts with the tooling and manual runtime first, then moves into the
language host you actually need.

## Python

Python uses the direct `ctypes + C ABI` path and is useful if you want a
scriptable host with minimal native-language setup.

Repository facts that matter:

- runtime artifacts are copied into `bindings/python/mblink/bin/`
- the package loads `mblink.dll` through `ctypes`
- the repository contains multiple runnable Python examples

- [../bindings/python/README.md](../bindings/python/README.md)

- UI-dev snapshot/control and HTTP MCP can be enabled through the optional devtools path

## Rust

Rust bindings live in:

- `bindings/rust/mblink-sys`
- `bindings/rust/mblink`

They are useful if you want:

- raw FFI and safe wrapper layers
- explicit dynamic library control
- an idiomatic Rust host integration over the same C API contract
- compile-time checks through `cargo check -p mblink-sys -p mblink`

Start with:

- [../bindings/rust/README.md](../bindings/rust/README.md)

## Go

Go bindings live in:

- `bindings/go/mblink`

They are Windows-oriented and are a supported option when you want:

- cgo-based host integration
- a direct native host model

The Go binding requires `CGO_ENABLED=1`, a C compiler on `PATH`, and the local
`mblink.dll` / `mblink.lib` copied by the native build. With cgo disabled, the
package intentionally exposes an unsupported stub instead of silently compiling a
partial binding.

Start with:

- [../bindings/go/README.md](../bindings/go/README.md)

## Devtools and parity

Across Python, Rust, and Go, the important development-only rule is the same:

- `mblink_devtools.dll` should be loaded on demand
- it should not be treated as the production runtime
- observable runtime behavior must stay aligned with the shared C API surface

Read [C API Runtime Parity](C_API_RUNTIME_PARITY.md) for the contract this repository is aiming to preserve.

## What not to claim yet

Do not overstate the binding story:

- Node.js is not a verified supported binding
- cross-platform parity should not be implied without fresh evidence
- Windows cgo is required for Go binding verification
