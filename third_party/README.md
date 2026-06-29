# Third-Party Dependencies

This directory contains a mix of tracked third-party source, bootstrap targets,
and locally prepared dependencies. Do not assume every directory is complete or
required for every build configuration.

## Tracked in this repository

The current public tree includes these dependency sources directly:

- `lexbor/` - Lexbor, Apache-2.0
- `nlohmann/` - nlohmann/json single-header library, MIT
- `preact/` - Preact ESM assets, MIT
- `stb/` - stb headers, MIT or public domain

See [../THIRD_PARTY_NOTICES.md](../THIRD_PARTY_NOTICES.md) for notice details.

## Bootstrapped by script

Run from the repository root:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\download_deps.ps1 -NonInteractive
```

The script prepares:

- `quickjs/` from QuickJS-ng at the pinned script commit
- `SDL3/` from libsdl-org/SDL at the pinned script commit

The pinned bootstrap metadata is also recorded in `../third_party.lock.json`.
CI checks that this lock file, `../scripts/download_deps.ps1`, and
`../THIRD_PARTY_NOTICES.md` stay in sync.

Existing directories are left untouched unless `-Force` is passed.

## Prepared separately

The default MBlink build currently expects Skia libraries at:

```text
third_party\skia\Debug\out\Debug-windows-x64\skia.lib
third_party\skia\Release\out\Release-windows-x64\skia.lib
```

The bootstrap script reports this requirement but does not fetch or fabricate a
Skia tree. Release artifacts should record Skia provenance before publication.
Use `third_party\skia\SKIA_PROVENANCE.json` and validate it with
`scripts\check_skia_provenance.ps1` during release prep.

## Non-claims

This document does not claim that:

- all third-party dependencies are pinned in one lockfile
- all dependency setup paths are validated on every platform
- every directory under `third_party/` is required for every build
- a fresh checkout can complete the default native build without prepared Skia
