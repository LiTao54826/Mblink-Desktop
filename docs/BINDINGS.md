# 绑定状态

本文档只记录当前仓库中能从目录、源码和构建脚本确认的绑定状态。

## 当前状态总览

| 语言 | 状态 | 依据 |
|---|---|---|
| Python | 已接入构建 | 存在 `bindings/python/CMakeLists.txt`、`setup.py`、`src/bindings.cpp`、Python 包目录 |
| Go | 未实现 / 占位 | 目录存在，当前扫描结果为空目录 |
| Rust | 未实现 / 占位 | 目录存在，当前扫描结果为空目录 |
| Node.js | 未实现 / 占位 | 目录存在，当前扫描结果为空目录 |

## Python 绑定

Python 绑定当前是唯一可以确认有真实实现的绑定层。

可确认内容：
- 使用 `pybind11` 构建扩展模块
- 存在 `bindings/python/src/bindings.cpp`
- 存在 Python 包包装层：
  - `lightui/__init__.py`
  - `lightui/app.py`
  - `lightui/shared.py`
  - `lightui/_ffi.py`
- 顶层 CMake 默认启用 Python binding

当前绑定形态应理解为：
- 以 `pybind11` 扩展模块为主
- 同时保留部分 Python 层封装与 FFI 辅助代码

## Python 绑定的当前问题

- 命名仍以 `LightUI` / `lightui` 为主
- 与仓库名 `MBink` 还未完全统一
- 版本信息和仓库品牌信息仍需统一整理

## Go / Rust / Node.js

当前不应在 README 或对外文档中描述为“已支持”。

更准确的表述是：
- 目录已预留
- 尚未形成可确认的实现与构建接入

## 后续整理原则

开源文档中只保留以下表述：
- Python：已有实现，仍需收口和验证
- Go / Rust / Node.js：预留目录，暂未提供可用绑定

