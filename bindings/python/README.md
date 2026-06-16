# Python Binding | Python 绑定

## Overview | 概览

This page describes the Python binding state that can be confirmed from the current repository and build scripts.
本文档只描述当前仓库和构建脚本中可确认的 Python 绑定状态。

## Current Status | 当前状态

Python is the only binding with repository-visible implementation and top-level build integration.
Python 是当前唯一同时具备可见实现和顶层构建接入的绑定。

Evidence | 可确认依据：

- `bindings/python/CMakeLists.txt`
- `bindings/python/mbink/_ffi.py`
- `bindings/python/mbink/app.py`
- Python package directory under `bindings/python/`
- top-level CMake enables `MBINK_BUILD_PYTHON_BINDING`

## Directory Layout | 目录结构

- `mbink/` — ctypes package entry / Python 包入口
- `setup.py` — packaging entry / 打包入口

## Non-Claims | 不应直接承诺的内容

The current repository state does not justify claims that:
当前仓库状态不足以直接宣称：

- all platforms are fully validated / 所有平台都已完整验证
- the API is fully stabilized / API 已完全稳定
- naming is fully unified with repository branding / 命名已和仓库品牌完全统一

## Build | 构建

```bash
cmake -B build
cmake --build build --config Release
```

See also | 另见：

- `docs/BINDINGS.md`
- `docs/BUILD.md`
- `docs/C_API_RUNTIME_PARITY.md`
