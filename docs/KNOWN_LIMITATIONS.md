# Known Limitations | 已知限制

## Current Constraints | 当前限制

### Naming | 命名

`MBink` / `mbink` naming still appears in multiple places.
`MBink` / `mbink` 命名仍出现在多个位置：

- CMake project names / CMake 项目名
- library and target names / 库与 target 名称
- Python package names / Python 包名
- public headers and API identifiers / 公开头文件与 API 标识符

### Bindings | 绑定

Only Python binding has verified implementation status.
当前只有 Python 绑定具备可验证实现。

The following should not be described as supported bindings:
以下目录当前不应描述为已支持绑定：

- `bindings/go/`
- `bindings/rust/`
- `bindings/nodejs/`

### UI Framework Compatibility | UI 框架兼容性

The verified JS UI lane today is centered on the lightweight official Preact modules:
当前已经验证的 JS UI 主线，集中在轻量化的官方 Preact 模块链路上：

- `preact`
- `preact/hooks`
- `preact/jsx-runtime`
- `preact/jsx-dev-runtime`

The repository should not yet be described as supporting full React runtimes or large browser-oriented scaffolds by default.
当前不应把这个仓库描述成默认支持完整 React 运行时，或默认兼容大型、偏浏览器生态的现成脚手架。

If another framework happens to run in a specific example, treat that as local evidence, not a general compatibility guarantee.
即使其他框架在某个局部示例里碰巧能运行，也只能算局部证据，不能上升为整体兼容承诺。

MBink does support real ESM `import`-based UI code, and the official Preact modules are embedded into the verified runtime path.
MBink 的确支持基于 ESM `import` 的 UI 代码，而且官方 Preact 模块已经嵌入到已验证的运行时链路里。

Some simple third-party packages may work after bundling and runtime verification, but that still does not make the UI runtime a full Node.js or npm compatibility layer.
一些简单第三方包在打包并经过运行时验证后也可能可以工作，但这仍然不意味着 UI 运行时已经变成完整的 Node.js 或 npm 兼容层。

### Platform Validation | 平台验证

- Windows has the strongest build evidence in the repository
  Windows 在仓库中有最完整的构建痕迹
- other platforms still require verification
  其他平台仍需验证

### Testing | 测试

Current repository state does not justify claims that:
当前仓库状态不足以直接宣称：

- the full test suite passes consistently / 全量测试稳定通过
- coverage is complete / 覆盖率完整
- all platforms have validated test results / 所有平台都有已验证测试结果

### Third-Party Dependencies | 第三方依赖

Third-party dependencies and related fetch/build traces still need cleanup for open-source packaging.
第三方依赖及相关抓取/构建痕迹仍需继续清理以适配开源发布。


