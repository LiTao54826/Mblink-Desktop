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


