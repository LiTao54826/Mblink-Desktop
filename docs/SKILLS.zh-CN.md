# Skills / 技能书说明

[English](SKILLS.md) | 中文

MBlink 仓库里自带了一套 repo-local skills / 技能书，用来让 AI 编码工具基于项目自己的工作流做事，而不是只看目录后自行猜测。

## 这是什么

MBlink 项目工作流对应的技能书目录在这里：

- `tools/mblink_ui_dev/skills/mblink-ui-dev/`

它本质上就是仓库里的普通文件：

- 人可以直接阅读
- 支持 skills 的代理可以把它当作指令入口
- 不支持 skills 的 shell 型代理也可以手动照着执行

当你希望 AI 工具按照 MBlink 已验证过的开发路径工作时，就应该使用这套技能书。

## 目录结构

```text
tools/mblink_ui_dev/skills/mblink-ui-dev/
  SKILL.md
  agents/
    openai.yaml
  references/
    cli-mcp-reference.md
    compatibility-guidelines.md
    native-elements.md
    supported-api-reference.md
```

## 每个文件是做什么的

### `SKILL.md`

这是最核心的入口文件。

它会说明：

- 什么时候应该使用 `mblink-ui-dev`
- 标准的 open -> snapshot -> query -> inspect -> edit -> build -> reload 工作流
- snapshot-first 的验证方式
- 先做 UI、再接宿主代码的开发顺序
- `esm_loader`、Python、Rust、Go 之间的一致性要求

如果你的 AI 客户端支持 repo-local skills，它首先应该读的就是这个文件。

### `agents/openai.yaml`

这是给 OpenAI 风格客户端准备的一份轻量元数据。

它主要提供：

- 展示名称
- 简短描述
- 默认提示词入口

它不是完整说明书。真正的工作流规则仍然写在 `SKILL.md` 里。

### `references/`

这个目录放的是技能书会引用的更细化资料：

- `cli-mcp-reference.md`：精确的 CLI 命令、MCP 映射、项目解析方式、模板说明
- `compatibility-guidelines.md`：运行时边界与兼容性约束
- `native-elements.md`：`terminal`、`logview` 等原生元素说明
- `supported-api-reference.md`：当前 MBlink UI 工作可依赖的 API 范围

## 如何和 AI 工具一起使用

### 如果你的客户端支持 skills

1. 打开 MBlink 仓库。
2. 让代理使用 `mblink-ui-dev` skill。
3. 在修改代码前先读取 `tools/mblink_ui_dev/skills/mblink-ui-dev/SKILL.md`。
4. 优先通过 `mblink-ui-dev.exe` 进行项目工作。
5. 只有在需要宿主级证据时，再下探到 `esm_loader`、Python、Rust 或 Go。

### 如果你的客户端不支持 repo-local skills

同一套规则仍然可以手动使用：

1. 直接阅读 `tools/mblink_ui_dev/skills/mblink-ui-dev/SKILL.md`。
2. 把这个文件路径告诉你的代理，或者把相关内容贴进上下文。
3. 按照同样的 `mblink-ui-dev.exe` CLI 工作流执行。

## 它和 MBlink 运行时分层是什么关系

技能书描述的是工作流，不是另一套运行时。

- `mblink-ui-dev.exe`：项目级工作入口，负责 open/build/snapshot/query/inspect/MCP
- `mblink.dll`：运行时核心
- `mblink_devtools.dll`：开发期可选伴随组件，用于实时快照/控制和运行时 HTTP MCP
- `esm_loader.exe`：围绕 `mblink.dll` 的最薄手动宿主

所以技能书并不会取代手动的 DLL 视角。它的作用是先把高层工具路径讲清楚，再告诉你什么时候需要回到更底层去验证。

## 快速开始

先构建工具：

```powershell
cmake -B build
cmake --build build --config Release --target mblink_ui_dev esm_loader -- /m:1
```

执行最小验证工作流：

```powershell
build\bin\Release\mblink-ui-dev.exe open --project "examples\todo_app_js"
build\bin\Release\mblink-ui-dev.exe snapshot --project "examples\todo_app_js"
build\bin\Release\mblink-ui-dev.exe query "#todo-input" --project "examples\todo_app_js"
```

如果你想直接走手动运行时路径：

```powershell
build\bin\Release\esm_loader.exe examples\todo_app_js\app.js
```

## 什么时候应该看这页

当你想快速搞清楚这些问题时，先看这页：

- MBlink 的 skills / 技能书到底放在哪里
- 这套技能书包含哪些文件
- skill-aware AI 工具应该怎样进入这个仓库
- `mblink-ui-dev`、`mblink.dll`、`mblink_devtools.dll`、`esm_loader` 之间是什么关系

## 相关文档

- [快速开始](QUICKSTART.zh-CN.md)
- [AI 工作流](AI_WORKFLOW.zh-CN.md)
- [工具总览](../tools/README.md)
- [贡献者自动化说明](../AGENTS.md)
