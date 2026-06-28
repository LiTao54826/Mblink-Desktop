# UI Combinations Showcase Notes

This example is a compact MBlink desktop UI regression target. It intentionally combines common controls in one screen so development agents can verify layout, input, state changes, scroll containers, overlays, and toast feedback through `mblink-ui-dev` or the `esm_loader` UI-dev hooks.

## What It Covers

- Application shell with sidebar, topbar, toolbar buttons, and internal scroll regions.
- Metrics, repeated cards, a table, tabs, segmented controls, badges, and selected rows.
- Text input, read-only input, select controls, textarea, checkbox, disabled buttons, and form validation.
- Loading, empty, error, and normal content states.
- Modal dialog, toast stack, and scrollable activity feed.
- Stable selectors for automation: `#combo-root`, `#global-search`, `#queue-table`, `#request-name`, `#request-create`, `#state-preview`, `#settings-dialog`, and `#activity-feed`.

## Verification Loop

Preferred direct C API runtime check:

```powershell
powershell -ExecutionPolicy Bypass -File examples\ui_combinations_showcase\verify.ps1
```

Official `mblink-ui-dev` CLI check with copied snapshot and screenshot evidence:

```powershell
powershell -ExecutionPolicy Bypass -File examples\ui_combinations_showcase\verify_cli.ps1
```

Manual snapshot check:

```powershell
build-tests\bin\Debug\esm_loader.exe examples\ui_combinations_showcase\app.js `
  --width 1180 --height 760 --title ui_combinations_showcase `
  --ui-dev-snapshot-file tmp\ui_combinations_showcase\snapshot.json `
  --ui-dev-snapshot-include-screenshot `
  --ui-dev-screenshot-file tmp\ui_combinations_showcase\screenshot.png `
  --quit 2
```

When `mblink-ui-dev build` is available, the project can also be opened through:

```powershell
build-tests\bin\Debug\mblink-ui-dev.exe open examples\ui_combinations_showcase
build-tests\bin\Debug\mblink-ui-dev.exe snapshot --include-screenshot --project examples\ui_combinations_showcase
```

## Problems Found While Developing

- Running `mblink-ui-dev` from the repository root can be ambiguous when multiple managed projects exist. Pass `--project <absolute-path>` for project-scoped commands.
- `mblink-ui-dev build` requires `esbuild` to be available on `PATH` or inside the project `node_modules`. The direct `esm_loader` UI-dev hook is useful for validating single-file examples when the build tool is not installed.
- Keep page-level scrolling disabled for desktop layouts. Scrollbars should live in the workspace, table, or feed regions that actually overflow.
- Selector and interaction checks are not enough for layout-sensitive UI. The first screenshot showed a table squeezed beside another panel even though `#queue-table` queried successfully, so the table was moved into a full-width panel.
- Scope global form styles carefully. A blanket `input { width: 100% }` made table checkboxes expand across their cells and visually hid the row text; `input[type="checkbox"]` now has explicit stable dimensions.

## Framework Bug Policy

Do not hide valid CSS, DOM, input, paint, or snapshot defects with app-specific styling. If a valid minimal slice fails in MBlink:

1. Reduce the issue to the smallest selector or component in this example or a focused fixture.
2. Verify the mismatch with snapshot, screenshot, query, inspect, logs, and errors.
3. Fix the shared framework/runtime path first: rendering core, C API, devtools command handling, or bindings.
4. Add or update a unit/regression test for the framework behavior.
5. Remove temporary UI workarounds after the framework fix lands.

App-side fallbacks are acceptable only to isolate the failure or keep a demo inspectable while the framework fix is being prepared.
