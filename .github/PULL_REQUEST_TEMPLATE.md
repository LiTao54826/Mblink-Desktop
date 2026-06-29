## Summary

<!-- Briefly describe what changed. -->

## Why

<!-- Explain why this change is needed now. -->

## Validation

- [ ] `git diff --check`
- [ ] `powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\check_dependency_lock.ps1`
- [ ] `py -3 scripts/check_code_structure_baseline.py`
- [ ] `py -3 scripts/check_circular_deps.py`
- [ ] Runtime snapshot or build evidence, if this affects runtime/UI behavior

Advisory: run `py -3 scripts/check_code_structure.py` when touching runtime
source or module boundaries. It currently reports known source-size debt.
Changes to `scripts/code_structure_baseline.json` need explicit review rationale.

## Impact

- [ ] docs only
- [ ] build/dependencies
- [ ] runtime behavior
- [ ] public API or bindings
- [ ] examples/tests
- [ ] third-party notices

## Notes

Mention Skia, QuickJS-ng, SDL3, or other third-party provenance changes here.
