#ifndef MBLINK_LAYOUT_ENGINE_H
#define MBLINK_LAYOUT_ENGINE_H

#include <memory>
#include "native_layout_engine.h"

// Forward declarations
namespace mblink {
class RenderObject;
struct ComputedStyle;
}

namespace mblink {

/**
 * @brief Layout engine using native C++ implementation
 *
 * This class wraps NativeLayoutEngine to provide CSS layout computation.
 * It supports Block, Flexbox, Grid, and IFC layouts according to W3C specifications.
 *
 * The implementation is translated from Taffy (Rust) to pure C++.
 */
class LayoutEngine {
public:
    LayoutEngine() = default;
    ~LayoutEngine() = default;

    // Prevent copying
    LayoutEngine(const LayoutEngine&) = delete;
    LayoutEngine& operator=(const LayoutEngine&) = delete;

    void BuildLayoutTree(std::shared_ptr<RenderObject> root, bool force_rebuild = false) {
        native_engine_.BuildLayoutTree(root, force_rebuild);
    }

    void ComputeLayout(float available_width, float available_height) {
        native_engine_.ComputeLayout(available_width, available_height);
    }

    bool ComputeIncrementalLayout(float available_width, float available_height) {
        return native_engine_.ComputeIncrementalLayout(available_width, available_height);
    }

    void GetLayoutInfo(std::shared_ptr<RenderObject> root) {
        native_engine_.GetLayoutInfo(root);
    }

    void UpdateStyle(RenderObject* render_obj, const ComputedStyle& style) {
        native_engine_.UpdateStyle(render_obj, style);
    }

    void MarkNeedsLayout(RenderObject* render_obj) {
        native_engine_.MarkNeedsLayout(render_obj);
    }

    /**
     * @brief Update content version for a render object
     * 
     * Called when content changes (text, children, or layout-affecting style).
     * Generates a new version number and propagates dirty marks to ancestors.
     * 
     * @param render_obj The render object whose content changed
     * 
     * **Feature: incremental-layout-optimization**
     * **Validates: Requirements 1.1, 1.2, 1.3**
     */
    void UpdateContentVersion(RenderObject* render_obj) {
        native_engine_.UpdateContentVersion(render_obj);
    }

    void AddElement(RenderObject* render_obj, RenderObject* parent, size_t insert_index = SIZE_MAX) {
        native_engine_.AddElement(render_obj, parent, insert_index);
    }

    void RemoveElement(RenderObject* render_obj) {
        native_engine_.RemoveElement(render_obj);
    }

    void Clear() {
        native_engine_.Clear();
    }

    bool HasElement(RenderObject* render_obj) const {
        return native_engine_.HasElement(render_obj);
    }

    /**
     * @brief Get the underlying native layout engine
     * @return Pointer to the native layout engine
     */
    NativeLayoutEngine* GetNativeEngine() {
        return &native_engine_;
    }

private:
    NativeLayoutEngine native_engine_;
};

} // namespace mblink

#endif // MBLINK_LAYOUT_ENGINE_H
