# MBink

MBink 是一个基于 C++ 的桌面 UI / 应用框架仓库，当前代码中可确认接入了 QuickJS、SDL3、Skia、Lexbor，以及一套自有的 DOM、事件、布局、渲染、合成与工具链模块。

> 当前仓库仍处于开源整理阶段。以下说明以源码、CMake、示例和测试结构为准，不沿用旧阶段文档中的完成度口径。

## 当前可以确认的内容

- 使用 `CMake` 作为主构建系统
- `core/` 下已接入 window、dom、event、layout、render、quickjs、network、devtools、compositor 等模块
- 存在统一 C ABI：`core/api/lightui.h`
- 顶层当前只接入 `bindings/python`
- 存在真实工具目标：`app_bundler`、`esm_loader`
- `app_loader` 当前未接入顶层构建
- 存在分层测试工程：unit / render / integration / property / performance
- 仓库内有较多示例与实验性 demo

## 当前不能直接承诺的内容

- Rust / Go / Node.js 绑定可用（当前仅能确认这些目录为预留空目录）
- 所有测试稳定通过
- 所有平台均已验证
- 项目命名已经完全从 `LightUI` 迁移到 `MBink`

## 仓库现状说明

当前代码和构建系统中仍大量使用 `LightUI` / `lightui` 命名，例如：
- CMake 项目名
- target 名称
- C API 命名
- Python 包名

这表示仓库仍处于品牌和对外接口整理阶段。

## 目录概览

```text
core/        核心模块
bindings/    语言绑定（当前仅 Python 可确认有实现）
examples/    示例与实验 demo
tests/       测试工程
tools/       工具链目标
scripts/     下载、测试、辅助脚本
docs/        开源整理后的文档
```

## 构建

基础构建：

```bash
cmake -B build
cmake --build build --config Release
```

启用测试：

```bash
cmake -B build -DLIGHTUI_BUILD_TESTS=ON
cmake --build build --config Release
ctest --test-dir build --output-on-failure
```

更多说明见：
- [docs/BUILD.md](docs/BUILD.md)
- [docs/TESTING.md](docs/TESTING.md)

## 绑定状态

- Python：已接入构建，存在真实源码与包结构
- Go：目录预留，当前为空
- Rust：目录预留，当前为空
- Node.js：目录预留，当前为空

更多说明见：[docs/BINDINGS.md](docs/BINDINGS.md)

## 架构与限制

- 架构概览见：[docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)
- 当前限制见：[docs/KNOWN_LIMITATIONS.md](docs/KNOWN_LIMITATIONS.md)
- 贡献方式见：[docs/CONTRIBUTING.md](docs/CONTRIBUTING.md)

## 开源整理原则

当前文档遵循以下原则：
- 只写代码、构建脚本、示例、测试能证明的事实
- 不再使用旧文档中的阶段进度、完成率和宣传性口径
- 逐步清理历史计划稿、总结稿和构建产物