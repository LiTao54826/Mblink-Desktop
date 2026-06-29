# Quick Start

English | [Chinese](QUICKSTART.zh-CN.md)

This is the fastest honest path for a new Windows checkout. MBlink is usable, but
the public build story is still Windows-first and Skia-prepared.

## What you are starting

MBlink has two practical first-run lanes:

1. `mblink-ui-dev.exe` for the AI-first development workflow
2. `esm_loader.exe` for the thinnest manual runtime path

If you are new to the repository, start with `mblink-ui-dev`.

## Prerequisites

- Windows
- Git
- PowerShell 7 or Windows PowerShell
- CMake
- A working MSVC C++ build environment
- Prepared Skia Debug and Release libraries for the default native build
- `esbuild` on `PATH` or in project `node_modules` for generated tool projects
- Python 3 if you run repository maintenance checks

The public dependency bootstrap fetches QuickJS-ng and SDL3 at pinned commits. It
does not download or build Skia because the current CMake files expect a prepared
binary layout.

## Prepare source dependencies

From the repository root:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\download_deps.ps1 -NonInteractive
```

Expected behavior:

- `third_party\quickjs` is cloned from QuickJS-ng and checked out at the script's
  pinned commit
- `third_party\SDL3` is cloned from SDL and checked out at the script's pinned
  commit
- Skia status is reported, but no random Skia source or prebuilt package is
  fetched

If an existing dependency directory is at a different commit, the script leaves it
alone unless you pass `-Force`.

## Prepare Skia

The default build currently expects:

```text
third_party\skia\Debug\out\Debug-windows-x64\skia.lib
third_party\skia\Release\out\Release-windows-x64\skia.lib
```

Record where those binaries came from before publishing a release artifact. See
[Build](BUILD.md), [Release](RELEASE.md), and
[Third-Party Notices](../THIRD_PARTY_NOTICES.md).

## Build the tools

```powershell
cmake -B build
cmake --build build --config Release --target mblink_ui_dev esm_loader -- /m:1
```

Expected outputs:

- `build\bin\Release\mblink-ui-dev.exe`
- `build\bin\Release\esm_loader.exe`
- `build\bin\Release\mblink.dll`

## Path A: AI-first development loop

Open and inspect the verified starter example:

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

## Path B: Manual runtime lane

If you want the clearest direct runtime shape:

```powershell
build\bin\Release\esm_loader.exe examples\todo_app_js\app.js
```

This path is useful when you want to understand MBlink in terms of one JS entry
file, one executable host, and one adjacent `mblink.dll`.

`esm_loader` also exposes explicit UI-dev files:

```powershell
build\bin\Release\esm_loader.exe examples\todo_app_js\app.js `
  --ui-dev-snapshot-file tmp\todo_snapshot.json `
  --ui-dev-console-file tmp\todo_console.json `
  --ui-dev-errors-file tmp\todo_errors.json
```

## Framework note

The clearest current UI path is:

- direct JS/ESM app entries
- `mblink-ui-dev` generated or repo-local projects
- the lightweight official Preact module path

The verified default module path is direct ESM import:

```js
import { h, render } from 'preact';
import { useState } from 'preact/hooks';
```

Simple third-party packages may work after bundling, but treat that as
case-by-case runtime compatibility rather than a complete Node.js or npm
ecosystem contract.

## Optional: Create a new project

```powershell
build\bin\Release\mblink-ui-dev.exe init "tmp\my-mblink-app" --purpose minimal --runtime tool
build\bin\Release\mblink-ui-dev.exe open --project "tmp\my-mblink-app"
build\bin\Release\mblink-ui-dev.exe snapshot --project "tmp\my-mblink-app"
```

Generated `tool` projects currently depend on `esbuild`.

## Where to go next

- [AI Workflow](AI_WORKFLOW.md)
- [Skills Guide](SKILLS.md)
- [Build](BUILD.md)
- [Examples](../examples/README.md)
- [Bindings](BINDINGS.md)
- [tools/README.md](../tools/README.md)
