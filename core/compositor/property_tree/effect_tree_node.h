/**
 * @file effect_tree_node.h
 * @brief 效果树节点
 *
 * 效果树节点管理元素的视觉效果属性：
 * - opacity（透明度）
 * - filter（滤镜）
 * - backdrop-filter（背景滤镜）
 * - blend-mode（混合模式）
 * - mask（遮罩）
 *
 * 参考 Chromium Blink: cc/trees/effect_node.h
 */

#pragma once

#include "core/compositor/property_tree/property_tree_node.h"
#include "include/core/SkBlendMode.h"
#include "include/core/SkRect.h"
#include <vector>
#include <string>

namespace lightui {

// 前向声明
class RenderObject;
class TransformTreeNode;
class ClipTreeNode;

/**
 * @brief 滤镜操作类型
 */
enum class FilterOperationType {
    kBlur,              // 模糊
    kBrightness,        // 亮度
    kContrast,          // 对比度
    kGrayscale,         // 灰度
    kHueRotate,         // 色相旋转
    kInvert,            // 反色
    kOpacity,           // 透明度
    kSaturate,          // 饱和度
    kSepia,             // 褐色
    kDropShadow,        // 阴影
};

/**
 * @brief 滤镜操作
 */
struct FilterOperation {
    FilterOperationType type;
    float value = 0;        // 主要参数值
    float x = 0, y = 0;     // 用于阴影偏移
    float spread = 0;       // 用于阴影扩展
    uint32_t color = 0;     // 用于阴影颜色

    /**
     * @brief 创建模糊滤镜
     */
    static FilterOperation Blur(float radius) {
        FilterOperation op;
        op.type = FilterOperationType::kBlur;
        op.value = radius;
        return op;
    }

    /**
     * @brief 创建亮度滤镜
     */
    static FilterOperation Brightness(float amount) {
        FilterOperation op;
        op.type = FilterOperationType::kBrightness;
        op.value = amount;
        return op;
    }

    /**
     * @brief 创建对比度滤镜
     */
    static FilterOperation Contrast(float amount) {
        FilterOperation op;
        op.type = FilterOperationType::kContrast;
        op.value = amount;
        return op;
    }

    /**
     * @brief 创建灰度滤镜
     */
    static FilterOperation Grayscale(float amount) {
        FilterOperation op;
        op.type = FilterOperationType::kGrayscale;
        op.value = amount;
        return op;
    }

    /**
     * @brief 创建透明度滤镜
     */
    static FilterOperation Opacity(float amount) {
        FilterOperation op;
        op.type = FilterOperationType::kOpacity;
        op.value = amount;
        return op;
    }
};

/**
 * @brief 效果标志
 */
enum class EffectFlags : uint32_t {
    kNone = 0,
    kHasOpacity = 1 << 0,
    kHasFilter = 1 << 1,
    kHasBackdropFilter = 1 << 2,
    kHasBlendMode = 1 << 3,
    kHasMask = 1 << 4,
    kRequiresIsolation = 1 << 5,  // 需要独立渲染表面
};

inline EffectFlags operator|(EffectFlags a, EffectFlags b) {
    return static_cast<EffectFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline EffectFlags operator&(EffectFlags a, EffectFlags b) {
    return static_cast<EffectFlags>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

inline bool HasFlag(EffectFlags flags, EffectFlags flag) {
    return (flags & flag) != EffectFlags::kNone;
}

/**
 * @brief 效果树节点
 *
 * 存储元素的视觉效果信息，支持：
 * - opacity 直接更新（不触发重新光栅化）
 * - 效果隔离判断
 * - 累积 opacity 计算
 */
class EffectTreeNode : public PropertyTreeNode<EffectTreeNode> {
public:
    EffectTreeNode();
    ~EffectTreeNode() override = default;

    // =========================================================================
    // Opacity
    // =========================================================================

    /**
     * @brief 获取透明度
     */
    float GetOpacity() const { return opacity_; }

    /**
     * @brief 设置透明度
     * @param opacity 透明度值 [0.0, 1.0]
     */
    void SetOpacity(float opacity);

    /**
     * @brief 是否有透明度效果
     */
    bool HasOpacity() const { return opacity_ < 1.0f; }

    /**
     * @brief 计算累积透明度（考虑所有祖先）
     */
    float GetAccumulatedOpacity() const;

    // =========================================================================
    // Filter
    // =========================================================================

    /**
     * @brief 获取滤镜列表
     */
    const std::vector<FilterOperation>& GetFilters() const { return filters_; }

    /**
     * @brief 设置滤镜列表
     */
    void SetFilters(std::vector<FilterOperation> filters);

    /**
     * @brief 添加滤镜
     */
    void AddFilter(const FilterOperation& filter);

    /**
     * @brief 清除所有滤镜
     */
    void ClearFilters();

    /**
     * @brief 是否有滤镜
     */
    bool HasFilter() const { return !filters_.empty(); }

    // =========================================================================
    // Backdrop Filter
    // =========================================================================

    /**
     * @brief 获取背景滤镜列表
     */
    const std::vector<FilterOperation>& GetBackdropFilters() const { return backdrop_filters_; }

    /**
     * @brief 设置背景滤镜列表
     */
    void SetBackdropFilters(std::vector<FilterOperation> filters);

    /**
     * @brief 是否有背景滤镜
     */
    bool HasBackdropFilter() const { return !backdrop_filters_.empty(); }

    // =========================================================================
    // Blend Mode
    // =========================================================================

    /**
     * @brief 获取混合模式
     */
    SkBlendMode GetBlendMode() const { return blend_mode_; }

    /**
     * @brief 设置混合模式
     */
    void SetBlendMode(SkBlendMode mode);

    /**
     * @brief 是否有非默认混合模式
     */
    bool HasBlendMode() const { return blend_mode_ != SkBlendMode::kSrcOver; }

    // =========================================================================
    // Mask
    // =========================================================================

    /**
     * @brief 获取遮罩 RenderObject
     */
    RenderObject* GetMask() const { return mask_; }

    /**
     * @brief 设置遮罩
     */
    void SetMask(RenderObject* mask);

    /**
     * @brief 是否有遮罩
     */
    bool HasMask() const { return mask_ != nullptr; }

    // =========================================================================
    // 效果隔离
    // =========================================================================

    /**
     * @brief 是否需要隔离（独立渲染表面）
     * @note 以下情况需要隔离：
     *       - 有 backdrop-filter
     *       - 有非默认 blend-mode
     *       - 有遮罩
     *       - opacity < 1 且有多个子元素
     */
    bool RequiresIsolation() const;

    /**
     * @brief 获取效果标志
     */
    EffectFlags GetFlags() const { return flags_; }

    // =========================================================================
    // 变换和裁剪关联
    // =========================================================================

    /**
     * @brief 获取关联的变换节点
     */
    TransformTreeNode* GetTransformNode() const { return transform_node_; }

    /**
     * @brief 设置关联的变换节点
     */
    void SetTransformNode(TransformTreeNode* node) { transform_node_ = node; }

    /**
     * @brief 获取输出裁剪节点
     */
    ClipTreeNode* GetOutputClipNode() const { return output_clip_node_; }

    /**
     * @brief 设置输出裁剪节点
     */
    void SetOutputClipNode(ClipTreeNode* node) { output_clip_node_ = node; }

    // =========================================================================
    // 直接更新支持
    // =========================================================================

    /**
     * @brief 是否可以直接更新 opacity
     * @note 如果元素已有独立层，opacity 更新不需要重新光栅化
     */
    bool CanDirectlyUpdateOpacity() const { return can_directly_update_opacity_; }

    /**
     * @brief 设置是否可以直接更新 opacity
     */
    void SetCanDirectlyUpdateOpacity(bool can) { can_directly_update_opacity_ = can; }

    // =========================================================================
    // RenderObject 关联
    // =========================================================================

    /**
     * @brief 获取关联的 RenderObject
     */
    RenderObject* GetRenderObject() const { return render_object_; }

    /**
     * @brief 设置关联的 RenderObject
     */
    void SetRenderObject(RenderObject* obj) { render_object_ = obj; }

private:
    /**
     * @brief 更新效果标志
     */
    void UpdateFlags();

    // 透明度
    float opacity_ = 1.0f;
    
    // 滤镜
    std::vector<FilterOperation> filters_;
    std::vector<FilterOperation> backdrop_filters_;
    
    // 混合模式
    SkBlendMode blend_mode_ = SkBlendMode::kSrcOver;
    
    // 遮罩
    RenderObject* mask_ = nullptr;
    
    // 关联节点
    TransformTreeNode* transform_node_ = nullptr;
    ClipTreeNode* output_clip_node_ = nullptr;
    
    // 关联的 RenderObject
    RenderObject* render_object_ = nullptr;
    
    // 效果标志
    EffectFlags flags_ = EffectFlags::kNone;
    
    // 是否可以直接更新 opacity
    bool can_directly_update_opacity_ = false;
    
    // 缓存的累积 opacity
    mutable float cached_accumulated_opacity_ = 1.0f;
    mutable bool accumulated_opacity_valid_ = false;
};

} // namespace lightui
