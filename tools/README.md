# Tools

This folder contains the main public tool surfaces around the MBlink runtime.

## Primary tools

### `mblink-ui-dev`

Use this when you want:

- project scaffolding
- open/build/reload loops
- UI snapshots
- query/inspect/click/input workflows
- stdio MCP for AI tooling

Current build/open behavior for generated `tool` projects depends on `esbuild` being available on `PATH` or inside the project `node_modules`.

Build:

```powershell
cmake --build build --config Release --target mblink_ui_dev esm_loader -- /m:1
```

Common commands:

```powershell
build\bin\Release\mblink-ui-dev.exe open --project "examples\todo_app_js"
build\bin\Release\mblink-ui-dev.exe snapshot --project "examples\todo_app_js"
build\bin\Release\mblink-ui-dev.exe snapshot --project "examples\todo_app_js" --response file --include-screenshot
build\bin\Release\mblink-ui-dev.exe info --project "examples\todo_app_js"
build\bin\Release\mblink-ui-dev.exe build --project "examples\todo_app_js"
build\bin\Release\mblink-ui-dev.exe reload --project "examples\todo_app_js"
build\bin\Release\mblink-ui-dev.exe serve
```

Precision control examples:

```powershell
build\bin\Release\mblink-ui-dev.exe query "#todo-input" --project "examples\todo_app_js"
build\bin\Release\mblink-ui-dev.exe inspect 'button[type="submit"]' --project "examples\todo_app_js"
build\bin\Release\mblink-ui-dev.exe click 'button[type="submit"]' --project "examples\todo_app_js"
build\bin\Release\mblink-ui-dev.exe input-text "#todo-input" "hello world" --project "examples\todo_app_js"
build\bin\Release\mblink-ui-dev.exe input-text "#todo-input" --clear --project "examples\todo_app_js"
build\bin\Release\mblink-ui-dev.exe scroll body --y 400 --project "examples\todo_app_js"
build\bin\Release\mblink-ui-dev.exe highlight "#todo-input" --color "#ff4d4f" --project "examples\todo_app_js"
```

Notes:

- `snapshot` is the main observation surface
- `snapshot --response file --include-screenshot` is the best public-proof path when you need both structured UI data and a PNG artifact
- first `snapshot` after `open` should already contain real UI data
- if the runtime window is manually closed, watch/reload should not be expected to silently relaunch it

Detailed workflow reference:

- `tools/mblink_ui_dev/skills/mblink-ui-dev/SKILL.md`
- `../docs/SKILLS.md`

### `esm_loader`

Use this when you want the thinnest manual host path around `mblink.dll`.

Example:

```powershell
build\bin\Release\esm_loader.exe examples\todo_app_js\app.js
```

It also supports explicit UI-dev files:

```powershell
build\bin\Release\esm_loader.exe examples\todo_app_js\app.js `
  --ui-dev-snapshot-file tmp\todo_snapshot.json `
  --ui-dev-command-file tmp\todo_command.json `
  --ui-dev-response-file tmp\todo_response.json
```

## How the tools fit together

- `mblink.dll` is the runtime core
- `esm_loader.exe` is the clearer manual lane
- `mblink-ui-dev.exe` is the higher-level project and AI workflow layer

If you are new to the repo, start with `mblink-ui-dev`. If you want to understand the runtime shape directly, drop to `esm_loader`.
