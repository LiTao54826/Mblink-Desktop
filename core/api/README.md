# API Module | API 模块

## Overview | 概览

`core/api/` provides the unified C ABI entry visible in the current repository.
`core/api/` 提供当前仓库中可见的统一 C ABI 入口。

## Files | 文件

- `mbink.h` — public header / 公开头文件
- `mbink.cpp` — implementation / 实现文件
- `CMakeLists.txt` — build integration / 构建接入

## Role | 作用

This layer is responsible for:
这一层负责：

- exposing core capabilities through a C-style entry surface
  通过 C 风格入口暴露核心能力
- serving as a stable boundary for bindings and hosts
  作为绑定层和宿主环境的边界
- reducing direct coupling to internal C++ types
  降低外部对内部 C++ 类型的直接耦合

## Repository-Visible Exports | 从仓库可见的导出

From `core/api/mbink.cpp`, at least these exports are visible:
从 `core/api/mbink.cpp` 至少可见这些导出函数：

- `mbink_init()`
- `mbink_cleanup()`
- `mbink_version()`
- `mbink_create()`
- `mbink_create_ex()`
- `mbink_default_config()`
- `mbink_destroy()`
- `mbink_run()`
- `mbink_stop()`

## Limitations | 限制

- exact API shape must follow `mbink.h` and `mbink.cpp`
  具体 API 形态应以 `mbink.h` 与 `mbink.cpp` 为准
- this document should not imply verified Rust / Go / Node.js support
  本文档不应暗示 Rust / Go / Node.js 已验证可用
- naming is not yet fully unified
  命名尚未完全统一

## Related Docs | 相关文档

- `docs/ARCHITECTURE.md`
- `docs/BINDINGS.md`
- `docs/KNOWN_LIMITATIONS.md`