# Bindings

[English](BINDINGS.md) | 中文

MBink 的结构核心是“一个共享运行时 + 多个宿主表面”。

## 先理解运行时模型

与其先按语言来理解，不如先按运行时层次来理解：

- `mbink.dll`：共享运行时核心
- `esm_loader.exe`：仓库里最容易直观理解的手动宿主
- `mbink-ui-dev.exe`：项目开发工具层
- `mbink_devtools.dll`：仅开发期使用的快照/控制与运行时 HTTP MCP 伴随组件

所以更合适的理解方式是：各种绑定都是围绕同一运行时的宿主适配层，而不是各自独立的一套运行时。

## 当前绑定状态

| 绑定 | 当前状态 | 说明 |
|---|---|---|
| Python | 除 `esm_loader` 外最适合作为公开上手路径 | 已接入顶层构建，也最容易读懂 |
| Rust | 已存在真实绑定包 | 当前更适合作为次级上手路径 |
| Go | 已存在真实绑定包 | 偏 Windows，仍属次级上手路径 |
| Node.js | 仅占位 | 不应描述为已支持 |

## 推荐阅读顺序

1. `mbink-ui-dev`
2. `esm_loader`
3. Python
4. Rust 或 Go（按需）

这个顺序更符合当前仓库里“最容易理解、也最有工作流证据”的部分。

## Python

Python 是当前最适合先读的语言绑定，原因包括：

- 它已经接入顶层 CMake 构建
- 它采用直接的 `ctypes + C ABI` 模型
- 仓库里有多份可运行 Python 示例

从这里开始：

- [../bindings/python/README.md](../bindings/python/README.md)

一些重要仓库事实：

- 运行时产物会复制到 `bindings/python/mbink/bin/`
- Python 包通过 `ctypes` 加载 `mbink.dll`
- UI-dev 快照/控制和 HTTP MCP 可以通过可选 devtools 路径启用

## Rust

Rust 绑定位于：

- `bindings/rust/mbink-sys`
- `bindings/rust/mbink`

当你需要下面这些特性时，它会比较有价值：

- 原始 FFI 与安全包装层
- 更显式的动态库控制
- 比 Python 更偏 idiomatic 的宿主集成方式

从这里开始：

- [../bindings/rust/README.md](../bindings/rust/README.md)

## Go

Go 绑定位于：

- `bindings/go/mbink`

在下面这些场景下它是一个合理选择：

- 需要基于 cgo 的宿主集成
- 需要更直接的原生宿主模型

从这里开始：

- [../bindings/go/README.md](../bindings/go/README.md)

## Devtools 与一致性

对 Python、Rust、Go 而言，有一条共同的开发期规则：

- `mbink_devtools.dll` 需要按需加载
- 它不应被视为生产运行时
- 可观察到的运行时行为应尽量和共享 C API 表面保持一致

这套约束的目标，请看 [C API Runtime Parity](C_API_RUNTIME_PARITY.md)。

## 当前不要宣称的内容

绑定层面暂时不要过度表述：

- Node.js 不是已验证、已支持的绑定
- 没有新鲜证据时，不应暗示跨平台一致性
- 某个源码目录存在，不等于这个绑定已经适合公开稳定支持
