# MBlink Python Binding

The Python binding is the easiest language binding to understand after `mblink-ui-dev` and `esm_loader`.

## What it is

- a `ctypes + C ABI` wrapper around `mblink.dll`
- a good reference if you want to see MBlink as a host runtime instead of only as a dev tool

## Current repository shape

Important files and folders:

- `bindings/python/mblink/`
- `bindings/python/setup.py`
- `bindings/python/examples/`
- `bindings/python/mblink/bin/mblink.dll`

## Build/runtime relationship

After building the repository, runtime artifacts are copied into:

- `bindings/python/mblink/bin/`

That means the Python package can load the local MBlink runtime without inventing a separate runtime story.

## Basic build path

From the repository root:

```powershell
cmake -B build
cmake --build build --config Release --target mblink_ui_dev esm_loader -- /m:1
```

If you specifically want the Python-integrated build surface as well:

```powershell
cmake --build build --config Release
```

## Install shape

From `bindings/python/`:

```powershell
py -3 -m pip install -e .
```

`py -3` is the safest command shape to use in this Windows workspace.

## Smallest way to read the API

Look at:

- `mblink/__init__.py`
- `mblink/app.py`

The main host entry point is `App`.

## Example path

One practical example is:

- `bindings/python/examples/todo_app/main.py`

From `bindings/python/examples/todo_app/` you can run:

```powershell
py -3 main.py
```

That example is useful because it shows:

- `App(...)`
- shared state
- Python-to-JS data updates
- JS-to-Python calls through `@app.bind(...)`

## Devtools and AI-adjacent helpers

The Python binding can also expose the same development-only inspection path used elsewhere in the repository:

- `App.ui_dev_snapshot(...)`
- `App.ui_dev_command(...)`
- `App.devtools_http_session(...)`

Those APIs depend on the optional `mblink_devtools.dll` path rather than changing the core runtime model.

## Parity expectation

Python, Rust, and Go bindings are release-gated against the same C API runtime
contract. If you add a parity-critical C API surface here, update the Rust and
Go bindings or record the gap before release.

## What this README does not claim

- not all platforms are equally verified
- the API should not yet be described as fully stabilized
- Python is one supported binding, not a separate runtime from Rust or Go

## Related docs

- [../../docs/BINDINGS.md](../../docs/BINDINGS.md)
- [../../docs/BUILD.md](../../docs/BUILD.md)
- [../../docs/C_API_RUNTIME_PARITY.md](../../docs/C_API_RUNTIME_PARITY.md)
