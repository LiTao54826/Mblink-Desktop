# Security Policy | 安全策略

## Reporting | 报告方式

This repository does not currently define a dedicated private security reporting channel.
当前仓库尚未提供专用的私密安全报告渠道。

If you believe you found a security issue:
如果你认为发现了安全问题：

- do not publish exploit details beyond what is necessary for initial triage
  不要公开超出初步分流所需的利用细节
- open a repository issue with a minimal description, affected area, and reproduction notes
  请提交 issue，并提供最小描述、影响范围和复现说明
- mark clearly that the report is security-related
  明确标注该报告与安全相关

## Scope | 范围

Security-sensitive areas may include:
可能涉及安全敏感的区域包括：

- JavaScript runtime integration / JavaScript 运行时集成
- native bindings and FFI boundaries / 原生绑定与 FFI 边界
- file loading and application bundling paths / 文件加载与应用打包路径
- network-related code / 网络相关代码
- windowing and platform integration code / 窗口与平台集成代码

## Current Status | 当前状态

This project is still being cleaned up as an open-source repository.
项目仍在进行开源整理。

That means:
这意味着：

- security review coverage is not claimed as complete
  当前不宣称安全审查覆盖完整
- supported platform and binding scope is still being clarified
  支持的平台与绑定范围仍在厘清中
- absence of a documented issue does not imply absence of risk
  没有文档记录的问题不代表不存在风险

