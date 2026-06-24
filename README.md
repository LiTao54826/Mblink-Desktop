<p align="center">
  <img src="./assets/logo.svg" alt="MBink logo" width="140" />
</p>

# MBink

English | [中文](README.zh-CN.md)

MBink is a Windows-first desktop UI framework and tooling stack for building small and medium desktop apps with a lightweight native runtime, modern JS UI, and AI-friendly development workflows.

## What MBink is

- A shared runtime centered on `mbink.dll`
- A thin manual runner in `esm_loader.exe`
- An AI-first development surface in `mbink-ui-dev.exe`
- Python integration today, plus early Rust and Go bindings in the tree

## What MBink is not

- Not a complete browser
- Not a WebView wrapper
- Not a polished cross-platform release yet

MBink is aimed at desktop apps that want a smaller, more controllable runtime and a development loop that works well for both humans and AI agents.

## Current UI compatibility target

MBink currently centers its JS UI compatibility around the lightweight official Preact ESM path:

- `preact`
- `preact/hooks`
- `preact/jsx-runtime`
- `preact/jsx-dev-runtime`

This is the clearest supported lane today for small and medium desktop apps.

Those official Preact modules are not just examples in the tree. They are registered as embedded runtime modules in the MBink runtime path, so the verified default experience is based on direct ESM imports such as:

- `import { h, render } from 'preact'`
- `import { useState } from 'preact/hooks'`
- `import { jsx } from 'preact/jsx-runtime'`

Other framework layers may partly work, but they are not a supported compatibility target yet. Do not assume full React support or compatibility with large browser-oriented scaffolds unless the repository documents that with fresh verification evidence.

## Imports and package model

MBink supports real ESM `import`-based UI code. The runtime and tooling can handle:

- local relative imports inside your project
- the embedded official Preact modules above
- some simple third-party packages after bundling and runtime verification

What this does not mean:

- the UI runtime is not Node.js
- CommonJS (`require`, `module.exports`) is not the default path
- Node built-ins such as `fs`, `path`, `process`, and `Buffer` are not a supported UI contract
- large browser-oriented React scaffolds should not be assumed to work

For the current supported API surface, read [docs/SKILLS.md](docs/SKILLS.md), [docs/AI_WORKFLOW.md](docs/AI_WORKFLOW.md), and the supported API references those pages point to.

## Why it is interesting

- `mbink-ui-dev` gives you a practical loop for `open`, `build`, `snapshot`, `query`, `inspect`, `click`, and `serve`
- `mbink.dll` stays the core runtime surface across tools and bindings
- `esm_loader` gives you a direct manual lane for running a single entry file without hiding the DLL/runtime shape
- The repository already contains runnable examples for app shells, todo apps, log views, native controls, and desktop-style layouts

## What it looks like

The screenshots below were captured from real `mbink-ui-dev` sessions on Windows.

![MBink todo_app_js example](docs/assets/todo_app_js.png)
![MBink modern_desktop_demo example](docs/assets/modern_desktop_demo.png)
![MBink ui_combinations_showcase example](docs/assets/ui_combinations_showcase.png)
![MBink html_demo example](docs/assets/html_demo.png)

## Quick Start

From the repository root on Windows:

```powershell
cmake -B build
cmake --build build --config Release --target mbink_ui_dev esm_loader -- /m:1
build\bin\Release\mbink-ui-dev.exe open --project "examples\todo_app_js"
build\bin\Release\mbink-ui-dev.exe snapshot --project "examples\todo_app_js"
build\bin\Release\mbink-ui-dev.exe snapshot --project "examples\todo_app_js" --response file --include-screenshot
```

What this gives you:

- a real MBink runtime window for `examples/todo_app_js`
- a structured UI snapshot you can inspect from the CLI
- an optional PNG screenshot written by the same runtime path when you add `--include-screenshot`

If you prefer the manual DLL-facing lane:

```powershell
build\bin\Release\esm_loader.exe examples\todo_app_js\app.js
```

Start with the full guide in [docs/QUICKSTART.md](docs/QUICKSTART.md).

## AI-First Workflow

MBink is designed so an AI agent can work through the same development surface a human uses:

- shell/CLI agents can drive `mbink-ui-dev.exe` directly
- MCP-capable clients can use `mbink-ui-dev.exe serve`
- direct C API hosts can expose live UI analysis through the optional `mbink_devtools.dll`

The repository ships a reusable skill at [tools/mbink_ui_dev/skills/mbink-ui-dev/SKILL.md](tools/mbink_ui_dev/skills/mbink-ui-dev/SKILL.md).

Start with [docs/SKILLS.md](docs/SKILLS.md) for the skillbook layout and how to use it with AI agents, then read [docs/AI_WORKFLOW.md](docs/AI_WORKFLOW.md) for the recommended workflow.

## Manual Runtime Model

If you want to understand the project surface without the dev tooling layer, start here:

- `mbink.dll`: the shared runtime
- `esm_loader.exe`: the thinnest manual host in this repository
- `mbink-ui-dev.exe`: project tooling, snapshots, build/watch, MCP, and inspection
- `mbink_devtools.dll`: optional development companion for UI snapshots, control, and runtime HTTP MCP

Read [docs/BINDINGS.md](docs/BINDINGS.md) and [docs/C_API_RUNTIME_PARITY.md](docs/C_API_RUNTIME_PARITY.md) for the runtime model.

## Example Apps

Good starting points in `examples/`:

- `todo_app_js`: smallest verified `mbink-ui-dev` example for quick checks
- `modern_desktop_demo`: desktop-style app shell and layout demo
- `terminal_logview_demo`: MBink native terminal/logview elements
- `component_demo`: smaller UI component combinations
- `official_preact_jsx_dev`: a tracked reference for the official Preact ESM path

## Project Status

MBink is in an early but usable repository stage:

- Windows has the strongest build and runtime evidence
- the dev tooling and example workflow are more mature than the public packaging story
- Python is the clearest binding to read first
- Rust and Go bindings exist, but public onboarding is still secondary
- Node.js should still be treated as a placeholder, not a supported binding

The goal of this repository is not "ship a full browser." It is to make modern desktop app development more efficient, lightweight, and inspectable.

## Project Reality

MBink is primarily maintained as a personal project.

Because development time and testing capacity are limited, not every example, platform, binding, framework combination, or edge case can be verified one by one before public release.

That is why the public docs stay close to fresh, repository-local evidence and keep steering readers toward the Windows-first, Preact-centered paths that have actually been checked.

If a path is not clearly documented with current verification evidence, treat it as experimental rather than promised support.

## Support the Author

If MBink is useful to you, sponsorship helps fund time for:

- testing more examples and workflows
- improving documentation and onboarding
- fixing compatibility issues
- maintaining tools and bindings

You can add a public sponsor QR image at `docs/assets/sponsor_qr.png` and reference it from this section when it is ready.

## Docs

- [docs/QUICKSTART.md](docs/QUICKSTART.md)
- [docs/AI_WORKFLOW.md](docs/AI_WORKFLOW.md)
- [docs/SKILLS.md](docs/SKILLS.md)
- [docs/BUILD.md](docs/BUILD.md)
- [docs/BINDINGS.md](docs/BINDINGS.md)
- [docs/C_API_RUNTIME_PARITY.md](docs/C_API_RUNTIME_PARITY.md)
- [docs/BROWSER_COMPATIBILITY.md](docs/BROWSER_COMPATIBILITY.md)
- [docs/README.md](docs/README.md)

## Repository Layout

```text
core/        Runtime, DOM, layout, render, and public C API
tools/       mbink-ui-dev, esm_loader, and build tooling
bindings/    Python, Rust, and Go bindings
examples/    Runnable examples and validation targets
docs/        Project documentation
tests/       Test and regression assets
```

## Contributing

Start with [docs/CONTRIBUTING.md](docs/CONTRIBUTING.md).

If you use AI coding tools in this repository, the canonical automation/contributor guidance lives in [AGENTS.md](AGENTS.md).

## License

MIT. See [LICENSE](LICENSE).
