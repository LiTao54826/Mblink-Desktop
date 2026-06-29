# Build

English | [Chinese](BUILD.zh-CN.md)

This page documents the current public build surface for MBlink. It is
intentionally Windows-first and avoids claiming unverified platform support.

## Build system

MBlink uses CMake at the repository root.

Important top-level build surfaces include:

- `core/`
- `bindings/python/`
- `tools/app_bundler/`
- `tools/esm_loader/`
- `tools/mblink_ui_dev/`
- `tests/` when enabled

## Dependency preparation

Prepare the fetchable source dependencies:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\download_deps.ps1 -NonInteractive
```

The script currently prepares:

- QuickJS-ng at a pinned commit under `third_party\quickjs`
- SDL3 at a pinned commit under `third_party\SDL3`

It does not download Skia. The default native build still expects prepared Skia
libraries under:

```text
third_party\skia\Debug\out\Debug-windows-x64\skia.lib
third_party\skia\Release\out\Release-windows-x64\skia.lib
```

`MBLINK_USE_SKIA=OFF` is not a supported public smoke path yet. Some runtime
modules still include or link Skia directly, so public onboarding should keep the
Skia requirement explicit until that configuration has fresh verification.

## Common commands

Configure:

```powershell
cmake -B build
```

Build the main tools used by public onboarding:

```powershell
cmake --build build --config Release --target mblink_ui_dev esm_loader -- /m:1
```

Build a broader default Release tree:

```powershell
cmake --build build --config Release
```

## Key outputs

The docs and examples mainly assume these Windows outputs:

- `build\bin\Release\mblink-ui-dev.exe`
- `build\bin\Release\esm_loader.exe`
- `build\bin\Release\mblink.dll`

Depending on what you build, you may also see:

- `build\bin\Release\mblink_devtools.dll`

For `mblink-ui-dev` project build/open workflows that use the `esbuild` builder,
a usable `esbuild` binary must also be available on `PATH` or inside the project
`node_modules`.

## CMake options

Common top-level options include:

- `MBLINK_BUILD_PYTHON_BINDING=ON`
- `MBLINK_BUILD_RUST_BINDING=OFF`
- `MBLINK_BUILD_GO_BINDING=OFF`
- `MBLINK_USE_SKIA=ON`
- `MBLINK_BUILD_TESTS=OFF`
- `MBLINK_ENABLE_LTO=OFF`

Treat these as current repository defaults, not long-term API promises.

## Tests

Enable tests explicitly:

```powershell
cmake -B build -DMBLINK_BUILD_TESTS=ON
cmake --build build --config Release
ctest --test-dir build --output-on-failure -C Release
```

Some local build trees may discover zero CTest tests depending on configuration.
Use test output as local evidence, not as a blanket claim that every checkout has
the same test matrix.

## Repository readiness checks

The public CI currently focuses on repository hygiene that can run from a fresh
Windows checkout without private Skia artifacts:

```powershell
git diff --check
py -3 scripts/check_code_structure_baseline.py
py -3 scripts/check_circular_deps.py
```

It also parses `scripts/download_deps.ps1` and verifies the public entry files.

The full code structure checker is still useful, but it is advisory today
because the current source tree has known pre-existing long-file debt:

```powershell
py -3 scripts/check_code_structure.py
```

Expected current status:

```text
fails with known source-size findings outside docs/support changes
```

The baseline checker is the hard gate: it allows the current oversized source
files listed in `scripts/code_structure_baseline.json` across the configured
root directories, fails if a new source file crosses the 2000-line threshold,
and fails if the manifest becomes stale.

The full native build remains a release gate until the Skia dependency story is
published with reproducible provenance.

## Runtime shape

- `mblink.dll` is the shared runtime
- `esm_loader.exe` is a thin manual host
- `mblink-ui-dev.exe` is the higher-level development tool surface
- `mblink_devtools.dll` is a development companion for snapshots/control

Read [C API Runtime Parity](C_API_RUNTIME_PARITY.md) for the contract behind that
split.

## Python packaging note

The Python package loads the runtime through `ctypes`, and current build behavior
copies runtime artifacts into:

```text
bindings/python/mblink/bin/
```

Read [../bindings/python/README.md](../bindings/python/README.md) for the
practical Python path.
