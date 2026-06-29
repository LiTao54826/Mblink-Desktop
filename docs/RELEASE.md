# Release

This is the current release-readiness checklist for MBlink. It is intentionally
conservative because the public dependency story is still Windows-first and
Skia-prepared.

## Release channels

Use GitHub releases when the repository is public. A release should include:

- source archive
- Windows build artifacts, if available
- release notes with validation commands
- third-party notice/provenance updates

The required release automation entry is
[`.github/workflows/release-readiness.yml`](../.github/workflows/release-readiness.yml).
It runs on `v*` tags and manual dispatch, and fails if Skia provenance or listed
Skia libraries are missing.

## Pre-release checks

From the repository root:

```powershell
git status --short
git diff --check
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\check_dependency_lock.ps1
py -3 scripts/check_code_structure_baseline.py
py -3 scripts/check_circular_deps.py
```

Also run `py -3 scripts/check_code_structure.py` and record the current advisory
output. Do not make it a release blocker until the known source-size baseline has
an owner.

Prepare dependencies:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\download_deps.ps1 -NonInteractive
```

Build the public onboarding targets after Skia has been prepared:

```powershell
cmake -B build
cmake --build build --config Release --target mblink_ui_dev esm_loader -- /m:1
```

Capture runtime evidence:

```powershell
build\bin\Release\mblink-ui-dev.exe snapshot --project "examples\todo_app_js" --response file --include-screenshot
```

## Skia gate

Do not publish binary artifacts until Skia provenance is recorded. The default
build expects:

```text
third_party\skia\Debug\out\Debug-windows-x64\skia.lib
third_party\skia\Release\out\Release-windows-x64\skia.lib
```

Release notes should include the Skia source revision, build flags, artifact
origin, and any required notices.

Create `third_party\skia\SKIA_PROVENANCE.json` before packaging binaries:

```json
{
  "component": "skia",
  "sourceUrl": "https://skia.googlesource.com/skia.git",
  "sourceRevision": "40-character upstream commit SHA",
  "buildConfiguration": "Debug/Release windows-x64 flags",
  "artifactOrigin": "local build, CI artifact, or trusted prebuilt source",
  "licenseFiles": [
    "third_party/skia/LICENSE"
  ],
  "debugLibrary": "third_party/skia/Debug/out/Debug-windows-x64/skia.lib",
  "releaseLibrary": "third_party/skia/Release/out/Release-windows-x64/skia.lib",
  "debugLibrarySha256": "64-character SHA256 hex digest",
  "releaseLibrarySha256": "64-character SHA256 hex digest"
}
```

Validate it during release prep:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\check_skia_provenance.ps1 -RequireLibraries
```

The release-readiness workflow runs the same gate on release tags. A release
branch or tag must make the listed Skia files and provenance available to the
runner before binary artifacts are published.

## Versioning

Until the project has stable public API guarantees, use pre-1.0 tags such as:

```text
v0.x.y
```

Prefer small releases with clear validation evidence over broad releases that
claim unverified platform or framework support.

## What not to claim yet

- Linux or macOS release support
- Full React compatibility
- Complete browser/WebView compatibility
- Fresh-checkout default native build without prepared Skia
- Node.js runtime compatibility in UI code
