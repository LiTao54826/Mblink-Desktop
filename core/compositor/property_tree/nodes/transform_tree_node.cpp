/**
 * @file transform_tree_node.cpp
 * @brief 变换树节点实现
 */

#include "core/compositor/property_tree/nodes/transform_tree_node.h"
#include <cmath>

namespace mbink {

TransformTreeNode::TransformTreeNode() {
    // Default to identity transform
    type_ = PropertyTransformType::kIdentity;
}

void TransformTreeNode::SetMatrix(const SkM44& matrix) {
    if (matrix_ == matrix) return;
    
    matrix_ = matrix;
    UpdateTransformType();
    InvalidateAccumulatedTransformCache();
    MarkDirty();
}

void TransformTreeNode::SetIdentity() {
    if (type_ == PropertyTransformType::kIdentity) return;
    
    matrix_ = SkM44();  // Default constructor is identity matrix
    type_ = PropertyTransformType::kIdentity;
    InvalidateAccumulatedTransformCache();
    MarkDirty();
}

void TransformTreeNode::SetOrigin(const SkV3& origin) {
    if (origin_.x == origin.x && origin_.y == origin.y && origin_.z == origin.z) return;
    
    origin_ = origin;
    InvalidateAccumulatedTransformCache();
    MarkDirty();
}

void TransformTreeNode::SetFlatten(bool flatten) {
    if (flatten_ == flatten) return;
    
    flatten_ = flatten;
    InvalidateAccumulatedTransformCache();
    MarkDirty();
}

bool TransformTreeNode::Is2DTransform() const {
    switch (type_) {
        case PropertyTransformType::kIdentity:
        case PropertyTransformType::k2DTranslation:
        case PropertyTransformType::k2DScale:
        case PropertyTransformType::k2DRotation:
        case PropertyTransformType::k2DAffine:
        case PropertyTransformType::kScrollTranslation:
            return true;
        case PropertyTransformType::k3DTransform:
        case PropertyTransformType::kPerspective:
            return false;
    }
    return true;
}

SkM44 TransformTreeNode::GetAccumulatedTransform() const {
    // 检查缓存
    if (accumulated_transform_valid_) {
        return cached_accumulated_transform_;
    }
    
    // 计算累积变换
    if (IsRoot()) {
        // 根节点：应用原点变换
        if (origin_.x != 0 || origin_.y != 0 || origin_.z != 0) {
            SkM44 result = SkM44::Translate(origin_.x, origin_.y, origin_.z);
            result = result * matrix_;
            result = result * SkM44::Translate(-origin_.x, -origin_.y, -origin_.z);
            cached_accumulated_transform_ = result;
        } else {
            cached_accumulated_transform_ = matrix_;
        }
    } else {
        // 非根节点：父节点累积变换 * 本地变换
        SkM44 parent_transform = parent_->GetAccumulatedTransform();
        
        // 如果需要扁平化，将父变换投影到 2D
        if (flatten_ && parent_->Is3DTransform()) {
            // 简化：将 z 分量设为 0
            // 完整实现需要更复杂的投影
        }
        
        // 应用原点变换
        SkM44 local_transform;
        if (origin_.x != 0 || origin_.y != 0 || origin_.z != 0) {
            local_transform = SkM44::Translate(origin_.x, origin_.y, origin_.z);
            local_transform = local_transform * matrix_;
            local_transform = local_transform * SkM44::Translate(-origin_.x, -origin_.y, -origin_.z);
        } else {
            local_transform = matrix_;
        }
        
        cached_accumulated_transform_ = parent_transform * local_transform;
    }
    
    accumulated_transform_valid_ = true;
    return cached_accumulated_transform_;
}

SkM44 TransformTreeNode::GetTransformTo(const TransformTreeNode* target) const {
    if (!target) {
        return GetAccumulatedTransform();
    }
    
    if (target == this) {
        return SkM44();  // 单位矩阵
    }
    
    // 找到公共祖先
    const TransformTreeNode* common = FindCommonAncestor(target);
    if (!common) {
        // 没有公共祖先，返回单位矩阵
        return SkM44();
    }
    
    // 计算从 this 到 common 的变换
    SkM44 to_common;  // 默认是单位矩阵
    const TransformTreeNode* node = this;
    while (node != common) {
        SkM44 local_transform;
        if (node->origin_.x != 0 || node->origin_.y != 0 || node->origin_.z != 0) {
            local_transform = SkM44::Translate(node->origin_.x, node->origin_.y, node->origin_.z);
            local_transform = local_transform * node->matrix_;
            local_transform = local_transform * SkM44::Translate(-node->origin_.x, -node->origin_.y, -node->origin_.z);
        } else {
            local_transform = node->matrix_;
        }
        to_common = local_transform * to_common;
        node = node->parent_;
    }
    
    // 计算从 common 到 target 的变换（需要逆变换）
    SkM44 from_common;  // 默认是单位矩阵
    node = target;
    while (node != common) {
        SkM44 local_transform;
        if (node->origin_.x != 0 || node->origin_.y != 0 || node->origin_.z != 0) {
            local_transform = SkM44::Translate(node->origin_.x, node->origin_.y, node->origin_.z);
            local_transform = local_transform * node->matrix_;
            local_transform = local_transform * SkM44::Translate(-node->origin_.x, -node->origin_.y, -node->origin_.z);
        } else {
            local_transform = node->matrix_;
        }
        from_common = local_transform * from_common;
        node = node->parent_;
    }
    
    // 尝试求逆
    SkM44 from_common_inv;
    if (!from_common.invert(&from_common_inv)) {
        // 矩阵不可逆，返回单位矩阵
        return SkM44();
    }
    
    return from_common_inv * to_common;
}

void TransformTreeNode::InvalidateAccumulatedTransformCache() {
    accumulated_transform_valid_ = false;
    
    // 递归使子节点缓存失效
    for (TransformTreeNode* child : children_) {
        child->InvalidateAccumulatedTransformCache();
    }
}

void TransformTreeNode::UpdateTransformType() {
    // Check if identity matrix
    SkM44 identity;  // Default constructor is identity matrix
    if (matrix_ == identity) {
        type_ = PropertyTransformType::kIdentity;
        return;
    }
    
    // Get matrix elements
    float m[16];
    matrix_.getColMajor(m);
    
    // Check for perspective components
    if (m[3] != 0 || m[7] != 0 || m[11] != 0 || m[15] != 1) {
        type_ = PropertyTransformType::kPerspective;
        return;
    }
    
    // Check for 3D components
    bool has_3d = (m[2] != 0 || m[6] != 0 || m[8] != 0 || m[9] != 0 || 
                   m[10] != 1 || m[14] != 0);
    if (has_3d) {
        type_ = PropertyTransformType::k3DTransform;
        return;
    }
    
    // 2D transform analysis
    bool has_translation = (m[12] != 0 || m[13] != 0);
    bool has_scale = (m[0] != 1 || m[5] != 1);
    bool has_skew = (m[1] != 0 || m[4] != 0);
    
    if (!has_translation && !has_scale && !has_skew) {
        type_ = PropertyTransformType::kIdentity;
    } else if (has_translation && !has_scale && !has_skew) {
        type_ = PropertyTransformType::k2DTranslation;
    } else if (!has_translation && has_scale && !has_skew) {
        type_ = PropertyTransformType::k2DScale;
    } else if (has_skew) {
        // Check if pure rotation
        float a = m[0], b = m[1], c = m[4], d = m[5];
        float det = a * d - b * c;
        if (std::abs(det - 1.0f) < 0.0001f && 
            std::abs(a - d) < 0.0001f && 
            std::abs(b + c) < 0.0001f) {
            type_ = PropertyTransformType::k2DRotation;
        } else {
            type_ = PropertyTransformType::k2DAffine;
        }
    } else {
        type_ = PropertyTransformType::k2DAffine;
    }
}

} // namespace mbink
