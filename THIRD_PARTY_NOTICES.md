# Third-Party Notices

MBlink is MIT-licensed. This file summarizes third-party components that are
visible in the public repository or prepared by the public dependency bootstrap.
It is a launch-readiness inventory, not a substitute for reviewing each upstream
license file before publishing binary artifacts.

## Tracked sources

| Component | Location | License | Notes |
| --- | --- | --- | --- |
| Lexbor | `third_party/lexbor` | Apache-2.0 | Includes upstream `LICENSE` and `NOTICE`. |
| nlohmann/json | `third_party/nlohmann/json.hpp` | MIT | Single-header library with SPDX/header notice. |
| Preact | `third_party/preact` | MIT | Repository copy currently identifies as `11.0.0-beta.1`. Upstream: `https://github.com/preactjs/preact`. |
| LeaferJS / leafer-ui | `examples/leafer_ui_showcase/js/leafer-ui` | MIT | Vendored browser ESM bundle and upstream `LICENSE` for Leafer canvas examples. Upstream: `https://github.com/leaferjs/leafer-ui`. |
| stb_image | `third_party/stb/stb_image.h` | MIT or public domain | Header contains upstream license text. |

## Bootstrapped source dependencies

The PowerShell bootstrap script prepares these source dependencies:

| Component | Target | Upstream | License | Pinned ref |
| --- | --- | --- | --- | --- |
| QuickJS-ng | `third_party/quickjs` | `https://github.com/quickjs-ng/quickjs.git` | MIT | `ca0d50dc2991c433f1c265239790e37b38c836b8` |
| SDL3 | `third_party/SDL3` | `https://github.com/libsdl-org/SDL.git` | zlib-style | `b9c790949e4ff6bc26813b437d477da694269ef5` |

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\download_deps.ps1 -NonInteractive
```

`third_party.lock.json` is the machine-readable source for bootstrapped
dependency URL/ref/license metadata. CI checks that this file, the bootstrap
script, and this notice file stay in sync.

## Prepared binary/source dependency

The default build currently requires prepared Skia libraries:

```text
third_party\skia\Debug\out\Debug-windows-x64\skia.lib
third_party\skia\Release\out\Release-windows-x64\skia.lib
```

Skia provenance is not fully captured by the public bootstrap yet. Before a
public binary release, record the exact Skia source revision, build flags,
license files, and artifact origin in the release notes or an updated notice
file.

Release prep should validate `third_party\skia\SKIA_PROVENANCE.json` with:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\check_skia_provenance.ps1 -RequireLibraries
```

## Release checklist

- Confirm third-party license files are included in source archives
- Confirm binary packages include required notices
- Confirm Skia provenance for any binary release
- Confirm bootstrapped dependency refs still match `scripts/download_deps.ps1`
