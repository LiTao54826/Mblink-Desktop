# Build | 构建

## Build System | 构建系统

- Primary build system: `CMake`
  主构建系统：`CMake`
- Top-level modules currently wired into the build:
  当前已接入顶层构建的模块：
  - `core/`
  - `bindings/python/`
  - `tools/app_bundler/`
  - `tools/esm_loader/`
  - `tests/` (disabled by default / 默认关闭)
- Not enabled in the default top-level build:
  默认未接入顶层构建：
  - `tools/app_loader/`

## CMake Options | CMake 选项

- `MBINK_BUILD_PYTHON_BINDING=ON`
- `MBINK_BUILD_RUST_BINDING=OFF`
- `MBINK_BUILD_GO_BINDING=OFF`
- `MBINK_USE_SKIA=ON`
- `MBINK_BUILD_TESTS=OFF`
- `MBINK_ENABLE_LTO=OFF`

## Configure and Build | 配置与构建

```bash
cmake -B build
cmake --build build --config Release
```

## Build with Tests | 构建并运行测试

```bash
cmake -B build -DMBINK_BUILD_TESTS=ON
cmake --build build --config Release
ctest --test-dir build --output-on-failure
```

## Python Binding Build | Python 绑定构建

Dependencies | 依赖：

- Python 3

Current build behavior | 当前构建行为：

- `mbink_api` is built as a shared library
  `mbink_api` 以共享库形式构建
- build outputs are copied to `bindings/python/mbink/bin/`
  构建产物会复制到 `bindings/python/mbink/bin/`
- Python package loads the runtime through `ctypes`
  Python 包通过 `ctypes` 加载运行时库

## Third-Party Dependencies | 第三方依赖

- QuickJS
- SDL3
- Skia
- Lexbor
- nlohmann/json
- GoogleTest

## Limitations | 限制

- Windows has the most complete build evidence in the repository
  仓库中 Windows 构建痕迹最完整
- Other platforms still require verification
  其他平台仍需验证
- Presence of a directory does not imply supported platform or binding status
  目录存在不代表平台或绑定已受支持


