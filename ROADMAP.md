# Roadmap | 路线图

This roadmap describes areas that are visible from the current repository state and likely worth continuing, without promising completion dates.
本路线图描述基于当前仓库状态可见、且值得继续推进的方向，不承诺完成日期。

## Current Priorities | 当前重点

### 1. Documentation cleanup | 文档整理

- keep README and `docs/` aligned with the current source tree
  保持 README 与 `docs/` 和当前源码树一致
- replace historical narrative documents with reference-style pages
  将历史叙事型文档替换为参考型页面
- add API reference entry points when the public surface is stable enough
  在公开接口足够稳定后补充 API 参考入口

### 2. Build and platform validation | 构建与平台验证

- verify non-Windows build paths
  验证非 Windows 平台构建路径
- reduce ambiguity around third-party dependency setup
  降低第三方依赖配置的歧义
- improve reproducibility of the documented build flow
  提高文档中构建流程的可复现性

### 3. Testing validation | 测试验证

- verify which existing test targets pass reliably
  验证哪些现有测试目标能稳定通过
- separate automated tests from manual verification assets more clearly
  更清晰地区分自动化测试与人工验证资产
- document the real status of performance and property tests
  记录性能测试与 property tests 的真实状态

### 4. Binding status cleanup | 绑定状态整理

- continue validating Python binding behavior
  持续验证 Python binding 行为
- keep placeholder binding directories clearly marked as non-supported until implemented
  在实现前明确将占位绑定目录标记为不受支持
- unify naming and packaging details where possible
  在可控范围内统一命名与打包细节

### 5. Repository cleanup | 仓库整理

- reduce leftover historical noise in docs and scripts
  继续清理文档与脚本中的历史噪音
- continue unifying `MBink` / `mbink` naming where safe
  在安全范围内继续统一 `MBink` / `mbink` 命名
- clarify which examples are actively runnable
  明确哪些示例当前可运行

## Out of Scope for This Document | 本文档不承诺

This file should not be used to promise:
本文件不应用于承诺：

- release dates / 发布时间
- unsupported platform claims / 未验证的平台支持
- unverified feature completion / 未验证功能完成度
- binding support that does not exist in the repository / 仓库中不存在的绑定支持

