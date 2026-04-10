# Bindings | 绑定

## Status | 状态

| Language | Status | Evidence | 说明 |
|---|---|---|---|
| Python | Integrated | `bindings/python/CMakeLists.txt`, `setup.py`, `mbink/_ffi.py`, Python package files | 已接入构建 |
| Go | Implemented | `bindings/go/go.mod`, `bindings/go/mbink/*.go`, `go test ./...` | 已实现 Go 绑定 |
| Rust | Implemented | `bindings/rust/README.md`, `bindings/rust/mbink`, `bindings/rust/mbink-sys` | 已实现 Rust 绑定 |
| Node.js | Placeholder | Directory exists, no verified implementation | 占位目录 |

## Python Binding | Python 绑定

Current verified facts | 当前可确认事实：

- built on `ctypes + C ABI`
  基于 `ctypes + C ABI`
- runtime library target: `core/api/mbink_api`
  运行时目标：`core/api/mbink_api`
- Python package layer exists
  存在 Python 包装层
- enabled in the top-level CMake build
  已接入顶层 CMake 构建

Python binding currently consists of | 当前形态包括：

- a shared runtime library loaded through `ctypes`
  一个通过 `ctypes` 加载的共享运行时库
- Python wrapper and FFI helper code
  Python 包装层与 FFI 辅助代码

## Go Binding | Go 绑定

Current verified facts | 当前可确认事实：

- Go package exists at `bindings/go/mbink`
  Go 包位于 `bindings/go/mbink`
- implemented with `cgo + core/api/mbink.h + bindings/go/mbink.lib`
  基于 `cgo + core/api/mbink.h + bindings/go/mbink.lib` 实现
- current implementation is Windows-oriented
  当前实现面向 Windows
- package exposes `App`, `State`, `Shared`, `LogView`, `Terminal`, resource helpers, and callback/event bindings
  提供 `App`、`State`、`Shared`、`LogView`、`Terminal`、资源辅助与回调/事件绑定
- verified by `go test ./...`
  已通过 `go test ./...` 验证
- examples available under `bindings/go/examples`
  示例位于 `bindings/go/examples`
- usage and run instructions documented in `bindings/go/README.md`
  使用与运行方式见 `bindings/go/README.md`

## Rust Binding | Rust 绑定

Current verified facts | 当前可确认事实：

- Rust binding package exists at `bindings/rust/mbink`
  Rust 绑定包位于 `bindings/rust/mbink`
- raw FFI layer exists at `bindings/rust/mbink-sys`
  原始 FFI 层位于 `bindings/rust/mbink-sys`
- repository contains README and runnable examples for Rust bindings
  仓库中包含 Rust 绑定 README 与可运行示例

## Other Bindings | 其他绑定

Current repository state for Node.js | 当前 Node.js 仓库状态：

- placeholder directory exists
  存在占位目录
- no verified implementation should be claimed
  不应宣称已有可验证实现
- no supported status should be documented
  不应写成已支持状态

## Notes | 说明

- naming still contains `MBink` / `mbink` compatibility layers
  命名仍保留 `MBink` / `mbink` 兼容层
- binding naming and versioning still need cleanup
  绑定命名和版本信息仍需整理



