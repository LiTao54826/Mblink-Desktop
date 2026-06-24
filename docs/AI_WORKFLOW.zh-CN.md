# AI Workflow

[English](AI_WORKFLOW.md) | 中文

MBink 从一开始就把 AI 辅助开发视为一等工作流，而不是事后补上的能力。

## 推荐的理解方式

先把项目分成这几层：

1. `mbink-ui-dev.exe`：项目级工作入口
2. `mbink.dll`：运行时行为核心
3. `mbink_devtools.dll`：只在需要开发期快照/控制或运行时 HTTP MCP 时才启用

落到实际使用上：

- 人类开发者和 shell 型代理都可以直接使用 `mbink-ui-dev.exe`
- 支持 MCP 的客户端可以通过 `mbink-ui-dev.exe serve` 连接
- 直接基于 C API 的宿主可以通过 `mbink_devtools.dll` 暴露实时 UI 分析能力

## 为什么它适合 AI

当前开发面已经围绕机器可读反馈组织起来：

- `snapshot`
- `query`
- `inspect`
- `logs`
- `errors`
- `click`
- `input-text`
- `scroll`

这意味着 AI 代理可以基于真实运行时证据工作，而不是只靠静态读代码猜测。

## 最快工作流

```powershell
cmake --build build --config Release --target mbink_ui_dev esm_loader -- /m:1
build\bin\Release\mbink-ui-dev.exe open --project "examples\todo_app_js"
build\bin\Release\mbink-ui-dev.exe snapshot --project "examples\todo_app_js"
build\bin\Release\mbink-ui-dev.exe snapshot --project "examples\todo_app_js" --response file --include-screenshot
build\bin\Release\mbink-ui-dev.exe query "#todo-input" --project "examples\todo_app_js"
```

一个典型循环是：

1. 打开项目
2. 获取 UI 快照
3. 查询或检查关键选择器
4. 修改文件
5. 构建或重载
6. 再次获取快照
7. 用 click / input / scroll 验证交互

## CLI 与 MCP

下面这些场景更适合 CLI：

- 你的代理具备 shell 权限
- 你想要简单、直接的命令式自动化
- 你在本地快速迭代

下面这些场景更适合 MCP：

- 你的客户端原生支持 MCP
- 你希望使用结构化工具和资源
- 你需要一个持久的 `active_project` 会话

启动 MCP 桥：

```powershell
build\bin\Release\mbink-ui-dev.exe serve
```

## 技能入口

仓库里已经带了一套用于 MBink 项目工作的 repo-local skills / 技能书。

如果你想先看完整结构和入口方式，建议先读 [Skills / 技能书说明](SKILLS.zh-CN.md)。

核心文件：

- [tools/mbink_ui_dev/skills/mbink-ui-dev/SKILL.md](../tools/mbink_ui_dev/skills/mbink-ui-dev/SKILL.md)
- [tools/mbink_ui_dev/skills/mbink-ui-dev/agents/openai.yaml](../tools/mbink_ui_dev/skills/mbink-ui-dev/agents/openai.yaml)

它适合作为以下内容的详细参考：

- 命令优先级
- snapshot-first 的验证方式
- 先做 UI、再接宿主代码的工作流
- `esm_loader`、Python、Rust、Go 之间的一致性要求

## 新应用的推荐工作流

1. 用 `mbink-ui-dev` 创建或打开项目
2. 先用 mock 数据把 UI 做出来
3. 通过 `snapshot`、`query`、`inspect` 验证
4. 之后再接入真实 Python、Rust 或 Go 宿主
5. 再用同样的验证路径检查真实宿主

这件事很重要，因为 MBink 并不打算伪装成一个完整浏览器。最稳妥的方式，是直接证明运行时行为。

## 什么时候往下切层

下面这些情况适合使用 `esm_loader`：

- 你想看最薄的宿主路径
- 你要在没有项目工具层的情况下验证运行时行为
- 你需要显式的 UI-dev 快照/控制文件

下面这些情况适合使用 Python / Rust / Go 宿主：

- 应用需要真实宿主逻辑
- 你要验证绑定一致性
- `tool` 运行时已经不足以提供最终证据

## 重要约束

- `tool` 运行时很适合做 UI 形态验证，但不能作为宿主特定行为的最终证据
- `mbink_devtools.dll` 是开发期组件，不应当被视为生产运行时
- Windows 是当前证据最充分的工作流平台
- 仅有构建成功还不够，真实快照才是更好的完成证据
- 对外示例和文档优先使用 `snapshot --response file --include-screenshot`，这样同时保留结构化数据和 PNG 证据
