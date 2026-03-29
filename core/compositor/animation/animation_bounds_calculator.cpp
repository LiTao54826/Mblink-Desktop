/**
 * @file animation_bounds_calculator.cpp
 * @brief 动画边界计算器实现
 */

#include "animation_bounds_calculator.h"
#include "core/render/animation/keyframes.h"
#include "core/render/utils/transform.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <regex>

namespace mbink {

// ============================================================================
// CalculateRotationBounds 实现
// ============================================================================

AnimationBounds AnimationBoundsCalculator::CalculateRotationBounds(
    const SkSize& element_size,
    float min_angle,
    float max_angle,
    const SkPoint& origin) {
    
    // 如果没有旋转，不需要扩展
    if (min_angle == 0.0f && max_angle == 0.0f) {
        return AnimationBounds();
    }

    const float width = element_size.width();
    const float height = element_size.height();

    // 计算元素四个角点相对于旋转原点的距离
    // 找出最远的角点距离，这决定了旋转时需要的最大半径
    SkPoint corners[4] = {
        {0, 0},                    // 左上
        {width, 0},                // 右上
        {width, height},           // 右下
        {0, height}                // 左下
    };

    float max_radius = 0.0f;
    for (const auto& corner : corners) {
        float dx = corner.fX - origin.fX;
        float dy = corner.fY - origin.fY;
        float radius = std::sqrt(dx * dx + dy * dy);
        max_radius = std::max(max_radius, radius);
    }

    // 旋转边界是以原点为中心、半径为 max_radius 的圆的外接正方形
    // 但我们需要考虑原点可能不在元素中心
    
    // 计算扩展后的边界
    // 边界需要包含：原点左侧 max_radius，原点右侧 max_radius
    //              原点上方 max_radius，原点下方 max_radius
    float left = origin.fX - max_radius;
    float top = origin.fY - max_radius;
    float right = origin.fX + max_radius;
    float bottom = origin.fY + max_radius;

    // 计算偏移量（边界左上角相对于元素原始左上角的偏移）
    SkPoint offset = {left, top};

    // 边界矩形（相对于新的左上角）
    SkRect bounds = SkRect::MakeWH(right - left, bottom - top);

    return AnimationBounds(bounds, offset, true);
}

// ============================================================================
// CalculateTranslationBounds 实现
// ============================================================================

AnimationBounds AnimationBoundsCalculator::CalculateTranslationBounds(
    const SkSize& element_size,
    const std::vector<SkPoint>& translations) {
    
    if (translations.empty()) {
        return AnimationBounds();
    }

    const float width = element_size.width();
    const float height = element_size.height();

    // 计算所有位移位置的边界
    float min_x = 0.0f;
    float max_x = width;
    float min_y = 0.0f;
    float max_y = height;

    for (const auto& trans : translations) {
        // 位移后的左上角
        min_x = std::min(min_x, trans.fX);
        min_y = std::min(min_y, trans.fY);
        
        // 位移后的右下角
        max_x = std::max(max_x, trans.fX + width);
        max_y = std::max(max_y, trans.fY + height);
    }

    // 检查是否需要扩展
    bool needs_expansion = (min_x < 0 || min_y < 0 || 
                           max_x > width || max_y > height);

    if (!needs_expansion) {
        return AnimationBounds();
    }

    // 计算偏移量（边界左上角相对于元素原始左上角的偏移）
    SkPoint offset = SkPoint::Make(min_x, min_y);

    // 边界矩形
    SkRect bounds = SkRect::MakeWH(max_x - min_x, max_y - min_y);

    return AnimationBounds(bounds, offset, true);
}

// ============================================================================
// CalculateScaleBounds 实现
// ============================================================================

AnimationBounds AnimationBoundsCalculator::CalculateScaleBounds(
    const SkSize& element_size,
    float max_scale_x,
    float max_scale_y,
    const SkPoint& origin) {
    
    // 如果缩放为 1，不需要扩展
    if (max_scale_x <= 1.0f && max_scale_y <= 1.0f) {
        return AnimationBounds();
    }

    const float width = element_size.width();
    const float height = element_size.height();

    // 计算缩放后的四个角点
    SkPoint corners[4] = {
        {0, 0}, {width, 0}, {width, height}, {0, height}
    };

    // 应用缩放变换
    SkMatrix scale_matrix;
    scale_matrix.setScale(max_scale_x, max_scale_y, origin.fX, origin.fY);
    scale_matrix.mapPoints(corners, 4);

    // 计算边界
    float min_x = corners[0].fX;
    float max_x = corners[0].fX;
    float min_y = corners[0].fY;
    float max_y = corners[0].fY;

    for (int i = 1; i < 4; ++i) {
        min_x = std::min(min_x, corners[i].fX);
        max_x = std::max(max_x, corners[i].fX);
        min_y = std::min(min_y, corners[i].fY);
        max_y = std::max(max_y, corners[i].fY);
    }

    // 也要包含原始位置
    min_x = std::min(min_x, 0.0f);
    max_x = std::max(max_x, width);
    min_y = std::min(min_y, 0.0f);
    max_y = std::max(max_y, height);

    // 计算偏移量
    SkPoint offset = SkPoint::Make(min_x, min_y);

    // 边界矩形
    SkRect bounds = SkRect::MakeWH(max_x - min_x, max_y - min_y);

    return AnimationBounds(bounds, offset, true);
}

// ============================================================================
// 辅助方法实现
// ============================================================================

SkRect AnimationBoundsCalculator::TransformRect(const SkRect& rect, const SkMatrix& transform) {
    SkPoint corners[4] = {
        {rect.fLeft, rect.fTop},
        {rect.fRight, rect.fTop},
        {rect.fRight, rect.fBottom},
        {rect.fLeft, rect.fBottom}
    };
    
    transform.mapPoints(corners, 4);
    
    float min_x = corners[0].fX;
    float max_x = corners[0].fX;
    float min_y = corners[0].fY;
    float max_y = corners[0].fY;
    
    for (int i = 1; i < 4; ++i) {
        min_x = std::min(min_x, corners[i].fX);
        max_x = std::max(max_x, corners[i].fX);
        min_y = std::min(min_y, corners[i].fY);
        max_y = std::max(max_y, corners[i].fY);
    }
    
    return SkRect::MakeLTRB(min_x, min_y, max_x, max_y);
}

std::optional<float> AnimationBoundsCalculator::ExtractRotationAngle(const std::string& transform_str) {
    // 匹配 rotate(Xdeg) 或 rotate(Xrad) 或 rotate(Xturn)
    std::regex rotate_regex(R"(rotate\s*\(\s*(-?[\d.]+)\s*(deg|rad|turn)?\s*\))", std::regex::icase);
    std::smatch match;
    
    if (std::regex_search(transform_str, match, rotate_regex)) {
        float value = std::stof(match[1].str());
        std::string unit = match[2].str();
        
        // 转换为度
        if (unit == "rad") {
            value = value * 180.0f / 3.14159265358979f;
        } else if (unit == "turn") {
            value = value * 360.0f;
        }
        // deg 或无单位默认为度
        
        return value;
    }
    
    return std::nullopt;
}

std::optional<SkPoint> AnimationBoundsCalculator::ExtractTranslation(
    const std::string& transform_str,
    const SkSize& element_size) {
    
    float tx = 0.0f, ty = 0.0f;
    bool found = false;
    
    // 匹配 translateX(value)
    std::regex translateX_regex(R"(translateX\s*\(\s*(-?[\d.]+)\s*(px|%|em|rem)?\s*\))", std::regex::icase);
    std::smatch match;
    if (std::regex_search(transform_str, match, translateX_regex)) {
        float value = std::stof(match[1].str());
        std::string unit = match[2].str();
        if (unit == "%") {
            tx = value / 100.0f * element_size.width();
        } else {
            tx = value; // px 或无单位
        }
        found = true;
    }
    
    // 匹配 translateY(value)
    std::regex translateY_regex(R"(translateY\s*\(\s*(-?[\d.]+)\s*(px|%|em|rem)?\s*\))", std::regex::icase);
    if (std::regex_search(transform_str, match, translateY_regex)) {
        float value = std::stof(match[1].str());
        std::string unit = match[2].str();
        if (unit == "%") {
            ty = value / 100.0f * element_size.height();
        } else {
            ty = value;
        }
        found = true;
    }
    
    // 匹配 translate(x, y) 或 translate(x)
    std::regex translate_regex(R"(translate\s*\(\s*(-?[\d.]+)\s*(px|%|em|rem)?\s*(?:,\s*(-?[\d.]+)\s*(px|%|em|rem)?)?\s*\))", std::regex::icase);
    if (std::regex_search(transform_str, match, translate_regex)) {
        float x_value = std::stof(match[1].str());
        std::string x_unit = match[2].str();
        if (x_unit == "%") {
            tx = x_value / 100.0f * element_size.width();
        } else {
            tx = x_value;
        }
        
        if (match[3].matched) {
            float y_value = std::stof(match[3].str());
            std::string y_unit = match[4].str();
            if (y_unit == "%") {
                ty = y_value / 100.0f * element_size.height();
            } else {
                ty = y_value;
            }
        }
        found = true;
    }
    
    if (found) {
        return SkPoint::Make(tx, ty);
    }
    return std::nullopt;
}

std::optional<std::pair<float, float>> AnimationBoundsCalculator::ExtractScale(const std::string& transform_str) {
    float sx = 1.0f, sy = 1.0f;
    bool found = false;
    
    // 匹配 scaleX(value)
    std::regex scaleX_regex(R"(scaleX\s*\(\s*(-?[\d.]+)\s*\))", std::regex::icase);
    std::smatch match;
    if (std::regex_search(transform_str, match, scaleX_regex)) {
        sx = std::stof(match[1].str());
        found = true;
    }
    
    // 匹配 scaleY(value)
    std::regex scaleY_regex(R"(scaleY\s*\(\s*(-?[\d.]+)\s*\))", std::regex::icase);
    if (std::regex_search(transform_str, match, scaleY_regex)) {
        sy = std::stof(match[1].str());
        found = true;
    }
    
    // 匹配 scale(x, y) 或 scale(x)
    std::regex scale_regex(R"(scale\s*\(\s*(-?[\d.]+)\s*(?:,\s*(-?[\d.]+))?\s*\))", std::regex::icase);
    if (std::regex_search(transform_str, match, scale_regex)) {
        sx = std::stof(match[1].str());
        if (match[2].matched) {
            sy = std::stof(match[2].str());
        } else {
            sy = sx; // scale(x) 表示均匀缩放
        }
        found = true;
    }
    
    if (found) {
        return std::make_pair(sx, sy);
    }
    return std::nullopt;
}

// ============================================================================
// ExtractTransforms 实现
// ============================================================================

std::vector<SkMatrix> AnimationBoundsCalculator::ExtractTransforms(
    const KeyframesRule& keyframes,
    const SkSize& element_size,
    const TransformOrigin& origin) {
    
    std::vector<SkMatrix> transforms;
    
    // 计算变换原点的像素坐标
    SkRect element_rect = SkRect::MakeWH(element_size.width(), element_size.height());
    SkPoint origin_point = origin.ToPoint(element_rect);
    
    for (const auto& keyframe : keyframes.keyframes) {
        // 查找 transform 属性
        auto it = keyframe.properties.find("transform");
        if (it == keyframe.properties.end()) {
            // 没有 transform，使用单位矩阵
            transforms.push_back(SkMatrix::I());
            continue;
        }
        
        const std::string& transform_str = it->second;
        
        // 使用 CSSTransform 解析
        auto css_transform = CSSTransform::Parse(transform_str);
        if (css_transform.has_value()) {
            SkMatrix matrix = css_transform->ToSkMatrix(element_rect, origin);
            transforms.push_back(matrix);
        } else {
            // 解析失败，使用单位矩阵
            transforms.push_back(SkMatrix::I());
        }
    }
    
    return transforms;
}

// ============================================================================
// CalculateUnionBounds 实现
// ============================================================================

AnimationBounds AnimationBoundsCalculator::CalculateUnionBounds(
    const SkSize& element_size,
    const std::vector<SkMatrix>& transforms) {
    
    if (transforms.empty()) {
        return AnimationBounds();
    }
    
    const float width = element_size.width();
    const float height = element_size.height();
    
    // 原始矩形
    SkRect original_rect = SkRect::MakeWH(width, height);
    
    // 初始化边界为原始矩形
    float min_x = 0.0f;
    float max_x = width;
    float min_y = 0.0f;
    float max_y = height;
    
    // 对每个变换计算边界并合并
    for (const auto& transform : transforms) {
        SkRect transformed = TransformRect(original_rect, transform);
        
        min_x = std::min(min_x, transformed.fLeft);
        max_x = std::max(max_x, transformed.fRight);
        min_y = std::min(min_y, transformed.fTop);
        max_y = std::max(max_y, transformed.fBottom);
    }
    
    // 检查是否需要扩展
    bool needs_expansion = (min_x < 0 || min_y < 0 || 
                           max_x > width || max_y > height);
    
    if (!needs_expansion) {
        return AnimationBounds();
    }
    
    // 计算偏移量
    SkPoint offset = SkPoint::Make(min_x, min_y);
    
    // 边界矩形
    SkRect bounds = SkRect::MakeWH(max_x - min_x, max_y - min_y);
    
    return AnimationBounds(bounds, offset, true);
}

// ============================================================================
// Calculate 实现
// ============================================================================

AnimationBounds AnimationBoundsCalculator::Calculate(
    const SkSize& element_size,
    const std::string& animation_name,
    const TransformOrigin& transform_origin) {
    
    // 获取 keyframes
    const KeyframesRule* keyframes = KeyframesManager::Instance().GetKeyframes(animation_name);
    if (!keyframes || !keyframes->IsValid()) {
        return AnimationBounds();
    }
    

    
    // 计算变换原点的像素坐标
    SkRect element_rect = SkRect::MakeWH(element_size.width(), element_size.height());
    SkPoint origin_point = transform_origin.ToPoint(element_rect);
    
    // 首先检查是否有旋转动画
    // 对于旋转动画，我们需要特殊处理，因为中间帧的边界可能比起始/结束帧更大
    float min_angle = 0.0f;
    float max_angle = 0.0f;
    bool has_rotation = false;
    bool has_transform = false;
    
    for (const auto& keyframe : keyframes->keyframes) {
        auto it = keyframe.properties.find("transform");
        if (it != keyframe.properties.end()) {
            has_transform = true;
            auto angle = ExtractRotationAngle(it->second);
            if (angle.has_value()) {
                if (!has_rotation) {
                    min_angle = angle.value();
                    max_angle = angle.value();
                    has_rotation = true;
                } else {
                    min_angle = std::min(min_angle, angle.value());
                    max_angle = std::max(max_angle, angle.value());
                }
            }
        }
    }
    

    
    // 如果没有任何 transform 属性，直接返回（不需要扩展边界）
    if (!has_transform) {
        return AnimationBounds();
    }
    
    // 如果有旋转动画，使用旋转边界计算
    // 旋转动画需要特殊处理，因为旋转过程中元素会经过所有中间角度
    if (has_rotation) {
        // 对于旋转动画，我们需要考虑整个旋转范围
        // 如果旋转范围超过 90 度，元素的角会在某些角度超出原始边界
        float angle_range = max_angle - min_angle;
        
        // 如果旋转范围大于等于 90 度，或者跨越了 45 度的倍数，
        // 我们需要使用外接圆来确保不裁剪
        // 简化处理：只要有旋转，就使用外接圆边界
        if (std::abs(angle_range) > 0.1f) {  // 有实际旋转
            return CalculateRotationBounds(element_size, min_angle, max_angle, origin_point);
        }
    }
    
    // 对于非旋转动画（位移、缩放等），使用联合边界计算
    std::vector<SkMatrix> transforms = ExtractTransforms(*keyframes, element_size, transform_origin);
    

    
    if (transforms.empty()) {
        return AnimationBounds();
    }
    
    // 计算联合边界
    AnimationBounds result = CalculateUnionBounds(element_size, transforms);

    return result;
}

} // namespace mbink
