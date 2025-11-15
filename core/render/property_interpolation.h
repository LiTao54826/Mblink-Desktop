#pragma once

#include <string>
#include <map>
#include <optional>
#include "include/core/SkColor.h"

namespace lightui {

/**
 * @brief 属性插值器
 * 
 * 负责在两个CSS属性值之间进行插值计算
 */
class PropertyInterpolation {
public:
    /**
     * @brief 在两个属性值之间插值
     * 
     * @param property 属性名
     * @param from 起始值
     * @param to 目标值
     * @param factor 插值因子 (0.0 ~ 1.0)
     * @return 插值后的属性值，如果无法插值则返回 nullopt
     */
    static std::optional<std::string> Interpolate(
        const std::string& property,
        const std::string& from,
        const std::string& to,
        float factor);
    
    /**
     * @brief 在两个属性映射之间插值
     * 
     * @param from 起始属性映射
     * @param to 目标属性映射
     * @param factor 插值因子 (0.0 ~ 1.0)
     * @return 插值后的属性映射
     */
    static std::map<std::string, std::string> InterpolateProperties(
        const std::map<std::string, std::string>& from,
        const std::map<std::string, std::string>& to,
        float factor);
    
private:
    /**
     * @brief 数值插值 (如 width, height, margin, padding 等)
     */
    static std::optional<std::string> InterpolateNumber(
        const std::string& from,
        const std::string& to,
        float factor);
    
    /**
     * @brief 颜色插值 (如 color, background-color, border-color 等)
     */
    static std::optional<std::string> InterpolateColor(
        const std::string& from,
        const std::string& to,
        float factor);
    
    /**
     * @brief Transform 插值
     */
    static std::optional<std::string> InterpolateTransform(
        const std::string& from,
        const std::string& to,
        float factor);
    
    /**
     * @brief 判断属性是否为数值类型
     */
    static bool IsNumberProperty(const std::string& property);
    
    /**
     * @brief 判断属性是否为颜色类型
     */
    static bool IsColorProperty(const std::string& property);
    
    /**
     * @brief 判断属性是否为 Transform 类型
     */
    static bool IsTransformProperty(const std::string& property);
    
    /**
     * @brief 解析数值和单位
     * @return {value, unit} 例如 "100px" -> {100.0, "px"}
     */
    static std::pair<float, std::string> ParseNumberWithUnit(const std::string& str);
    
    /**
     * @brief 解析颜色值
     * @return SkColor
     */
    static std::optional<SkColor> ParseColor(const std::string& str);
    
    /**
     * @brief 将 SkColor 转换为 CSS 颜色字符串
     */
    static std::string ColorToString(SkColor color);
    
    /**
     * @brief 在两个颜色之间插值
     */
    static SkColor InterpolateColorValue(SkColor from, SkColor to, float factor);
};

} // namespace lightui

