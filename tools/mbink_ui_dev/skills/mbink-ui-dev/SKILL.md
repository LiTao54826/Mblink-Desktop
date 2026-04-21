---
name: mbink-ui-dev
description: Operate the MBink UI development toolchain through `mbink-ui-dev` CLI or MCP to initialize projects, open runtimes, inspect live snapshots, query and inspect UI elements, click or input or scroll or highlight the live UI, read or write project files, build, watch, reload the runtime, diagnose console or JS errors, and read MCP resources for MBink UI automation. Use when Codex needs to create, debug, test, or iterate on an MBink UI project, when a user mentions `mbink-ui-dev`, MBink UI snapshots, daemon or watch workflows, selector-based UI diagnosis, or when Claude Desktop or Cursor needs MCP access to an MBink UI project.
---

# MBink UI Dev

## Overview

Use `mbink-ui-dev` as the primary control surface for MBink UI development. Treat the latest progress described in `MBINK_UI_DEV_TOOL_DESIGN.md` as authoritative: P2 is complete, the P3 precision-control loop is now available, and ecosystem items such as HTTP+SSE transport, MCP notifications, and VS Code preview are still future work.

## Choose the Control Surface

- Use the CLI when the agent has shell access and wants direct JSON responses.
- Use MCP when the agent benefits from structured tools, resources, session-scoped `active_project`, or IDE integration.
- Treat daemon state as project-scoped. Open the project first, then keep all observation, edit, build, and interaction steps routed through that same project.
- In multi-project situations, pass `--project <abs-path>` or call `open_project` explicitly instead of guessing.

## Follow the Standard Development Loop

1. Open the project through `mbink-ui-dev open` or MCP `open_project`.
2. Read project state through `info` or `get_project_info`.
3. Observe the live UI through `snapshot` or `snapshot_ui`.
4. Locate the exact target through `query_element` instead of guessing selectors.
5. Diagnose the target through `inspect` when layout, style, or component state matters.
6. Read or write source files through `read` and `write` or `read_file` and `write_file`.
7. Build through `build`, usually with watch enabled during tight iteration loops.
8. Reload through `reload` after builds. Current documented behavior is fixed `restart_runtime`, even though the design keeps `css` and `remount` and `restart` as future semantics.
9. Use `click`, `input_text`, `scroll`, and `highlight` to validate interactions or spotlight the exact element under review.
10. Re-run `snapshot_ui`, `query_element`, `get_console_logs`, and `get_js_errors` to verify the effect.

## Prefer These Tool Priorities

- Call `snapshot_ui` before acting so the UI state is grounded in real data.
- Use `query_element` to locate and `inspect` to diagnose. Do not debug by selector guesswork alone.
- Prefer dedicated UI control tools over `eval_js` for locate, inspect, click, input, scroll, and highlight actions.
- Prefer source edits plus build plus reload over `eval_js` patches.
- Treat `eval_js` as an escape hatch for one-off checks, temporary instrumentation, or state probes that do not deserve a source edit.
- After interactions, re-check with `snapshot_ui` or `query_element`. The documented P3 path includes post-interaction resampling and snapshot timing fixes, so immediate verification is expected to work.
- Do not reload a failed build. Fix build errors first.
- Use logs and JS errors as supporting evidence, not as a replacement for snapshots.

## Bootstrap Projects and Templates Deliberately

- Use `init` or `init_project` to scaffold new projects.
- CLI `mbink-ui-dev init` now accepts an optional target directory. `mbink-ui-dev init` scaffolds into the current directory, while `mbink-ui-dev init my-app` scaffolds into `my-app`.
- Choose from `preact-jsx`, `preact-ts`, `vanilla-js`, `python`, `go`, or `rust`.
- Open the new project immediately after init and verify the initial runtime with a snapshot.
- Templates are embedded into `mbink-ui-dev.exe` at build time, so distribution does not depend on shipping a separate `tools/mbink_ui_dev/templates` directory.
- Host starters generate a multi-file frontend scaffold under `ui/app.js` plus host-specific source trees such as `src/*`, `cmd/*`, or `rust/src/*`, depending on the selected template.
- Expect `preact-jsx` and `preact-ts` projects to import official `preact` / `preact/hooks` modules directly; do not assume global `Preact` or `PreactHooks` are injected.
- Read [cli-mcp-reference.md](references/cli-mcp-reference.md) when template choice, config layout, or generated structure matters.

## Use File and Interaction Operations Intentionally

- Prefer `write` with stdin or `--from` for substantial file content.
- Use `write_file` for project-relative MCP edits that should trigger the normal dev loop.
- Use `click`, `input_text`, and `scroll` to validate UI behavior after edits instead of assuming the component now works.
- Use `highlight` when the user or another agent needs visual disambiguation of the selected element.

## Use MCP Resources and Ecosystem Integrations

- Use `resources/read` for stable, read-only project state such as `ui://snapshot`, `ui://console_logs`, `ui://build_status`, `project://file_tree`, `project://config`, and `project://file/{path}`.
- Prefer session-scoped `active_project` in multi-step MCP work so later tool calls and resource reads stay aligned.
- Use stdio MCP by default. Treat HTTP plus SSE transport, MCP notifications, additional ecosystem integrations, and VS Code side-panel preview as future-stage items, not current assumptions.
- When current behavior and long-term design differ, prefer the document's explicit "当前实现" notes over the future-facing design direction.

## Use This Reference When Needed

- Read [cli-mcp-reference.md](references/cli-mcp-reference.md) for exact CLI commands, MCP tool mapping, resource URIs, project resolution rules, templates, and current versus future capability boundaries.
