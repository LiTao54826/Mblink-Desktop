# Bindings

English | [中文](BINDINGS.zh-CN.md)

MBink is organized around one shared runtime and multiple host surfaces.

## The runtime model first

Before thinking in terms of language bindings, think in terms of runtime layers:

- `mbink.dll` is the shared runtime core
- `esm_loader.exe` is the clearest manual host in this repository
- `mbink-ui-dev.exe` is the project-development tool surface
- `mbink_devtools.dll` is a development-only companion for snapshot/control and runtime HTTP MCP

That means bindings are best understood as host adapters around the same runtime, not separate runtimes.

## Current binding status

| Binding | Current state | Notes |
|---|---|---|
| Python | Best public onboarding path after `esm_loader` | Integrated in top-level build and easiest to read |
| Rust | Real binding packages exist | Better treated as secondary onboarding today |
| Go | Real binding packages exist | Windows-oriented and still secondary onboarding |
| Node.js | Placeholder only | Do not describe as supported |

## Recommended order for new readers

1. `mbink-ui-dev`
2. `esm_loader`
3. Python
4. Rust or Go as needed

This order matches the parts of the repo that are easiest to understand and most grounded in current workflow evidence.

## Python

Python is the clearest language binding to read first because:

- it is integrated into the top-level CMake build
- it uses a direct `ctypes + C ABI` model
- the repository contains multiple runnable Python examples

Start here:

- [../bindings/python/README.md](../bindings/python/README.md)

Repository facts that matter:

- runtime artifacts are copied into `bindings/python/mbink/bin/`
- the package loads `mbink.dll` through `ctypes`
- UI-dev snapshot/control and HTTP MCP can be enabled through the optional devtools path

## Rust

Rust bindings live in:

- `bindings/rust/mbink-sys`
- `bindings/rust/mbink`

They are useful if you want:

- raw FFI and safe wrapper layers
- explicit dynamic library control
- a more idiomatic host integration than Python

Start with:

- [../bindings/rust/README.md](../bindings/rust/README.md)

## Go

Go bindings live in:

- `bindings/go/mbink`

They are currently Windows-oriented and are a reasonable option if you want:

- cgo-based host integration
- a direct native host model

Start with:

- [../bindings/go/README.md](../bindings/go/README.md)

## Devtools and parity

Across Python, Rust, and Go, the important development-only rule is the same:

- `mbink_devtools.dll` should be loaded on demand
- it should not be treated as the production runtime
- observable runtime behavior should stay aligned with the shared C API surface

Read [C API Runtime Parity](C_API_RUNTIME_PARITY.md) for the contract this repository is aiming to preserve.

## What not to claim yet

Do not overstate the binding story:

- Node.js is not a verified supported binding
- cross-platform parity should not be implied without fresh evidence
- the presence of source directories is not the same as stable public support
