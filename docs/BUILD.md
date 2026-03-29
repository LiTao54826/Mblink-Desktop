# 构建说明

本文档只描述当前仓库中可从代码和构建脚本确认的构建方式。

## 构建系统

项目使用 CMake 作为主构建系统。

顶层当前可确认接入：
- `core/` 核心模块
- `bindings/python/` Python 绑定
- `tools/app_bundler/`
- `tools/esm_loader/`
- `tests/`（默认关闭）

从顶层 `CMakeLists.txt` 可见：
- `bindings/python` 由 `LIGHTUI_BUILD_PYTHON_BINDING` 控制
- `tests` 由 `LIGHTUI_BUILD_TESTS` 控制
- `tools/app_loader` 当前处于注释状态，不参与默认构建

## 主要 CMake 选项

- `LIGHTUI_BUILD_PYTHON_BINDING=ON`
- `LIGHTUI_BUILD_RUST_BINDING=OFF`
- `LIGHTUI_BUILD_GO_BINDING=OFF`
- `LIGHTUI_USE_SKIA=ON`
- `LIGHTUI_BUILD_TESTS=OFF`
- `LIGHTUI_ENABLE_LTO=OFF`

## 基本构建

```bash
cmake -B build
cmake --build build --config Release
```

## 构建测试

```bash
cmake -B build -DLIGHTUI_BUILD_TESTS=ON
cmake --build build --config Release
ctest --test-dir build --output-on-failure
```

## 构建 Python 绑定

默认情况下 Python binding 会被接入顶层构建。

它依赖：
- Python 3
- pybind11

单独关注的输出位置：
- `bindings/python/lightui/bin/`

从当前构建脚本还可以确认：
- `lightui_api` 会作为共享库构建
- 构建完成后会复制到 `bindings/python/lightui/bin/`
- Python 绑定会链接多组核心静态库，而不只是一个单独的入口目标

## 依赖说明

从代码可见，项目依赖或接入了以下关键组件：
- QuickJS
- SDL3
- Skia
- Lexbor
- nlohmann/json
- GoogleTest（测试）

其中部分第三方内容可能通过仓库内第三方目录或额外下载脚本提供。

## 当前限制

- 仓库当前对 Windows 的构建痕迹最明显
- 其他平台状态需要额外验证
- 第三方依赖整理尚未完成，开源收口中

