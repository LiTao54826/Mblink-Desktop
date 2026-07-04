# Leafer UI Showcase

This example is an interactive runtime playground for the npm `leafer-ui` canvas
library on MBlink. The downloaded package runtime is vendored at
`js/leafer-ui/web.module.min.js`, renders a Leafer stage, and exposes stable
selectors for snapshot and click verification.

```powershell
build\bin\Release\mblink-ui-dev.exe build --project examples\leafer_ui_showcase
powershell -NoProfile -ExecutionPolicy Bypass -File .\tests\regression\test_leafer_ui_showcase.ps1
```

The regression captures initial and post-interaction snapshots/screenshots plus
`console.json` and `errors.json`.
