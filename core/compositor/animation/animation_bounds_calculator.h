/**
 * @file animation_bounds_calculator.h
 * @brief 动画边界计算器 - 计算动画过程中元素的完整边界范围
 *
 * AnimationBoundsCalculator 负责：
 * - 从 keyframes 提取所有变换值
 * - 计算旋转、位移、缩放动画的边界
 * - 计算组合变换的联合边界
 *
 * 用于解决动画元素在旋转或移动时被错误裁剪的问题。
 */

#pragma once

#include <optional>
#include <string>
#include <vector>
#include "include/core/SkMatrix.h"
#include "include/core/SkPoint.h"
#include "include/core/SkRect.h"
#include "include/core/SkSize.h"

namespace mbink {

// 前向声明
struct KeyframesRule;
struct TransformOrigin;

/**
 * @brief 动画边界信息
 *
 * 包含扩展后的边界、偏移量和是否需要扩展的标记。
 */
struct AnimationBounds {
    SkRect bounds;           ///< 扩展后的边界（相对于元素原点）
    SkPoint offset;          ///< 相对于原始位置的偏移（通常为负值）
    bool needs_expansion;    ///< 是否需要扩展边界

    /**
     * @brief 默认构造函数
     */
    AnimationBounds()
        : bounds(SkRect::MakeEmpty())
        , offset({0, 0})
        , needs_expansion(false) {}

    /**
     * @brief 构造函数
     * @param b 边界矩形
     * @param off 偏移量
     * @param expand 是否需要扩展
     */
    AnimationBounds(const SkRect& b, const SkPoint& off, bool expand)
        : bounds(b)
        , offset(off)
        , needs_expansion(expand) {}
};

/**
 * @brief 动画边界计算器
 *
 * 提供静态方法计算各种类型动画的边界范围。
 */
class AnimationBoundsCalculator {
public:
    /**
     * @brief 计算动画的完整边界
     *
     * 从 keyframes 提取所有变换值，计算能容纳整个动画过程的边界。
     *
     * @param element_size 元素的原始尺寸
     * @param animation_name 动画名称
     * @param transform_origin 变换原点
     * @return 动画边界信息
     */
    static AnimationBounds Calculate(
        const SkSize& element_size,
        const std::string& animation_name,
        const TransformOrigin& transform_origin);

    /**
     * @brief 计算旋转动画的边界
     *
     * 对于旋转动画，边界需要足够大以容纳元素在任意旋转角度下的位置。
     * 使用元素的对角线长度作为边界的最小尺寸。
     *
     * @param element_size 元素尺寸
     * @param min_angle 最小旋转角度（度）
     * @param max_angle 最大旋转角度（度）
     * @param origin 变换原点（相对于元素左上角）
     * @return 动画边界
     */
    static AnimationBounds CalculateRotationBounds(
        const SkSize& element_size,
        float min_angle,
        float max_angle,
        const SkPoint& origin);

    /**
     * @brief 计算位移动画的边界
     *
     * 计算所有位移值的联合边界，确保元素在任意位移位置都不会被裁剪。
     *
     * @param element_size 元素尺寸
     * @param translations 所有位移值列表
     * @return 动画边界
     */
    static AnimationBounds CalculateTranslationBounds(
        const SkSize& element_size,
        const std::vector<SkPoint>& translations);

    /**
     * @brief 计算缩放动画的边界
     *
     * 根据最大缩放值计算边界，确保元素在最大缩放时不会被裁剪。
     *
     * @param element_size 元素尺寸
     * @param max_scale_x 最大 X 缩放
     * @param max_scale_y 最大 Y 缩放
     * @param origin 变换原点（相对于元素左上角）
     * @return 动画边界
     */
    static AnimationBounds CalculateScaleBounds(
        const SkSize& element_size,
        float max_scale_x,
        float max_scale_y,
        const SkPoint& origin);

    /**
     * @brief 从 keyframes 提取所有变换矩阵
     *
     * 解析 keyframes 中每个关键帧的 transform 属性，
     * 转换为 SkMatrix 列表。
     *
     * @param keyframes 关键帧规则
     * @param element_size 元素尺寸（用于解析百分比值）
     * @param origin 变换原点
     * @return 所有变换矩阵列表
     */
    static std::vector<SkMatrix> ExtractTransforms(
        const KeyframesRule& keyframes,
        const SkSize& element_size,
        const TransformOrigin& origin);

    /**
     * @brief 计算多个变换的联合边界
     *
     * 对每个变换矩阵应用到元素的四个角点，
     * 计算所有变换后位置的联合边界框。
     *
     * @param element_size 元素尺寸
     * @param transforms 变换矩阵列表
     * @return 联合边界
     */
    static AnimationBounds CalculateUnionBounds(
        const SkSize& element_size,
        const std::vector<SkMatrix>& transforms);

private:
    /**
     * @brief 计算矩形在给定变换下的边界框
     * @param rect 原始矩形
     * @param transform 变换矩阵
     * @return 变换后的边界框
     */
    static SkRect TransformRect(const SkRect& rect, const SkMatrix& transform);

    /**
     * @brief 从 transform 字符串解析旋转角度
     * @param transform_str transform 属性值
     * @return 旋转角度（度），如果没有旋转返回 nullopt
     */
    static std::optional<float> ExtractRotationAngle(const std::string& transform_str);

    /**
     * @brief 从 transform 字符串解析位移值
     * @param transform_str transform 属性值
     * @param element_size 元素尺寸（用于解析百分比）
     * @return 位移值，如果没有位移返回 nullopt
     */
    static std::optional<SkPoint> ExtractTranslation(
        const std::string& transform_str,
        const SkSize& element_size);

    /**
     * @brief 从 transform 字符串解析缩放值
     * @param transform_str transform 属性值
     * @return 缩放值 (scaleX, scaleY)，如果没有缩放返回 nullopt
     */
    static std::optional<std::pair<float, float>> ExtractScale(const std::string& transform_str);
};

} // namespace mbink
