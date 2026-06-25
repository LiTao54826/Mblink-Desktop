/**
 * @file compositing_reasons.h
 * @brief 合成原因
 *
 * 定义元素需要独立合成层的原因：
 * - will-change 属性
 * - 活动动画
 * - 重叠
 * - 特殊元素（video、canvas）
 *
 * 参考 Chromium Blink: platform/graphics/compositing_reasons.h
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace mblink {

/**
 * @brief 合成原因位标志
 */
enum class CompositingReasons : uint32_t {
    kNone = 0,
    
    // will-change 相关
    kWillChangeTransform = 1 << 0,
    kWillChangeOpacity = 1 << 1,
    kWillChangeFilter = 1 << 2,
    kWillChangeScrollPosition = 1 << 3,
    
    // 活动动画相关
    kActiveTransformAnimation = 1 << 4,
    kActiveOpacityAnimation = 1 << 5,
    kActiveFilterAnimation = 1 << 6,
    
    // 重叠相关
    kOverlap = 1 << 7,
    kAssumedOverlap = 1 << 8,
    
    // 定位相关
    kFixedPosition = 1 << 9,
    kStickyPosition = 1 << 10,
    
    // 效果相关
    kBackdropFilter = 1 << 11,
    kBlendMode = 1 << 12,
    kIsolation = 1 << 13,
    kMask = 1 << 14,
    kClipPath = 1 << 15,
    
    // 特殊元素
    kVideo = 1 << 16,
    kCanvas = 1 << 17,
    kIFrame = 1 << 18,
    kPlugin = 1 << 19,
    
    // 滚动相关
    kScrollbar = 1 << 20,
    kOverflowScrolling = 1 << 21,
    
    // 3D 相关
    k3DTransform = 1 << 22,
    kPreserve3D = 1 << 23,
    kPerspective = 1 << 24,
    
    // 其他
    kRoot = 1 << 25,
    kLayerForForeground = 1 << 26,
    kLayerForBackground = 1 << 27,
    kLayerForMask = 1 << 28,
    kLayerForClippingMask = 1 << 29,
    kLayerForScrollingContents = 1 << 30,
    kLayerForSquashingContents = 1u << 31,
};

// =========================================================================
// 位运算支持
// =========================================================================

inline CompositingReasons operator|(CompositingReasons a, CompositingReasons b) {
    return static_cast<CompositingReasons>(
        static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline CompositingReasons operator&(CompositingReasons a, CompositingReasons b) {
    return static_cast<CompositingReasons>(
        static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

inline CompositingReasons operator~(CompositingReasons a) {
    return static_cast<CompositingReasons>(~static_cast<uint32_t>(a));
}

inline CompositingReasons& operator|=(CompositingReasons& a, CompositingReasons b) {
    a = a | b;
    return a;
}

inline CompositingReasons& operator&=(CompositingReasons& a, CompositingReasons b) {
    a = a & b;
    return a;
}

// =========================================================================
// 辅助函数
// =========================================================================

/**
 * @brief 检查是否有指定的合成原因
 */
inline bool HasCompositingReason(CompositingReasons reasons, CompositingReasons flag) {
    return (static_cast<uint32_t>(reasons) & static_cast<uint32_t>(flag)) != 0;
}

/**
 * @brief 检查是否有任何合成原因
 */
inline bool HasAnyCompositingReason(CompositingReasons reasons) {
    return reasons != CompositingReasons::kNone;
}

/**
 * @brief 检查是否需要独立层（强制合成原因）
 */
inline bool RequiresOwnLayer(CompositingReasons reasons) {
    const CompositingReasons kForcedReasons = 
        CompositingReasons::kWillChangeTransform |
        CompositingReasons::kWillChangeOpacity |
        CompositingReasons::kActiveTransformAnimation |
        CompositingReasons::kActiveOpacityAnimation |
        CompositingReasons::kVideo |
        CompositingReasons::kCanvas |
        CompositingReasons::kIFrame |
        CompositingReasons::kBackdropFilter |
        CompositingReasons::k3DTransform |
        CompositingReasons::kRoot;
    
    return (static_cast<uint32_t>(reasons) & static_cast<uint32_t>(kForcedReasons)) != 0;
}

/**
 * @brief 检查是否可以直接更新（不需要重新光栅化）
 */
inline bool CanDirectlyUpdate(CompositingReasons reasons) {
    return HasCompositingReason(reasons, CompositingReasons::kWillChangeTransform) ||
           HasCompositingReason(reasons, CompositingReasons::kWillChangeOpacity) ||
           HasCompositingReason(reasons, CompositingReasons::kActiveTransformAnimation) ||
           HasCompositingReason(reasons, CompositingReasons::kActiveOpacityAnimation);
}

/**
 * @brief 获取合成原因的描述字符串
 * @param reasons 合成原因
 * @return 描述字符串列表
 */
inline std::vector<std::string> GetCompositingReasonDescriptions(CompositingReasons reasons) {
    std::vector<std::string> descriptions;
    
    if (HasCompositingReason(reasons, CompositingReasons::kWillChangeTransform)) {
        descriptions.push_back("will-change: transform");
    }
    if (HasCompositingReason(reasons, CompositingReasons::kWillChangeOpacity)) {
        descriptions.push_back("will-change: opacity");
    }
    if (HasCompositingReason(reasons, CompositingReasons::kWillChangeFilter)) {
        descriptions.push_back("will-change: filter");
    }
    if (HasCompositingReason(reasons, CompositingReasons::kWillChangeScrollPosition)) {
        descriptions.push_back("will-change: scroll-position");
    }
    if (HasCompositingReason(reasons, CompositingReasons::kActiveTransformAnimation)) {
        descriptions.push_back("Active transform animation");
    }
    if (HasCompositingReason(reasons, CompositingReasons::kActiveOpacityAnimation)) {
        descriptions.push_back("Active opacity animation");
    }
    if (HasCompositingReason(reasons, CompositingReasons::kActiveFilterAnimation)) {
        descriptions.push_back("Active filter animation");
    }
    if (HasCompositingReason(reasons, CompositingReasons::kOverlap)) {
        descriptions.push_back("Overlaps other composited content");
    }
    if (HasCompositingReason(reasons, CompositingReasons::kAssumedOverlap)) {
        descriptions.push_back("Assumed overlap");
    }
    if (HasCompositingReason(reasons, CompositingReasons::kFixedPosition)) {
        descriptions.push_back("Fixed position");
    }
    if (HasCompositingReason(reasons, CompositingReasons::kStickyPosition)) {
        descriptions.push_back("Sticky position");
    }
    if (HasCompositingReason(reasons, CompositingReasons::kBackdropFilter)) {
        descriptions.push_back("Has backdrop-filter");
    }
    if (HasCompositingReason(reasons, CompositingReasons::kBlendMode)) {
        descriptions.push_back("Has blend mode");
    }
    if (HasCompositingReason(reasons, CompositingReasons::kIsolation)) {
        descriptions.push_back("Isolation");
    }
    if (HasCompositingReason(reasons, CompositingReasons::kMask)) {
        descriptions.push_back("Has mask");
    }
    if (HasCompositingReason(reasons, CompositingReasons::kClipPath)) {
        descriptions.push_back("Has clip-path");
    }
    if (HasCompositingReason(reasons, CompositingReasons::kVideo)) {
        descriptions.push_back("Video element");
    }
    if (HasCompositingReason(reasons, CompositingReasons::kCanvas)) {
        descriptions.push_back("Canvas element");
    }
    if (HasCompositingReason(reasons, CompositingReasons::kIFrame)) {
        descriptions.push_back("IFrame element");
    }
    if (HasCompositingReason(reasons, CompositingReasons::kScrollbar)) {
        descriptions.push_back("Scrollbar");
    }
    if (HasCompositingReason(reasons, CompositingReasons::kOverflowScrolling)) {
        descriptions.push_back("Overflow scrolling");
    }
    if (HasCompositingReason(reasons, CompositingReasons::k3DTransform)) {
        descriptions.push_back("3D transform");
    }
    if (HasCompositingReason(reasons, CompositingReasons::kPreserve3D)) {
        descriptions.push_back("preserve-3d");
    }
    if (HasCompositingReason(reasons, CompositingReasons::kPerspective)) {
        descriptions.push_back("Has perspective");
    }
    if (HasCompositingReason(reasons, CompositingReasons::kRoot)) {
        descriptions.push_back("Root layer");
    }
    
    return descriptions;
}

/**
 * @brief 获取合成原因的简短描述
 * @param reasons 合成原因
 * @return 简短描述字符串
 */
inline std::string GetCompositingReasonsSummary(CompositingReasons reasons) {
    if (reasons == CompositingReasons::kNone) {
        return "None";
    }
    
    auto descriptions = GetCompositingReasonDescriptions(reasons);
    if (descriptions.empty()) {
        return "Unknown";
    }
    
    std::string result = descriptions[0];
    for (size_t i = 1; i < descriptions.size() && i < 3; ++i) {
        result += ", " + descriptions[i];
    }
    
    if (descriptions.size() > 3) {
        result += " (+" + std::to_string(descriptions.size() - 3) + " more)";
    }
    
    return result;
}

} // namespace mblink
