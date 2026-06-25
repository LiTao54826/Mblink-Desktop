/**
 * @file css_filters.h
 * @brief CSS 滤镜（filter 和 backdrop-filter）实现
 */

#pragma once

#include "include/core/SkImageFilter.h"
#include "include/core/SkPaint.h"
#include <string>
#include <vector>
#include <optional>
#include <memory>

namespace mblink {

/**
 * @brief CSS 滤镜类型
 */
enum class CSSFilterType {
    Blur,           // blur(radius)
    Brightness,     // brightness(amount)
    Contrast,       // contrast(amount)
    Grayscale,      // grayscale(amount)
    Sepia,          // sepia(amount)
    Saturate,       // saturate(amount)
    HueRotate,      // hue-rotate(angle)
    Invert,         // invert(amount)
    Opacity,        // opacity(amount)
    DropShadow      // drop-shadow(offset-x offset-y blur-radius color)
};

/**
 * @brief CSS 滤镜参数
 */
struct CSSFilter {
    CSSFilterType type;
    
    // 通用参数
    float value = 0.0f;          // 用于 blur, brightness, contrast, etc.
    float amount = 1.0f;         // 用于百分比类型的滤镜
    float angle = 0.0f;          // 用于 hue-rotate
    
    // drop-shadow 专用参数
    float offset_x = 0.0f;
    float offset_y = 0.0f;
    float blur_radius = 0.0f;
    uint32_t color = 0xFF000000; // 默认黑色
    
    /**
     * @brief 构造函数
     */
    CSSFilter(CSSFilterType t = CSSFilterType::Blur) : type(t) {}
    
    /**
     * @brief 创建 blur 滤镜
     * @param radius 模糊半径（像素）
     */
    static CSSFilter Blur(float radius);
    
    /**
     * @brief 创建 brightness 滤镜
     * @param amount 亮度值（0-1为变暗，1为正常，>1为变亮）
     */
    static CSSFilter Brightness(float amount);
    
    /**
     * @brief 创建 contrast 滤镜
     * @param amount 对比度值（0-1为降低，1为正常，>1为增强）
     */
    static CSSFilter Contrast(float amount);
    
    /**
     * @brief 创建 grayscale 滤镜
     * @param amount 灰度值（0为彩色，1为完全灰度）
     */
    static CSSFilter Grayscale(float amount);
    
    /**
     * @brief 创建 sepia 滤镜
     * @param amount 棕褐色值（0为彩色，1为完全棕褐色）
     */
    static CSSFilter Sepia(float amount);
    
    /**
     * @brief 创建 saturate 滤镜
     * @param amount 饱和度值（0为灰度，1为正常，>1为增强）
     */
    static CSSFilter Saturate(float amount);
    
    /**
     * @brief 创建 hue-rotate 滤镜
     * @param angle 色相旋转角度（度）
     */
    static CSSFilter HueRotate(float angle);
    
    /**
     * @brief 创建 invert 滤镜
     * @param amount 反转值（0为正常，1为完全反转）
     */
    static CSSFilter Invert(float amount);
    
    /**
     * @brief 创建 opacity 滤镜
     * @param amount 不透明度值（0为完全透明，1为完全不透明）
     */
    static CSSFilter Opacity(float amount);
    
    /**
     * @brief 创建 drop-shadow 滤镜
     * @param offset_x X 轴偏移
     * @param offset_y Y 轴偏移
     * @param blur_radius 模糊半径
     * @param color 阴影颜色
     */
    static CSSFilter DropShadow(float offset_x, float offset_y, float blur_radius, uint32_t color);
};

/**
 * @brief CSS 滤镜列表
 */
class CSSFilterList {
public:
    /**
     * @brief 添加滤镜
     */
    void AddFilter(const CSSFilter& filter);
    
    /**
     * @brief 清空滤镜列表
     */
    void Clear();
    
    /**
     * @brief 获取滤镜数量
     */
    size_t GetCount() const { return filters_.size(); }
    
    /**
     * @brief 获取滤镜列表
     */
    const std::vector<CSSFilter>& GetFilters() const { return filters_; }
    
    /**
     * @brief 检查是否为空
     */
    bool IsEmpty() const { return filters_.empty(); }
    
    /**
     * @brief 创建 Skia 图像滤镜
     * @return Skia 图像滤镜对象
     * 
     * 将 CSS 滤镜列表转换为 Skia 的 SkImageFilter 链
     */
    sk_sp<SkImageFilter> CreateSkiaFilter() const;
    
    /**
     * @brief 应用滤镜到画笔
     * @param paint Skia 画笔对象
     * 
     * 将滤镜应用到 SkPaint 对象上
     */
    void ApplyToPaint(SkPaint& paint) const;

private:
    std::vector<CSSFilter> filters_;
};

/**
 * @brief CSS 滤镜解析器
 */
class CSSFilterParser {
public:
    /**
     * @brief 解析 filter 属性值
     * @param value CSS filter 属性值（如 "blur(5px) brightness(1.2)"）
     * @return 滤镜列表
     * 
     * 支持的语法：
     * - blur(5px)
     * - brightness(1.2)
     * - contrast(0.8)
     * - grayscale(0.5)
     * - sepia(0.3)
     * - saturate(1.5)
     * - hue-rotate(90deg)
     * - invert(0.5)
     * - opacity(0.8)
     * - drop-shadow(2px 2px 4px rgba(0,0,0,0.5))
     */
    static std::optional<CSSFilterList> Parse(const std::string& value);
    
    /**
     * @brief 解析单个滤镜函数
     * @param filter_func 滤镜函数字符串（如 "blur(5px)"）
     * @return 滤镜对象
     */
    static std::optional<CSSFilter> ParseSingleFilter(const std::string& filter_func);

private:
    /**
     * @brief 提取函数名和参数
     * @param filter_func 滤镜函数字符串
     * @return {函数名, 参数字符串}
     */
    static std::optional<std::pair<std::string, std::string>> ExtractFunctionAndArgs(
        const std::string& filter_func);
    
    /**
     * @brief 解析长度值（支持 px, em, rem 等单位）
     * @param value 长度字符串（如 "5px"）
     * @return 像素值
     */
    static float ParseLength(const std::string& value);
    
    /**
     * @brief 解析角度值（支持 deg, rad, turn 等单位）
     * @param value 角度字符串（如 "90deg"）
     * @return 角度值（度）
     */
    static float ParseAngle(const std::string& value);
    
    /**
     * @brief 解析百分比或数字
     * @param value 值字符串（如 "50%" 或 "0.5"）
     * @return 归一化值（0-1）
     */
    static float ParseNumberOrPercentage(const std::string& value);
    
    /**
     * @brief 去除字符串首尾空格
     */
    static std::string Trim(const std::string& str);
    
    /**
     * @brief 分割滤镜函数列表
     * @param value 完整的 filter 属性值
     * @return 滤镜函数字符串列表
     */
    static std::vector<std::string> SplitFilters(const std::string& value);
};

/**
 * @brief CSS 滤镜渲染器
 * 
 * 提供滤镜渲染的辅助功能
 */
class CSSFilterRenderer {
public:
    /**
     * @brief 创建单个滤镜的 Skia 图像滤镜
     * @param filter CSS 滤镜对象
     * @param input 输入滤镜（用于链接多个滤镜）
     * @return Skia 图像滤镜
     */
    static sk_sp<SkImageFilter> CreateSkiaFilter(const CSSFilter& filter, 
                                                 sk_sp<SkImageFilter> input = nullptr);

private:
    /**
     * @brief 创建颜色矩阵滤镜
     * @param matrix 5x4 颜色矩阵
     * @param input 输入滤镜
     * @return Skia 图像滤镜
     */
    static sk_sp<SkImageFilter> CreateColorMatrixFilter(const float matrix[20],
                                                        sk_sp<SkImageFilter> input);
};

} // namespace mblink

