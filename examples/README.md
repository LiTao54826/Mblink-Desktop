# Examples

The examples directory contains runnable projects and historical validation
targets. Treat examples as evidence-backed only when they have been checked with
the current build.

## Recommended first examples

| Example | Purpose | Suggested check |
| --- | --- | --- |
| `todo_app_js` | Smallest practical `mblink-ui-dev` starter | `open`, `snapshot`, `query` |
| `modern_desktop_demo` | Desktop-style app shell and layout | `open`, screenshot snapshot |
| `terminal_logview_demo` | Native terminal/log view elements | `open`, inspect relevant elements |
| `component_demo` | Smaller UI component combinations | `snapshot` |
| `markdown_preview_compat` | Markdown preview workflow with safe rendering and chat pressure | `examples\markdown_preview_compat\verify.ps1` |
| `official_preact_jsx_dev` | Preact ESM path reference | build/open after `esbuild` setup |
| `leafer_ui_showcase` | Interactive npm `leafer-ui` canvas package check | `tests/regression/test_leafer_ui_showcase.ps1` |

## Standard smoke commands

After building `mblink-ui-dev`:

```powershell
build\bin\Release\mblink-ui-dev.exe open --project "examples\todo_app_js"
build\bin\Release\mblink-ui-dev.exe snapshot --project "examples\todo_app_js"
build\bin\Release\mblink-ui-dev.exe snapshot --project "examples\todo_app_js" --response file --include-screenshot
```

For manual runtime checks:

```powershell
build\bin\Release\esm_loader.exe examples\todo_app_js\app.js
```

## Updating examples

When changing an example, include:

- exact command used
- snapshot or screenshot evidence for UI changes
- any dependency assumptions such as `esbuild`
- whether the example uses the supported Preact ESM path or an experimental path

Do not present an example as a compatibility guarantee for an entire framework or
package ecosystem unless that has dedicated verification.
