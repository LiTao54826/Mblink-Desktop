<p align="center">
  <img src="./assets/logo.svg" alt="MBlink logo" width="140" />
</p>

# Mblink Desktop

[English](README.md) | 中文

MBlink 是一个以 Windows 为主的桌面 UI 框架与工具链，目标是用更轻量的原生运行时、现代 JavaScript UI 和对 AI 友好的开发流程，来构建中小型桌面应用。

## MBlink 是什么

- 以 `mblink.dll` 为核心的共享运行时
- 一个更贴近底层、便于手动理解的 `esm_loader.exe`
- 一个面向 AI 开发流程的 `mblink-ui-dev.exe`
- 基于同一套 C API 契约的 Python、Rust、Go 宿主绑定

## MBlink 不是什么

- 不是完整浏览器
- 不是 WebView 壳
- 还不是一个成熟的跨平台发布产品

MBlink 面向的是希望获得更小体积、更强可控性，以及更适合人类与 AI 协同开发流程的桌面应用场景。

## 当前 UI 兼容主线

MBlink 目前的 JS UI 兼容主线，是围绕轻量化的官方 Preact ESM 模块链路展开的：

- `preact`
- `preact/hooks`
- `preact/jsx-runtime`
- `preact/jsx-dev-runtime`

这也是当前最清晰、最适合中小型桌面应用的支持路径。

而且这些官方 Preact 模块不只是仓库里的示例依赖，它们已经作为内置运行时模块注册进 MBlink 的运行时链路里了。所以当前已验证、最默认的写法就是直接使用 ESM 导入：

- `import { h, render } from 'preact'`
- `import { useState } from 'preact/hooks'`
- `import { jsx } from 'preact/jsx-runtime'`

其他框架层就算局部能跑，也不应当视为当前已支持的兼容目标。除非仓库后续给出新的验证证据，否则不要默认它已经完整支持 React，或已经兼容大型、偏浏览器生态的现成脚手架。

## import 与包模型

MBlink 支持真实的 ESM `import` 风格 UI 代码。当前运行时和工具链可以处理：

- 项目内部的本地相对导入
- 上面这些内置的官方 Preact 模块
- 一部分在打包后并经过运行时验证的简单第三方包

但这不意味着：

- UI 运行时就是 Node.js
- CommonJS（`require`、`module.exports`）已经成为默认路径
- `fs`、`path`、`process`、`Buffer` 这类 Node 内置模块已经可用
- 大型、偏浏览器生态的 React 脚手架可以默认跑通

关于当前已支持的 API 范围，建议继续阅读 [docs/SKILLS.zh-CN.md](docs/SKILLS.zh-CN.md)、[docs/AI_WORKFLOW.zh-CN.md](docs/AI_WORKFLOW.zh-CN.md) 以及它们指向的 API 参考文档。

## 为什么值得关注

- `mblink-ui-dev` 提供了 `open`、`build`、`snapshot`、`query`、`inspect`、`click`、`serve` 这一套实用工作流
- `mblink.dll` 作为运行时核心，在工具链和各语言绑定之间保持一致
- `esm_loader` 提供了一条不隐藏 DLL/运行时结构的手动入口
- 仓库里已经有应用壳、Todo、日志视图、原生控件、桌面布局等可运行示例

## 实际效果

下面这些图都来自 Windows 上真实 `mblink-ui-dev` 会话后的运行截图。

![MBlink todo_app_js example](docs/assets/todo_app_js.png)
![MBlink modern_desktop_demo example](docs/assets/modern_desktop_demo.png)
![MBlink ui_combinations_showcase example](docs/assets/ui_combinations_showcase.png)
![MBlink html_demo example](docs/assets/html_demo.png)

## 快速开始

在 Windows 上，从仓库根目录执行：

```powershell
cmake -B build
cmake --build build --config Release --target mblink_ui_dev esm_loader -- /m:1
build\bin\Release\mblink-ui-dev.exe open --project "examples\todo_app_js"
build\bin\Release\mblink-ui-dev.exe snapshot --project "examples\todo_app_js"
build\bin\Release\mblink-ui-dev.exe snapshot --project "examples\todo_app_js" --response file --include-screenshot
```

你会得到：

- 一个真实运行中的 MBlink 窗口
- 一份可供 CLI 检查的结构化 UI 快照
- 在加上 `--include-screenshot` 时，由同一路径生成的 PNG 截图

如果你更想从 DLL / 运行时视角直接理解项目：

```powershell
build\bin\Release\esm_loader.exe examples\todo_app_js\app.js
```

完整入门请看 [docs/QUICKSTART.zh-CN.md](docs/QUICKSTART.zh-CN.md)。

## AI 优先工作流

MBlink 的设计目标之一，是让 AI 代理和人类开发者走同一条开发路径：

- shell / CLI 代理可以直接驱动 `mblink-ui-dev.exe`
- 支持 MCP 的客户端可以通过 `mblink-ui-dev.exe serve`
- 直接基于 C API 的宿主可以通过可选的 `mblink_devtools.dll` 暴露实时 UI 分析能力

仓库里已经包含可复用的技能文件：

- [tools/mblink_ui_dev/skills/mblink-ui-dev/SKILL.md](tools/mblink_ui_dev/skills/mblink-ui-dev/SKILL.md)

先看 [docs/SKILLS.zh-CN.md](docs/SKILLS.zh-CN.md) 了解这套技能书的结构和用法，再看 [docs/AI_WORKFLOW.zh-CN.md](docs/AI_WORKFLOW.zh-CN.md) 进入推荐工作流。

## 手动运行时模型

如果你想先搞清楚项目结构，而不是先从高层工具入手，可以先理解这几个层次：

- `mblink.dll`：共享运行时
- `esm_loader.exe`：仓库里最薄的一层手动宿主
- `mblink-ui-dev.exe`：项目工具、快照、构建/重载、MCP 和检查能力
- `mblink_devtools.dll`：开发时可选的 UI 快照、控制和运行时 HTTP MCP 伴随组件

运行时模型请看 [docs/BINDINGS.zh-CN.md](docs/BINDINGS.zh-CN.md) 和 [docs/C_API_RUNTIME_PARITY.md](docs/C_API_RUNTIME_PARITY.md)。

## 示例应用

`examples/` 中值得优先看的入口有：

- `todo_app_js`：最小、已验证、适合快速检查 `mblink-ui-dev`
- `modern_desktop_demo`：偏桌面应用风格的壳与布局示例
- `terminal_logview_demo`：MBlink 原生 terminal / logview 元素示例
- `component_demo`：更小粒度的 UI 组件组合示例
- `official_preact_jsx_dev`：官方 Preact ESM 链路的参考示例

## 当前项目阶段

MBlink 目前属于“早期但已经能看懂和跑起来”的仓库阶段：

- Windows 是证据最充分的平台
- 工具链和示例工作流比对外发布包装更成熟
- Python、Rust、Go 绑定共享同一套运行时契约
- 发布前必须重新通过 Python、Rust、Go 绑定检查
- Node.js 目前仍应视为占位，不应当作已支持绑定

这个仓库当前的目标不是“做一个完整浏览器”，而是让现代桌面应用开发更高效、更轻量、更可检查。

## 项目现实情况

MBlink 目前主要还是一个个人维护项目。

由于个人开发时间和测试精力有限，不可能在公开发布前把每个示例、每个平台、每种绑定、每个框架组合和所有边角情况都一一测试完。

所以当前公开文档会尽量只围绕仓库里有新鲜验证证据的路径来写，也会持续把读者引导到 Windows-first、以 Preact 为主线、已经实际跑通过的路径上。

凡是没有被当前文档明确配上验证证据的路径，都更适合先当作实验性能力理解，而不是默认已经承诺支持。

## 支持作者

如果 MBlink 对你有帮助，赞助可以直接支持这些事情继续推进：

- 补更多示例和工作流测试
- 继续完善文档和上手体验
- 修复兼容性问题
- 维护工具链和各语言绑定

可以在 `docs/assets/sponsor_qr.png` 放置公开赞助收款码图片，准备好后再把它接到这里展示。

## 文档

- [docs/QUICKSTART.zh-CN.md](docs/QUICKSTART.zh-CN.md)
- [docs/AI_WORKFLOW.zh-CN.md](docs/AI_WORKFLOW.zh-CN.md)
- [docs/SKILLS.zh-CN.md](docs/SKILLS.zh-CN.md)
- [docs/BUILD.zh-CN.md](docs/BUILD.zh-CN.md)
- [docs/BINDINGS.zh-CN.md](docs/BINDINGS.zh-CN.md)
- [docs/C_API_RUNTIME_PARITY.md](docs/C_API_RUNTIME_PARITY.md)
- [docs/BROWSER_COMPATIBILITY.md](docs/BROWSER_COMPATIBILITY.md)
- [docs/README.zh-CN.md](docs/README.zh-CN.md)

## 仓库结构

```text
core/        运行时、DOM、布局、渲染和公共 C API
tools/       mblink-ui-dev、esm_loader 以及构建工具
bindings/    Python、Rust、Go 绑定
examples/    可运行示例与验证目标
docs/        项目文档
tests/       测试与回归资产
```

## 贡献

请先阅读 [docs/CONTRIBUTING.md](docs/CONTRIBUTING.md)。

如果你会在这个仓库里使用 AI 编码工具，请同时阅读 [AGENTS.md](AGENTS.md)。

## 许可证

MIT，详见 [LICENSE](LICENSE)。
