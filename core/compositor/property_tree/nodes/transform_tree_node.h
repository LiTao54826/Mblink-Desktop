/**
 * @file transform_tree_node.h
 * @brief Transform tree node
 *
 * Transform tree node manages element's 2D/3D transform properties:
 * - 4x4 transform matrix (supports 3D transforms)
 * - Transform origin
 * - Flatten flag (flatten into 2D)
 * - Scroll transform support
 *
 * Reference: Chromium Blink cc/trees/transform_node.h
 */

#pragma once

#include "core/compositor/property_tree/nodes/property_tree_node.h"
#include "include/core/SkM44.h"
#include "include/core/SkPoint3.h"

namespace lightui {

// Forward declarations
class RenderObject;
class ScrollTreeNode;

/**
 * @brief Property tree transform type
 * Used for optimization: different transform types can use different computation paths
 */
enum class PropertyTransformType {
    kIdentity,          // Identity transform (no transform)
    k2DTranslation,     // 2D translation
    k2DScale,           // 2D scale
    k2DRotation,        // 2D rotation
    k2DAffine,          // 2D affine transform
    k3DTransform,       // 3D transform
    kPerspective,       // Perspective
    kScrollTranslation, // Scroll translation
};

/**
 * @brief Transform tree node
 *
 * Stores element's transform information, supports:
 * - Accumulated transform computation (from root to current node)
 * - Inter-node transform computation
 * - Direct property update (without triggering re-rasterization)
 */
class TransformTreeNode : public PropertyTreeNode<TransformTreeNode> {
public:
    TransformTreeNode();
    ~TransformTreeNode() override = default;

    // =========================================================================
    // Transform matrix
    // =========================================================================

    /**
     * @brief Get local transform matrix (4x4)
     */
    const SkM44& GetMatrix() const { return matrix_; }

    /**
     * @brief Set local transform matrix
     * @param matrix New transform matrix
     */
    void SetMatrix(const SkM44& matrix);

    /**
     * @brief Set to identity matrix
     */
    void SetIdentity();

    /**
     * @brief Check if identity transform
     */
    bool IsIdentity() const { return type_ == PropertyTransformType::kIdentity; }

    // =========================================================================
    // Transform origin
    // =========================================================================

    /**
     * @brief Get transform origin
     */
    const SkV3& GetOrigin() const { return origin_; }

    /**
     * @brief Set transform origin
     * @param origin New transform origin
     */
    void SetOrigin(const SkV3& origin);

    /**
     * @brief Set transform origin (2D)
     */
    void SetOrigin(float x, float y) { SetOrigin({x, y, 0}); }

    // =========================================================================
    // Flatten
    // =========================================================================

    /**
     * @brief Should flatten (flatten into 2D)
     * @note Corresponds to CSS transform-style: flat
     */
    bool ShouldFlatten() const { return flatten_; }

    /**
     * @brief Set flatten flag
     */
    void SetFlatten(bool flatten);

    // =========================================================================
    // Transform type
    // =========================================================================

    /**
     * @brief Get transform type
     */
    PropertyTransformType GetTransformType() const { return type_; }

    /**
     * @brief Is scroll translation
     */
    bool IsScrollTranslation() const { return type_ == PropertyTransformType::kScrollTranslation; }

    /**
     * @brief Is 2D transform
     */
    bool Is2DTransform() const;

    /**
     * @brief Is 3D transform
     */
    bool Is3DTransform() const { return type_ == PropertyTransformType::k3DTransform || type_ == PropertyTransformType::kPerspective; }

    // =========================================================================
    // Scroll association
    // =========================================================================

    /**
     * @brief Get associated scroll node
     */
    ScrollTreeNode* GetScrollNode() const { return scroll_node_; }

    /**
     * @brief Set associated scroll node
     */
    void SetScrollNode(ScrollTreeNode* node) { scroll_node_ = node; }

    // =========================================================================
    // Accumulated transform computation
    // =========================================================================

    /**
     * @brief Compute accumulated transform from root to current node
     * @return Accumulated transform matrix
     */
    SkM44 GetAccumulatedTransform() const;

    /**
     * @brief Compute transform from current node to target node
     * @param target Target node
     * @return Transform matrix, returns identity if cannot compute
     */
    SkM44 GetTransformTo(const TransformTreeNode* target) const;

    /**
     * @brief Invalidate accumulated transform cache
     */
    void InvalidateAccumulatedTransformCache();

    // =========================================================================
    // Direct update support
    // =========================================================================

    /**
     * @brief Can directly update (without affecting layer structure)
     * @note If element has own layer, transform update doesn't need re-rasterization
     */
    bool CanDirectlyUpdate() const { return can_directly_update_; }

    /**
     * @brief Set can directly update
     */
    void SetCanDirectlyUpdate(bool can) { can_directly_update_ = can; }

    // =========================================================================
    // RenderObject association
    // =========================================================================

    /**
     * @brief Get associated RenderObject
     */
    RenderObject* GetRenderObject() const { return render_object_; }

    /**
     * @brief Set associated RenderObject
     */
    void SetRenderObject(RenderObject* obj) { render_object_ = obj; }

private:
    /**
     * @brief Update transform type based on matrix content
     */
    void UpdateTransformType();

    // Local transform matrix
    SkM44 matrix_;
    
    // Transform origin
    SkV3 origin_ = {0, 0, 0};
    
    // Flatten flag
    bool flatten_ = true;
    
    // Transform type
    PropertyTransformType type_ = PropertyTransformType::kIdentity;
    
    // Associated scroll node
    ScrollTreeNode* scroll_node_ = nullptr;
    
    // Associated RenderObject
    RenderObject* render_object_ = nullptr;
    
    // Can directly update
    bool can_directly_update_ = false;
    
    // Cached accumulated transform
    mutable SkM44 cached_accumulated_transform_;
    mutable bool accumulated_transform_valid_ = false;
};

} // namespace lightui
