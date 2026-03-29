# Python 绑定

本文档只描述当前仓库中可以从代码和构建脚本确认的 Python 绑定状态。

## 当前状态

Python 是当前唯一可以确认已接入构建系统的语言绑定。

可确认依据：
- 存在 `bindings/python/CMakeLists.txt`
- 存在 `bindings/python/src/bindings.cpp`
- 绑定实现使用 `pybind11`
- 存在 Python 包目录 `bindings/python/lightui/`
- 顶层 CMake 默认启用 `LIGHTUI_BUILD_PYTHON_BINDING`

## 当前目录组成

- `src/`：绑定实现源码
- `lightui/`：Python 包包装层
- `setup.py`：打包与安装入口

## 当前不应过度承诺的内容

目前不应直接宣称：
- 全平台稳定可用
- API 已完全定型
- 与仓库名 `MBink` 已完全统一

当前代码中仍大量使用 `LightUI` / `lightui` 命名。

## 基本使用

如需构建 Python 绑定，可先从项目根目录执行：

```bash
cmake -B build
cmake --build build --config Release
```

更详细的绑定状态说明见：
- `docs/BINDINGS.md`
- `docs/BUILD.md`