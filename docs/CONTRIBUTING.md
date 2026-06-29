# Contributing

Thank you for taking MBlink seriously enough to contribute. The project is early,
Windows-first, and intentionally careful about public claims.

## Good first contribution areas

- Bug fixes with reproducible cases
- Build stability improvements
- Dependency/setup documentation fixes
- Example validation and cleanup
- Snapshot-based `mblink-ui-dev` workflow improvements
- Focused tests or regression assets

## Before you start

Check whether your change affects:

1. module boundaries
2. build entry points or dependencies
3. public C API or language bindings
4. examples or tests
5. user-visible runtime behavior
6. public docs that new contributors will trust

For MBlink-specific automation, read:

- [../AGENTS.md](../AGENTS.md)
- [AI Workflow](AI_WORKFLOW.md)
- [tools/mblink_ui_dev/skills/mblink-ui-dev/SKILL.md](../tools/mblink_ui_dev/skills/mblink-ui-dev/SKILL.md)

## Basic workflow

```powershell
git checkout -b feature/your-change
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\download_deps.ps1 -NonInteractive
cmake -B build
cmake --build build --config Release --target mblink_ui_dev esm_loader -- /m:1
```

The default build requires prepared Skia libraries. See [Build](BUILD.md) before
opening a build issue from a fresh checkout.

## Validation

For docs/support changes:

```powershell
git diff --check
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\check_dependency_lock.ps1
py -3 scripts/check_code_structure_baseline.py
py -3 scripts/check_circular_deps.py
```

`scripts/check_code_structure.py` currently reports known source-size debt in
existing runtime files, so treat it as an advisory report unless your change
touches those areas.

The hard baseline for oversized source files lives in
`scripts/code_structure_baseline.json`. Updating it should be treated as a
reviewable architecture decision, not routine cleanup.

For UI/tooling changes, prefer runtime evidence:

```powershell
build\bin\Release\mblink-ui-dev.exe snapshot --project "examples\todo_app_js" --response file --include-screenshot
```

For test-related changes:

```powershell
cmake -B build -DMBLINK_BUILD_TESTS=ON
cmake --build build --config Release
ctest --test-dir build --output-on-failure -C Release
```

## Pull request checklist

- Describe what changed
- Explain why the change is needed
- Include exact validation commands and outcomes
- Mention impact on build, tests, bindings, examples, docs, or public interfaces
- Avoid broad refactors mixed into behavior changes
- Keep generated files and local build output out of the PR

## Documentation rules

- Do not describe placeholder directories as supported features
- Do not treat historical plans as current project state
- Mark unverified capabilities clearly
- Keep claims aligned with current source, build scripts, and fresh verification
- If Skia, QuickJS-ng, SDL3, or bundled third-party code is affected, update
  [Third-Party Notices](../THIRD_PARTY_NOTICES.md) when needed
