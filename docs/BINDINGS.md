# Bindings | 绑定

## Status | 状态

| Language | Status | Evidence | 说明 |
|---|---|---|---|
| Python | Integrated | `bindings/python/CMakeLists.txt`, `setup.py`, `mbink/_ffi.py`, Python package files | 已接入构建 |
| Go | Placeholder | Directory exists, no verified implementation | 占位目录 |
| Rust | Placeholder | Directory exists, no verified implementation | 占位目录 |
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

## Other Bindings | 其他绑定

Current repository state for Go, Rust, and Node.js | 当前仓库状态：

- placeholder directories exist
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



