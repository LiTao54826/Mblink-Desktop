# 测试说明

本文档只描述当前仓库中可以从代码结构和 CMake 配置确认的测试状态。

## 当前测试结构

仓库中存在多类测试与测试辅助目录：
- `tests/unit/`
- `tests/render/`
- `tests/integration/`
- `tests/property/`
- `tests/performance/`
- `tests/js/`
- `tests/style_tests/`
- `tests/regression/`
- `tests/test_utils/`

其中，`unit / render / integration / property / performance` 主要对应 CMake 接入的 C++ 测试目标；`js / style_tests / regression` 更适合作为脚本、回归样例和人工验证材料来理解。

## CMake 中已接入的测试目标

可确认的目标：
- `mbink_unit_tests`
- `mbink_render_tests`
- `mbink_integration_tests`
- `mbink_property_tests`
- `mbink_performance_tests`（需额外开启）

## 启用方式

```bash
cmake -B build -DMBINK_BUILD_TESTS=ON
cmake --build build --config Release
ctest --test-dir build --output-on-failure
```

性能测试需要额外选项：

```bash
cmake -B build -DMBINK_BUILD_TESTS=ON -DMBINK_BUILD_PERF_TESTS=ON
cmake --build build --config Release
```

## 当前可确认范围

从测试源码和 CMake 配置可见，测试覆盖方向包括：
- DOM
- Event
- Layout
- Render
- QuickJS
- Lexbor
- Compositor
- Integration
- Property-based testing
- Performance

从 `tests/unit/CMakeLists.txt` 和 `tests/property/CMakeLists.txt` 还能进一步确认：
- unit tests 覆盖 DOM / Event / Layout / Render / QuickJS / Lexbor / Utils / Compositor
- property tests 覆盖 Render / Layout / Bundler / Compositor / Virtual Text
- property tests 会直接引用 `tools/app_bundler` 的部分源码参与测试

此外，仓库中还保留了一批与样式、脚本运行和回归问题相关的辅助测试材料。

## 当前不应过度承诺的内容

目前只能确认测试工程和测试源码存在，不能仅凭目录结构宣称：
- 所有测试稳定通过
- 覆盖率完整
- 所有平台测试均已验证
- 所有脚本和样式测试都已自动化维护

## 对外建议表述

建议在 README 中使用如下口径：
- 项目包含较完整的测试结构
- 当前仍需进一步验证测试通过率与跨平台状态
- 一部分测试材料用于人工验证和回归观察

