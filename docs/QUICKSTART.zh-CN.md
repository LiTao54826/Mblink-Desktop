# Quick Start

[English](QUICKSTART.md) | 中文

这份文档是目前在 Windows 上理解并跑起 MBink 的最快路径。

## 你要启动的是什么

MBink 目前有两条最实用的首次上手路径：

1. `mbink-ui-dev.exe`：面向 AI 优先的开发工作流
2. `esm_loader.exe`：最薄的一层手动运行时入口

如果你是第一次看这个仓库，建议先从 `mbink-ui-dev` 开始。

## 框架说明

当前最清晰的 UI 路径是：

- 直接的 JS / ESM 入口
- `mbink-ui-dev` 生成出来的项目，或仓库内现有项目
- 轻量化的官方 Preact 模块链路

所以现在更准确的理解方式是：这个仓库是偏 Preact 路线的，而不是一个可以直接承接完整 React 应用或大型浏览器脚手架的通用运行时。

当前已验证、最默认的模块导入路径就是直接使用 ESM import，尤其是这些已经内置进运行时链路的官方 Preact 模块：

```js
import { h, render } from 'preact';
import { useState } from 'preact/hooks';
```

一些简单第三方包在打包后也可能可以工作，但这应当按个案做运行时验证，不能把它理解成已经完整承诺了 Node.js 或 npm 整体生态兼容。

## 前置条件

- Windows
- CMake
- 能构建本仓库的 C++ 开发环境
- 如果你要打开/构建生成出来的 `tool` 项目，需要让 `esbuild` 出现在 `PATH` 或项目自身的 `node_modules` 中
- Python 不是必须，但在这个工作区里 `py -3` 是更稳妥的调用方式

## 先构建工具

在仓库根目录执行：

```powershell
cmake -B build
cmake --build build --config Release --target mbink_ui_dev esm_loader -- /m:1
```

预期输出：

- `build\bin\Release\mbink-ui-dev.exe`
- `build\bin\Release\esm_loader.exe`
- `build\bin\Release\mbink.dll`

## 路径 A：AI 优先开发循环

打开已经验证过的示例项目：

```powershell
build\bin\Release\mbink-ui-dev.exe open --project "examples\todo_app_js"
build\bin\Release\mbink-ui-dev.exe snapshot --project "examples\todo_app_js"
build\bin\Release\mbink-ui-dev.exe snapshot --project "examples\todo_app_js" --response file --include-screenshot
```

一些常用后续命令：

```powershell
build\bin\Release\mbink-ui-dev.exe info --project "examples\todo_app_js"
build\bin\Release\mbink-ui-dev.exe query "#todo-input" --project "examples\todo_app_js"
build\bin\Release\mbink-ui-dev.exe click 'button[type="submit"]' --project "examples\todo_app_js"
```

你应该看到：

- 一个真实运行中的 MBink 窗口
- 一份结构化 DOM / UI 快照
- 在加上 `--include-screenshot` 时生成的 PNG 截图

这条路径适合：

- 快速迭代 UI
- 通过 CLI 或 MCP 驱动 AI 代理
- 用结构化快照代替手动猜测

## 路径 B：使用 `esm_loader` 的手动运行时入口

如果你想直接理解最底层的运行时形态：

```powershell
build\bin\Release\esm_loader.exe examples\todo_app_js\app.js
```

这条路径有助于你从下面几个角度理解 MBink：

- 一个 JS 入口文件
- 一个宿主可执行文件
- 一个旁边放着的 `mbink.dll`

当你希望在不经过更高层工具的情况下使用 UI-dev 快照/控制能力时，`esm_loader` 也暴露了对应文件接口：

```powershell
build\bin\Release\esm_loader.exe examples\todo_app_js\app.js `
  --ui-dev-snapshot-file tmp\todo_snapshot.json `
  --ui-dev-console-file tmp\todo_console.json `
  --ui-dev-errors-file tmp\todo_errors.json
```

## 可选：创建一个新项目

你也可以直接生成一个新的最小项目：

```powershell
build\bin\Release\mbink-ui-dev.exe init "tmp\my-mbink-app" --purpose minimal --runtime tool
build\bin\Release\mbink-ui-dev.exe open --project "tmp\my-mbink-app"
build\bin\Release\mbink-ui-dev.exe snapshot --project "tmp\my-mbink-app"
```

说明：

- 当前打开生成的 `tool` 项目仍然依赖本地可用的 `esbuild`

当前可用的脚手架组合包括：

- `minimal`、`showcase`、`desktop-app`
- `tool`、`python`、`rust`、`go`

如果只看当前最稳妥的 JS UI 路径，仍然应该优先理解为“小型 MBink 项目 + 官方 Preact 链路”，而不是完整 React 生态栈。

## Python 作为次级手动宿主

在 `esm_loader` 之后，Python 是最容易继续读下去的一条绑定路径，但它仍然不是主入口，主入口还是 `mbink-ui-dev`。

参见：

- [绑定说明](BINDINGS.zh-CN.md)
- [Python 绑定 README](../bindings/python/README.md)

## 下一步读什么

- [AI 工作流](AI_WORKFLOW.zh-CN.md)
- [Skills / 技能书说明](SKILLS.zh-CN.md)
- [构建说明](BUILD.zh-CN.md)
- [绑定说明](BINDINGS.zh-CN.md)
- [tools/README.md](../tools/README.md)
