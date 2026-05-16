# Complex UI Stage 2A Retained Scroll Blocking Evidence

## Scope

Stage 2A measured fallback contraction for scroll ownership / retained-present correctness.

This slice does not relax scroll full-dirty behavior. It tightens the retained-present guard so conservative scroll fallback classes also block reuse of retained main content.

## Change Summary

- Added `UnifiedFrameStats::scroll_retained_present_blocking_fallbacks`.
- Added `RenderPipeline::HasPendingScrollRetainedPresentBlockingFallback()`.
- Changed `Window::Render()` retained-main reuse and invalidation from aggregate full-dirty fallback checks to the explicit retained-present blocking fallback policy.
- Kept existing fallback classification:
  - incremental-eligible fallback: clip-layer full-dirty scrolls
  - conservative fallback: ancestor-layer fallback plus missing-layer-target fallback
  - retained-present blocking fallback: incremental-eligible fallback plus conservative fallback

## Impact Gate

GitNexus impact checks completed before edits:

- `HasPendingScrollFullDirtyFallback`: LOW, direct affected files were `core/window/window.cpp` and `tests/property/compositor/test_render_pipeline_properties.cpp`.
- `ProcessFrame`: LOW, direct affected files were `core/window/window.cpp`, `tests/property/compositor/test_visual_consistency_properties.cpp`, and `tests/property/compositor/test_render_pipeline_properties.cpp`.
- `UnifiedFrameStats`: LOW, no upstream impacted symbols reported.
- `Window::Render` was found by GitNexus context at `core/window/window.cpp:1221`; the CLI could not disambiguate that symbol through `impact`, so the edit stayed within the already-gated retained-present consumer path.

No HIGH or CRITICAL risk warning was returned.

## Runtime Evidence

`tests/scroll_reset_test.html` was run with `MBINK_BASELINE_FRAME_STATS=1`.

Evidence log:

- `.omx/logs/stage2a-scroll-reset-default-20260516.log`

Observed native scroll frames:

```text
scrolls_handled=1
scroll_full_dirty_fallbacks=1
scroll_clip_layer_full_dirty_fallbacks=1
scroll_ancestor_layer_full_dirty_fallbacks=0
scroll_missing_layer_target_fallbacks=0
scroll_incremental_eligible_fallbacks=1
scroll_conservative_fallbacks=0
scroll_retained_present_blocking_fallbacks=1
scroll_last_reason=1
```

Matching window frames reported:

```text
retained_main_scroll_fallback_blocked=1
```

This confirms the new retained-present blocking metric is surfaced in native frame stats and visible at the window boundary.

Control note: a UI Dev DOM `scrollTop` command successfully changed DOM scroll position, but did not exercise the C++ scroll pipeline (`scrolls_handled=0`). It is not used as fallback contraction evidence.

## Verification

- `cmake --build build --config Release --target mbink_property_tests -- /p:TrackFileAccess=false /m:1`
- `node tests/js/window_retained_scroll_fallback_check.js`
- `mbink_property_tests.exe --gtest_filter="*ScrollInvalidationStatsRecordedPerFrame:*ScrollInvalidationStatsSurviveConfigSwitchBeforeFrame:*LegacyScrollInvalidationStatsRecordedPerFrame:*MissingLayerScrollFallbackClassifiedConservative"`
- `mbink_property_tests.exe --gtest_filter="PipelineScrollTest.*:ScrollHandlingTest.*:EdgeCaseTest.ScrollInvalidationStatsRecordMissingLayerFallback:VisualConsistencyTest.*:CompositorTest.*:RasterizerTest.*"`
- `cmake --build build --config Release --target mbink_ui_dev esm_loader -- /p:TrackFileAccess=false /m:1`

All executed gates passed.

Post-review gates:

- Architect verification: APPROVED.
- Deslop pass: scoped to the five changed source/test files; no behavior-preserving cleanup edits were needed after duplicate/dead-code/naming review.
- GitNexus pre-commit check: `npx gitnexus status` reported the index up to date at `dd6214d`; the CLI does not expose the `detect_changes` subcommand in this environment (`unknown command 'detect_changes'`), so the scope was verified by targeted impact checks plus `git diff --name-status`.

## Decision

Do not contract clip-layer full-dirty scroll fallback yet.

The runtime scroll sample shows the fallback is currently `incremental eligible`, but the existing scroll architecture still paints non-layered descendants into the clip layer. Removing the full-dirty mark globally would risk stale pixels. The safe Stage 2A result is to make retained-present blocking explicit and complete, including conservative fallback classes.

Next safe contraction work should first prove a narrow scroll container state where all scroll-moving content is represented by independent child layers, then disable clip-layer full-dirty only for that state.
