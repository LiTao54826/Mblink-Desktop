# AI Workflow

English | [中文](AI_WORKFLOW.zh-CN.md)

MBink is intentionally shaped so AI-assisted development is a first-class workflow, not an afterthought.

## Recommended mental model

Use these layers:

1. `mbink-ui-dev.exe` for project work
2. `mbink.dll` for runtime behavior
3. `mbink_devtools.dll` only when you need development-only snapshot/control or runtime HTTP MCP

In practice:

- humans and shell-based agents can use `mbink-ui-dev.exe` directly
- MCP-capable clients can connect through `mbink-ui-dev.exe serve`
- direct C API hosts can expose live UI analysis through `mbink_devtools.dll`

## Why the workflow fits AI well

The development surface is already organized around machine-readable feedback:

- `snapshot`
- `query`
- `inspect`
- `logs`
- `errors`
- `click`
- `input-text`
- `scroll`

That means an AI agent can work from real runtime evidence instead of only code inspection.

## Fastest workflow

```powershell
cmake --build build --config Release --target mbink_ui_dev esm_loader -- /m:1
build\bin\Release\mbink-ui-dev.exe open --project "examples\todo_app_js"
build\bin\Release\mbink-ui-dev.exe snapshot --project "examples\todo_app_js"
build\bin\Release\mbink-ui-dev.exe snapshot --project "examples\todo_app_js" --response file --include-screenshot
build\bin\Release\mbink-ui-dev.exe query "#todo-input" --project "examples\todo_app_js"
```

Typical loop:

1. open a project
2. snapshot the UI
3. query or inspect important selectors
4. edit files
5. build or reload
6. snapshot again
7. confirm interaction with click/input/scroll

## CLI vs MCP

Use CLI when:

- your agent has shell access
- you want simple command-driven automation
- you are iterating locally and quickly

Use MCP when:

- your client supports MCP natively
- you want structured tools and resources
- you want a persistent `active_project` session

Start the MCP bridge with:

```powershell
build\bin\Release\mbink-ui-dev.exe serve
```

## Skill entry point

The repository includes a repo-local skillbook for MBink project work.

Start with [Skills Guide](SKILLS.md) if you want the full layout and entry model.

Core files:

- [tools/mbink_ui_dev/skills/mbink-ui-dev/SKILL.md](../tools/mbink_ui_dev/skills/mbink-ui-dev/SKILL.md)
- [tools/mbink_ui_dev/skills/mbink-ui-dev/agents/openai.yaml](../tools/mbink_ui_dev/skills/mbink-ui-dev/agents/openai.yaml)

Use it as the detailed reference for:

- command priorities
- snapshot-first verification
- UI-first then host-integration workflow
- parity expectations across `esm_loader`, Python, Rust, and Go

## Recommended workflow for new app work

1. create or open a project with `mbink-ui-dev`
2. build the UI with mock data first
3. verify through `snapshot`, `query`, and `inspect`
4. only then wire the real Python, Rust, or Go host
5. re-run the same verification path against the real host

This is important because MBink is not trying to imitate a complete browser. The safest workflow is to prove runtime behavior directly.

## When to drop down to lower layers

Use `esm_loader` when:

- you want the thinnest host path
- you are validating runtime behavior without the project tooling layer
- you want explicit UI-dev snapshot/control files

Use Python/Rust/Go hosts when:

- the app needs real host logic
- you want to validate binding parity
- `tool` runtime is no longer enough evidence

## Important constraints

- `tool` runtime is great for UI shape, but not final proof for host-specific behavior
- `mbink_devtools.dll` is development-only and should not be treated as the production runtime
- Windows has the strongest verified workflow
- build success alone is not enough; a real snapshot is the better proof
- for public-facing examples and docs, prefer `snapshot --response file --include-screenshot` so the proof includes both DOM data and a PNG artifact
