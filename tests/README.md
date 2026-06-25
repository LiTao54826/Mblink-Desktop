# Tests | 测试目录

## Overview | 概览

This directory documents test materials that can be confirmed from the current repository structure and `tests/CMakeLists.txt`.
本文档只描述当前仓库结构和 `tests/CMakeLists.txt` 中可确认的测试信息。

## Layout | 目录结构

Current test-related directories include:
当前测试相关目录包括：

- `unit/`
- `render/`
- `integration/`
- `performance/`
- `property/`
- `js/`
- `style_tests/`
- `regression/`
- `test_utils/`

## CMake Test Targets | CMake 测试目标

The following targets can be confirmed from `tests/CMakeLists.txt`:
从 `tests/CMakeLists.txt` 可确认以下目标：

- `mblink_unit_tests`
- `mblink_render_tests`
- `mblink_integration_tests`
- `mblink_property_tests`
- `mblink_performance_tests` (optional / 可选)

## Confirmed Facts | 当前可确认事实

- the repository contains a relatively complete test hierarchy
  仓库中存在较完整的测试分层
- C++ test targets are wired into the main build system
  C++ 测试目标已接入主构建系统
- additional JS, HTML, style, and regression assets are also present
  还存在 JS、HTML、style 与 regression 测试材料

## Non-Claims | 不应直接承诺的内容

The current repository state does not justify claims that:
当前仓库状态不足以直接宣称：

- all tests pass in the current revision / 当前版本所有测试均通过
- all tests are fully maintained / 所有测试都在持续维护
- all platforms have been validated / 所有平台都已完成验证

## Recommended Usage | 建议使用方式

If you need to validate the current repository state, start with:
如果你要验证当前仓库状态，建议先执行：

```bash
cmake -B build -DMBLINK_BUILD_TESTS=ON
cmake --build build --config Release
ctest --test-dir build --output-on-failure
```

See also | 另见：

- `docs/TESTING.md`