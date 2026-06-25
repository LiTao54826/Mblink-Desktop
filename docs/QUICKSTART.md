# Quick Start

English | [中文](QUICKSTART.zh-CN.md)

This page is the fastest path to understanding and running MBlink on Windows.

## What you are starting

MBlink has two practical first-run lanes:

1. `mblink-ui-dev.exe` for the AI-first development workflow
2. `esm_loader.exe` for the thinnest manual runtime path

If you are new to the repository, start with `mblink-ui-dev`.

## Framework note

The clearest current UI path is:

- direct JS/ESM app entries
- `mblink-ui-dev` generated or repo-local projects
- the lightweight official Preact module path

That means the repository should currently be read as Preact-oriented, not as a drop-in runtime for full React apps or large browser-oriented scaffolds.

The verified default module path is direct ESM import, especially for the embedded official Preact modules:

```js
import { h, render } from 'preact';
import { useState } from 'preact/hooks';
```

Simple third-party packages may also work after bundling, but they should be treated as case-by-case runtime compatibility, not as proof of a complete Node.js or npm ecosystem contract.

## Prerequisites

- Windows
- CMake
- A working C++ build environment for this repository
- `esbuild` available on `PATH` or in the project `node_modules` if you want to open/build generated `tool` projects
- Python is optional, but `py -3` is the safest command shape in this workspace

## Build the tools

From the repository root:

```powershell
cmake -B build
cmake --build build --config Release --target mblink_ui_dev esm_loader -- /m:1
```

Expected outputs:

- `build\bin\Release\mblink-ui-dev.exe`
- `build\bin\Release\esm_loader.exe`
- `build\bin\Release\mblink.dll`

## Path A: AI-first development loop

Open the verified example project:

```powershell
build\bin\Release\mblink-ui-dev.exe open --project "examples\todo_app_js"
build\bin\Release\mblink-ui-dev.exe snapshot --project "examples\todo_app_js"
build\bin\Release\mblink-ui-dev.exe snapshot --project "examples\todo_app_js" --response file --include-screenshot
```

Useful follow-up commands:

```powershell
build\bin\Release\mblink-ui-dev.exe info --project "examples\todo_app_js"
build\bin\Release\mblink-ui-dev.exe query "#todo-input" --project "examples\todo_app_js"
build\bin\Release\mblink-ui-dev.exe click 'button[type="submit"]' --project "examples\todo_app_js"
```

What you should expect:

- a real MBlink runtime window
- a structured DOM/UI snapshot
- an optional PNG screenshot when `--include-screenshot` is used

What this path is for:

- fast UI iteration
- AI-agent control through CLI or MCP
- structured snapshots instead of manual guesswork

## Path B: Manual runtime lane with `esm_loader`

If you want the clearest direct runtime shape:

```powershell
build\bin\Release\esm_loader.exe examples\todo_app_js\app.js
```

This path is useful when you want to understand MBlink in terms of:

- one JS entry file
- one executable host
- one adjacent `mblink.dll`

`esm_loader` also exposes explicit UI-dev hooks when you want snapshot/control behavior without the higher-level tool:

```powershell
build\bin\Release\esm_loader.exe examples\todo_app_js\app.js `
  --ui-dev-snapshot-file tmp\todo_snapshot.json `
  --ui-dev-console-file tmp\todo_console.json `
  --ui-dev-errors-file tmp\todo_errors.json
```

## Optional: Create a new project

You can also scaffold a new minimal project:

```powershell
build\bin\Release\mblink-ui-dev.exe init "tmp\my-mblink-app" --purpose minimal --runtime tool
build\bin\Release\mblink-ui-dev.exe open --project "tmp\my-mblink-app"
build\bin\Release\mblink-ui-dev.exe snapshot --project "tmp\my-mblink-app"
```

Notes:

- opening a generated `tool` project currently depends on `esbuild` being installed locally

Available scaffold combinations include:

- `minimal`, `showcase`, `desktop-app`
- `tool`, `python`, `rust`, `go`

The safest JS UI expectation today is still a small MBlink project built around the official Preact path, not a full React ecosystem stack.

## Python as a secondary manual host

Python is the easiest binding to read after `esm_loader`, but it is still a secondary onboarding path compared with `mblink-ui-dev`.

See:

- [Bindings](BINDINGS.md)
- [Python binding README](../bindings/python/README.md)

## Where to go next

- [AI Workflow](AI_WORKFLOW.md)
- [Skills Guide](SKILLS.md)
- [Build](BUILD.md)
- [Bindings](BINDINGS.md)
- [tools/README.md](../tools/README.md)
