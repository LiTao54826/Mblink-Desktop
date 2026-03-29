# 测试目录说明

本文档只描述当前仓库中可以从目录结构和 CMake 配置确认的测试状态。

## 当前结构

测试目录当前包括：
- `unit/`
- `render/`
- `integration/`
- `performance/`
- `property/`
- `js/`
- `style_tests/`
- `regression/`
- `test_utils/`

## 已接入的 CMake 测试目标

从 `tests/CMakeLists.txt` 可确认：
- `mbink_unit_tests`
- `mbink_render_tests`
- `mbink_integration_tests`
- `mbink_property_tests`
- `mbink_performance_tests`（可选）

## 当前可以确认的事实

- 仓库中存在较完整的测试分层
- C++ 测试工程已接入主构建系统
- 还存在一批 JS、HTML、style 和 regression 测试辅助文件

## 当前不能直接承诺的内容

- 所有测试在当前版本下均可通过
- 所有测试已完成维护
- 所有平台都已验证

## 建议使用方式

如果需要验证当前仓库状态，优先从以下流程开始：

```bash
cmake -B build -DMBINK_BUILD_TESTS=ON
cmake --build build --config Release
ctest --test-dir build --output-on-failure
```

更详细说明见：
- `docs/TESTING.md`