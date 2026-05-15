<p align="center">
  <img src="./assets/logo.svg" alt="MBink Logo" width="140" />
</p>

# MBink

C++ desktop UI / application framework.
C++ 桌面 UI / 应用框架。

## Highlights | 特点

- CMake-based build
  基于 CMake 的构建系统
- Core modules for DOM, event, layout, render, compositor, windowing, scripting, and networking
  包含 DOM、事件、布局、渲染、合成、窗口、脚本与网络等核心模块
- Unified C API via `core/api/mbink.h`
  通过 `core/api/mbink.h` 提供统一 C API
- Python binding integrated in the top-level build
  Python binding 已接入顶层构建
- Tooling targets for bundling and ESM loading
  包含应用打包与 ESM 加载工具目标

## Status | 当前状态

| Area | Status | 说明 |
|---|---|---|
| Build system | CMake | 主构建系统 |
| Core source tree | `core/` | 核心源码目录 |
| Public C API | `core/api/mbink.h` | 对外 C 接口 |
| Python binding | Integrated | 已接入构建 |
| Go / Rust bindings | Implemented packages | 已有绑定包实现 |
| Node.js binding | Placeholder directory | 当前仅占位目录 |
| Browser compatibility | Partial, Chrome/Blink-oriented runtime layer | 当前为偏 Chrome/Blink 风格的基础兼容层，不能宣称完整浏览器兼容 |
| Test targets | Available with `MBINK_BUILD_TESTS=ON` | 通过选项启用 |
| Platform validation | Windows evidence is strongest | Windows 痕迹最完整 |

## Browser Compatibility | 浏览器兼容性

Current code shows a basic browser runtime compatibility layer with `window`, `navigator`, DOM query APIs, limited DOM polyfills, form / anchor base behavior, layout, and networking primitives.
当前代码体现的是一套基础浏览器运行时兼容层，包含 `window`、`navigator`、DOM 查询 API、有限 DOM polyfills、表单 / 锚点基础行为、布局与网络基础能力。

It should not be described as fully compatible with Chrome / Firefox / Safari / Edge, nor as full Web platform parity.
当前不应描述为已完整兼容 Chrome / Firefox / Safari / Edge，也不应宣称已达到完整 Web 平台一致性。

See | 详见：[docs/BROWSER_COMPATIBILITY.md](docs/BROWSER_COMPATIBILITY.md)

## Repository Layout | 仓库结构

```text
core/        Core modules / 核心模块
bindings/    Language bindings / 语言绑定
examples/    Examples and demos / 示例与演示
tests/       Test targets and test assets / 测试与测试资产
tools/       Tooling targets / 工具目标
scripts/     Build and helper scripts / 构建与辅助脚本
docs/        Project documentation / 项目文档
third_party/ Third-party dependencies / 第三方依赖
```

## Build | 构建

```bash
cmake -B build
cmake --build build --config Release
```

### Build with Tests | 构建并运行测试

```bash
cmake -B build -DMBINK_BUILD_TESTS=ON
cmake --build build --config Release
ctest --test-dir build --output-on-failure
```

See also | 参考：

- [docs/BUILD.md](docs/BUILD.md)
- [docs/TESTING.md](docs/TESTING.md)

## Examples | 示例

- `tools/mbink_ui_dev/` is the canonical starter path for new projects.
- `mbink-ui-dev init <dir> --purpose <minimal|showcase|desktop-app> --runtime <tool|python|rust|go>` generates slim purpose-first templates.
- Host runtimes generate runnable starters: `python host/main.py`, `go run ./host`, or `cargo run --manifest-path rust_host/Cargo.toml`.
- `examples/modern_desktop_demo/`
- `examples/component_demo/`
- `examples/html_demo/`
- `examples/preact_demo/`
- `examples/terminal_logview_demo/`

## Bindings | 绑定

| Language | Status | 说明 |
|---|---|---|
| Python | Implemented | 已实现 |
| Go | Implemented | 已实现，位于 `bindings/go/mbink` |
| Rust | Implemented | 已实现，位于 `bindings/rust/mbink` |
| Node.js | Placeholder | 占位 |

See | 详见：[docs/BINDINGS.md](docs/BINDINGS.md) · [bindings/go/README.md](bindings/go/README.md) · [bindings/rust/README.md](bindings/rust/README.md)

## Documentation | 文档

- [docs/README.md](docs/README.md)
- [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)
- [docs/BUILD.md](docs/BUILD.md)
- [docs/TESTING.md](docs/TESTING.md)
- [docs/BINDINGS.md](docs/BINDINGS.md)
- [docs/BROWSER_COMPATIBILITY.md](docs/BROWSER_COMPATIBILITY.md)
- [docs/KNOWN_LIMITATIONS.md](docs/KNOWN_LIMITATIONS.md)
- [docs/KNOWN_ISSUES.md](docs/KNOWN_ISSUES.md)
- [docs/CONTRIBUTING.md](docs/CONTRIBUTING.md)
- [CHANGELOG.md](CHANGELOG.md)
- [ROADMAP.md](ROADMAP.md)
- [SECURITY.md](SECURITY.md)
- [SUPPORT.md](SUPPORT.md)
- [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md)

## Contributing | 贡献

See [docs/CONTRIBUTING.md](docs/CONTRIBUTING.md).

## License | 许可证

MIT. See [LICENSE](LICENSE).
