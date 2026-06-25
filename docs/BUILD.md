# Build

English | [中文](BUILD.zh-CN.md)

This page documents the current public build surface for MBlink with Windows-first wording.

## Build system

MBlink uses CMake at the repository root.

Important top-level build surfaces already wired into `CMakeLists.txt` include:

- `core/`
- `bindings/python/`
- `tools/app_bundler/`
- `tools/esm_loader/`
- `tools/mblink_ui_dev/`
- `tests/` when enabled

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

The docs and examples in this repository mainly assume these Windows outputs:

- `build\bin\Release\mblink-ui-dev.exe`
- `build\bin\Release\esm_loader.exe`
- `build\bin\Release\mblink.dll`

Depending on what you build, you may also see:

- `build\bin\Release\mblink_devtools.dll`

For `mblink-ui-dev` project build/open workflows that use the `esbuild` builder, a usable `esbuild` binary must also be available on `PATH` or inside the project `node_modules`.

## CMake options

Common top-level options include:

- `MBLINK_BUILD_PYTHON_BINDING=ON`
- `MBLINK_BUILD_RUST_BINDING=OFF`
- `MBLINK_BUILD_GO_BINDING=OFF`
- `MBLINK_USE_SKIA=ON`
- `MBLINK_BUILD_TESTS=OFF`
- `MBLINK_ENABLE_LTO=OFF`

Treat those as current repository defaults, not long-term API promises.

## Tests

Enable tests explicitly:

```powershell
cmake -B build -DMBLINK_BUILD_TESTS=ON
cmake --build build --config Release
ctest --test-dir build --output-on-failure -C Release
```

Be careful with claims here: in the current active `build/` tree for this workspace, `ctest --test-dir build -N -C Release` reported zero discovered tests. So public docs should present test commands as available workflow, not as guaranteed passing proof for every local build directory.

## Tool-focused build

If your goal is the AI-first development loop, the most relevant target pair is:

```powershell
cmake --build build --config Release --target mblink_ui_dev esm_loader -- /m:1
```

That gives you:

- `mblink-ui-dev` for project open/build/snapshot/query/MCP
- `esm_loader` for the thinnest manual host path

## Runtime shape

The runtime model is:

- `mblink.dll` is the shared runtime
- `esm_loader.exe` is a thin manual host
- `mblink-ui-dev.exe` is the higher-level development tool surface

Read [C API Runtime Parity](C_API_RUNTIME_PARITY.md) for the contract behind that split.

## Python packaging note

The Python package loads the runtime through `ctypes`, and current build behavior copies runtime artifacts into:

- `bindings/python/mblink/bin/`

Read [../bindings/python/README.md](../bindings/python/README.md) for the practical Python path.

## Platform note

Windows currently has the strongest verified build and runtime evidence in this repository.

Other platforms may contain useful source structure or binding work, but they should still be described as needing verification unless you have fresh proof.
