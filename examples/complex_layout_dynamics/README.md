# Complex Layout Dynamics

This MBlink UI-dev example is a runtime regression target for complex desktop layouts.

It combines:

- a fixed shell with sidebar, topbar, scrollable main stage, and scrollable right rail
- a dense data grid with filtering, row selection, row insertion, and density changes
- a kanban board with dynamic card insertion and lane movement
- a form/editor surface with textarea, select, checkbox, and drawer toggling
- an analytics view with chart/state switches
- modal, toast, and scroll interactions

Run the verification script from the repository root:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File examples\complex_layout_dynamics\verify.ps1
```

The script opens the real `mblink-ui-dev.exe` runtime, captures screenshots and snapshots, exercises dynamic changes, checks selector geometry, samples idle CPU, and writes evidence under `tmp\complex_layout_dynamics\`.
