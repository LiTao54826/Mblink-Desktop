# Build

[English](BUILD.md) | 中文

这份文档描述 MBlink 当前对外可讲清楚的构建面，表述以 Windows-first 为主。

## 构建系统

MBlink 在仓库根目录使用 CMake。

当前已经接入顶层 `CMakeLists.txt` 的主要构建面包括：

- `core/`
- `bindings/python/`
- `tools/app_bundler/`
- `tools/esm_loader/`
- `tools/mblink_ui_dev/`
- 在启用时包含 `tests/`

## 常用命令

配置：

```powershell
cmake -B build
```

构建公开上手最关键的两个工具：

```powershell
cmake --build build --config Release --target mblink_ui_dev esm_loader -- /m:1
```

构建更完整的默认 Release 树：

```powershell
cmake --build build --config Release
```

## 关键输出

当前文档和示例主要假定你会拿到这些 Windows 输出：

- `build\bin\Release\mblink-ui-dev.exe`
- `build\bin\Release\esm_loader.exe`
- `build\bin\Release\mblink.dll`

根据构建目标不同，你还可能看到：

- `build\bin\Release\mblink_devtools.dll`

如果 `mblink-ui-dev` 的项目 build/open 工作流使用的是 `esbuild` builder，那么系统 `PATH` 或项目 `node_modules` 里还需要有可用的 `esbuild`。

## CMake 选项

常见顶层选项包括：

- `MBLINK_BUILD_PYTHON_BINDING=ON`
- `MBLINK_BUILD_RUST_BINDING=OFF`
- `MBLINK_BUILD_GO_BINDING=OFF`
- `MBLINK_USE_SKIA=ON`
- `MBLINK_BUILD_TESTS=OFF`
- `MBLINK_ENABLE_LTO=OFF`

请把它们理解成当前仓库默认值，而不是长期稳定 API 承诺。

## 测试

如果要显式启用测试：

```powershell
cmake -B build -DMBLINK_BUILD_TESTS=ON
cmake --build build --config Release
ctest --test-dir build --output-on-failure -C Release
```

这里不要过度表述：在当前这个工作区的活动 `build/` 目录里，`ctest --test-dir build -N -C Release` 之前返回过零个已发现测试。所以对外文档应把测试命令描述为“可用工作流”，而不是“每个本地 build 目录都已经验证通过”的证明。

## 以工具链为中心的构建

如果你的目标是 AI 优先开发循环，那么最值得关心的目标组合是：

```powershell
cmake --build build --config Release --target mblink_ui_dev esm_loader -- /m:1
```

它会给你：

- `mblink-ui-dev`：用于项目 open/build/snapshot/query/MCP
- `esm_loader`：用于最薄的手动宿主路径

## 运行时形态

运行时模型可以概括为：

- `mblink.dll` 是共享运行时
- `esm_loader.exe` 是薄宿主
- `mblink-ui-dev.exe` 是更高层的开发工具面

这三者之间的关系，请继续阅读 [C API Runtime Parity](C_API_RUNTIME_PARITY.md)。

## Python 打包说明

Python 包通过 `ctypes` 加载运行时，当前构建行为会把运行时产物复制到：

- `bindings/python/mblink/bin/`

更实际的 Python 路径请看 [../bindings/python/README.md](../bindings/python/README.md)。

## 平台说明

Windows 是当前仓库中构建和运行时证据最充分的平台。

其他平台也许已经有部分源码结构或绑定工作，但如果没有新鲜验证证据，就应该继续把它们描述为“待验证”，而不是“已支持”。
