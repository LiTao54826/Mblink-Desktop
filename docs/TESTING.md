# Testing | 测试

## Test Layout | 测试结构

- `tests/unit/`
- `tests/render/`
- `tests/integration/`
- `tests/property/`
- `tests/performance/`
- `tests/js/`
- `tests/style_tests/`
- `tests/regression/`
- `tests/test_utils/`

## CMake Test Targets | CMake 测试目标

- `mbink_unit_tests`
- `mbink_render_tests`
- `mbink_integration_tests`
- `mbink_property_tests`
- `mbink_performance_tests` (optional / 可选)

## Build and Run Tests | 构建并运行测试

```bash
cmake -B build -DMBINK_BUILD_TESTS=ON
cmake --build build --config Release
ctest --test-dir build --output-on-failure
```

Runtime parity regression:

```powershell
cmake --build build --config Release --target mbink_api esm_loader mbink_ui_dev
powershell -NoProfile -ExecutionPolicy Bypass -File tests\regression\test_esm_loader_c_api_parity.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File tests\regression\test_mbink_ui_dev_p0_regression.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File tests\regression\test_mbink_ui_dev_responsiveness.ps1
```

The `test_esm_loader_c_api_parity.ps1` script verifies that `esm_loader.exe`
stays a thin `mbink.dll` consumer and that snapshot, console, errors,
lifecycle, UI-dev command handling, and `--no-scripts` behavior flow through
the C API.

Performance tests | 性能测试：

```bash
cmake -B build -DMBINK_BUILD_TESTS=ON -DMBINK_BUILD_PERF_TESTS=ON
cmake --build build --config Release
```

## Coverage Areas Visible from the Repository | 从仓库可见的覆盖方向

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

Additional facts visible from test build files | 从测试构建文件还能确认：

- unit tests cover DOM / Event / Layout / Render / QuickJS / Lexbor / Utils / Compositor
  unit tests 覆盖 DOM / Event / Layout / Render / QuickJS / Lexbor / Utils / Compositor
- property tests cover Render / Layout / Bundler / Compositor / Virtual Text
  property tests 覆盖 Render / Layout / Bundler / Compositor / Virtual Text
- property tests reuse parts of `tools/app_bundler`
  property tests 会复用 `tools/app_bundler` 的部分源码

## Limits of Current Claims | 当前表述边界

The repository structure alone does not prove that:
仅凭仓库结构不能证明：

- all tests pass consistently / 所有测试稳定通过
- coverage is complete / 覆盖率完整
- all platforms are validated / 所有平台都已验证
- all script and style tests are fully automated / 所有脚本与样式测试都已自动化


