# MBink Incremental Rendering Implementation Plan

## 1. Objective
Achieve "Enterprise-Grade" incremental rendering performance for MBink. The goal is to minimize CPU usage and drawing commands by only processing elements that actually change or are visible.

**Success Criteria:**
1.  **Layout**: Modifying a style property (e.g., `width`) only re-layouts the affected subtree, not the entire document.
2.  **Tree Structure**: DOM Node insertion/removal updates the Render Tree locally, without a full rebuild.
3.  **Painting**: Off-screen elements are aggressively culled (not painted).
4.  **Drawing**: Only dirty regions are rasterized (already partially implemented).

---

## 2. Current State Analysis
-   **Partial Dirty Rects**: Supported. The system tracks dirty rects and expects to use `clipRect` for incremental painting.
-   **View Culling**: Partially implemented (recently added `quickReject`), but needs comprehensive coverage.
-   **Incremental Layout**: **Disabled/Broken**. Code comments explicitly state "Incremental layout has bugs, full layout forced every frame".
-   **Incremental Tree Updates**: **Not Implemented**. `WindowDOMObserver` calls `InvalidateRenderTree()` on any node addition/removal, forcing a full tree rebuild.

---

## 3. Implementation Steps

### Phase 1: Fix & Enable Incremental Layout
**Goal**: Stop re-calculating the entire layout tree when a single element changes.

1.  **Refine Dirty Propagation**:
    -   Ensure `RenderObject::MarkNeedsLayout()` correctly flags the object and potentially its parent chain.
    -   Verify that `MarkNeedsLayout` sets a flag that `LayoutDirtySubtree` can traverse.

2.  **Implement `RenderTreeUpdater::UpdateStyle`**:
    -   Ensure style changes (e.g., `color`, `font-size`) propagate to `RenderObject::SetComputedStyle`.
    -   Differentiate between "Paint Only" changes (color) and "Layout" changes (width, font-size).

3.  **Fix `Window::Render` Logic**:
    -   Remove the "Force Full Layout" fallback block.
    -   Enable the `LayoutDirtySubtree` path.
    -   **Critical**: Ensure `LayoutDirtySubtree` calls the underlying Layout Engine (Taffy or Native) correctly for a specific node.
        -   *Note*: Since MBink uses Taffy for Flexbox, we must ensure Taffy's cache is not unnecessarily cleared.

### Phase 2: Incremental Render Tree Updates
**Goal**: Stop destroying/recreating `RenderObject`s on DOM changes.

1.  **Refactor `WindowDOMObserver::OnNodeAdded`**:
    -   Remove `InvalidateRenderTree()`.
    -   Implement logic to find the parent `RenderObject`.
    -   Create the correct `RenderObject` subclass (Block, Inline, Text, Image) based on the DOM node.
    -   Insert the new `RenderObject` into the parent's children list at the correct index (matching DOM order).
    -   Call `MarkNeedsLayout()` on the *parent*.

2.  **Refactor `WindowDOMObserver::OnNodeRemoved`**:
    -   Remove `InvalidateRenderTree()`.
    -   Find the `RenderObject` associated with the removed DOM node.
    -   Call `RemoveChild` on the parent `RenderObject`.
    -   Call `MarkNeedsLayout()` on the *parent*.

3.  **Handle Special Cases**:
    -   Moving nodes (Reparenting).
    -   `TextNode` content updates (already handled via attribute change, but verify).

### Phase 3: Comprehensive View Culling (Frustum Culling)
**Goal**: Zero CPU time spent on off-screen elements.

1.  **Standardize Culling**:
    -   Apply the `quickReject` logic (added in previous step) to *all* container types (`RenderBlock`, `RenderInline`, `RenderTable`, `RenderFlex`, `RenderGrid`).
    -   Ensure the "safety margin" (e.g., 50px) is consistent and sufficient for shadows/outlines.

2.  **Optimize Long Lists**:
    -   Verify that implicit lists (e.g., long `<ul>` or `<div>` sequences) benefit from this. When the parent paints children, it should check cull rects before calling child `Paint()`. *Improvement*: Add a check in `RenderObject::PaintChildren` loop to skip children whose layout bounds don't intersect the dirty rect/clip.

---

## 4. Execution Plan (Step-by-Step)

| Step | Task | Description |
| :--- | :--- | :--- |
| **01** | **Prepare Test Case** | Create a "Performance Test" HTML with 1000 items and a button to modify one item/add one item. Verify current lag/rebuild behavior using logs. |
| **02** | **Enable Incr Layout** | Modify `Window::Render` to prioritize incremental layout path. Fix `NativeLayoutEngine` or `RenderBlock::Layout` to respect `needs_layout_` flag. |
| **03** | **Debug Layout** | Verify the test case updates correctly without layout glitches. |
| **04** | **Impl View Culling** | Apply `quickReject` to all remaining RenderObjects. |
| **05** | **Impl Tree Updates** | Rewrite `WindowDOMObserver` to manage RenderObjects directly. |

---

## 5. Verification
After implementation, we will use the test case to confirm:
1.  Logs show "Incremental Rendering" mode is active.
2.  Logs show "RenderTree rebuild" count is 0 after initial load.
3.  FPS remains high during DOM manipulation.
