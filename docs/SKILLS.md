# Skills Guide

English | [中文](SKILLS.zh-CN.md)

MBink ships a repo-local skillbook so AI coding tools can work from project-specific instructions instead of guessing from the raw tree.

## What this is

The skillbook for MBink project work lives at:

- `tools/mbink_ui_dev/skills/mbink-ui-dev/`

It is plain repository content:

- humans can read it directly
- skill-aware agents can load it as instructions
- shell-based agents can still follow it manually

Use this surface when you want an AI tool to work on MBink UI projects through the same verified development loop a human would use.

## Layout

```text
tools/mbink_ui_dev/skills/mbink-ui-dev/
  SKILL.md
  agents/
    openai.yaml
  references/
    cli-mcp-reference.md
    compatibility-guidelines.md
    native-elements.md
    supported-api-reference.md
```

## What each file does

### `SKILL.md`

This is the canonical entry point.

It explains:

- when to use `mbink-ui-dev`
- the standard open -> snapshot -> query -> inspect -> edit -> build -> reload loop
- snapshot-first verification
- UI-first, host-integration-second workflow
- runtime parity expectations across `esm_loader`, Python, Rust, and Go

If your AI client supports repo-local skills, this is the file it should read first.

### `agents/openai.yaml`

This is a small metadata stub for OpenAI-style clients.

It provides:

- a display name
- a short description
- a default prompt hint

It is not the full workflow guide. The real instructions still live in `SKILL.md`.

### `references/`

These files hold deeper reference material used by the skill:

- `cli-mcp-reference.md`: exact CLI commands, MCP mapping, project resolution, templates
- `compatibility-guidelines.md`: runtime boundaries and compatibility rules
- `native-elements.md`: guidance for native elements such as `terminal` and `logview`
- `supported-api-reference.md`: the currently supported API surface for MBink UI work

## How to use it with AI tools

### If your client supports skills

1. Open the MBink repository.
2. Ask the agent to use the `mbink-ui-dev` skill.
3. Let it read `tools/mbink_ui_dev/skills/mbink-ui-dev/SKILL.md` before editing.
4. Drive project work through `mbink-ui-dev.exe` first.
5. Drop to `esm_loader`, Python, Rust, or Go only when you need host-level proof.

### If your client does not support repo-local skills

You can still use the same guidance manually:

1. Read `tools/mbink_ui_dev/skills/mbink-ui-dev/SKILL.md`.
2. Point your agent at that file, or paste the relevant instructions into its context.
3. Follow the same CLI loop with `mbink-ui-dev.exe`.

## Relationship to MBink runtime layers

The skillbook is about workflow, not a separate runtime.

- `mbink-ui-dev.exe`: primary project workflow surface for open/build/snapshot/query/inspect/MCP
- `mbink.dll`: runtime core
- `mbink_devtools.dll`: optional development companion for live snapshot/control and runtime HTTP MCP
- `esm_loader.exe`: thinnest manual host around `mbink.dll`

That means the skillbook does not replace the manual DLL-facing model. It explains how to use the higher-level tool first, then verify the lower layers when needed.

## Fast start

Build the tools:

```powershell
cmake -B build
cmake --build build --config Release --target mbink_ui_dev esm_loader -- /m:1
```

Run the smallest verified workflow:

```powershell
build\bin\Release\mbink-ui-dev.exe open --project "examples\todo_app_js"
build\bin\Release\mbink-ui-dev.exe snapshot --project "examples\todo_app_js"
build\bin\Release\mbink-ui-dev.exe query "#todo-input" --project "examples\todo_app_js"
```

If you want a manual runtime check instead:

```powershell
build\bin\Release\esm_loader.exe examples\todo_app_js\app.js
```

## When to read this guide

Use this page when you want to understand:

- where the MBink skillbook lives
- what files are part of it
- how skill-aware AI tooling should enter the repository
- how `mbink-ui-dev`, `mbink.dll`, `mbink_devtools.dll`, and `esm_loader` fit together

## Related docs

- [Quick Start](QUICKSTART.md)
- [AI Workflow](AI_WORKFLOW.md)
- [Tools Overview](../tools/README.md)
- [Contributor Automation Notes](../AGENTS.md)
