/**
 * @file css_value.h
 * @brief CSS 值解析和转换
 * 
 * 功能：
 * - CSS 单位解析（px, %, em, rem, auto）
 * - CSS 颜色值解析
 * - CSS 数值解析
 * - 单位转换
 */

#pragma once

#include <string>
#include <optional>
#include <vector>
#include "include/core/SkColor.h"

namespace lightui {

/**
 * @brief CSS 单位类型
 */
enum class CSSUnit {
    PX,      // 像素
    PERCENT, // 百分比
    EM,      // 相对于字体大小
    REM,     // 相对于根元素字体大小
    AUTO,    // 自动
    NONE     // 无单位
};

/**
 * @brief CSS 长度值
 */
struct CSSLength {
    float value;
    CSSUnit unit;
    
    CSSLength() : value(0.0f), unit(CSSUnit::PX) {}
    CSSLength(float v, CSSUnit u) : value(v), unit(u) {}
    
    /**
     * @brief 转换为像素值
     * @param base_value 基准值（用于百分比和 em/rem 计算）
     * @param font_size 字体大小（用于 em 计算）
     * @param root_font_size 根字体大小（用于 rem 计算）
     * @return 像素值
     */
    float ToPx(float base_value = 0.0f, float font_size = 16.0f, float root_font_size = 16.0f) const;
    
    /**
     * @brief 是否为自动值
     */
    bool IsAuto() const { return unit == CSSUnit::AUTO; }
    
    /**
     * @brief 是否为零值
     */
    bool IsZero() const { return value == 0.0f && unit != CSSUnit::AUTO; }
};

/**
 * @brief CSS 边距/内边距值（四个方向）
 */
struct CSSEdges {
    CSSLength top;
    CSSLength right;
    CSSLength bottom;
    CSSLength left;
    
    CSSEdges() = default;
    CSSEdges(const CSSLength& all) : top(all), right(all), bottom(all), left(all) {}
    CSSEdges(const CSSLength& t, const CSSLength& r, const CSSLength& b, const CSSLength& l)
        : top(t), right(r), bottom(b), left(l) {}
};

/**
 * @brief CSS 边框样式
 */
enum class CSSBorderStyle {
    NONE,
    SOLID,
    DASHED,
    DOTTED,
    DOUBLE
};

/**
 * @brief CSS 边框定义
 */
struct CSSBorder {
    CSSLength width;
    CSSBorderStyle style;
    SkColor color;

    CSSBorder() : width(0.0f, CSSUnit::PX), style(CSSBorderStyle::NONE), color(SK_ColorBLACK) {}
};

/**
 * @brief CSS 圆角定义（四个角）
 */
struct CSSBorderRadius {
    CSSLength top_left;
    CSSLength top_right;
    CSSLength bottom_right;
    CSSLength bottom_left;

    CSSBorderRadius() = default;
    CSSBorderRadius(const CSSLength& all)
        : top_left(all), top_right(all), bottom_right(all), bottom_left(all) {}
    CSSBorderRadius(const CSSLength& tl, const CSSLength& tr,
                    const CSSLength& br, const CSSLength& bl)
        : top_left(tl), top_right(tr), bottom_right(br), bottom_left(bl) {}
};

/**
 * @brief CSS 阴影定义
 */
struct CSSBoxShadow {
    float offset_x;      // X 偏移
    float offset_y;      // Y 偏移
    float blur_radius;   // 模糊半径
    float spread_radius; // 扩展半径
    SkColor color;       // 阴影颜色
    bool inset;          // 是否为内阴影

    CSSBoxShadow()
        : offset_x(0), offset_y(0), blur_radius(0), spread_radius(0),
          color(SK_ColorBLACK), inset(false) {}
};

/**
 * @brief CSS 渐变色停止点
 */
struct CSSGradientStop {
    SkColor color;
    float position; // 0.0 到 1.0

    CSSGradientStop() : color(SK_ColorBLACK), position(0.0f) {}
    CSSGradientStop(SkColor c, float p) : color(c), position(p) {}
};

/**
 * @brief CSS 线性渐变定义
 */
struct CSSLinearGradient {
    float angle;  // 角度（度数）
    std::vector<CSSGradientStop> stops;

    CSSLinearGradient() : angle(0.0f) {}
};

/**
 * @brief CSS 径向渐变定义
 */
struct CSSRadialGradient {
    float center_x;  // 中心 X（0.0 到 1.0）
    float center_y;  // 中心 Y（0.0 到 1.0）
    bool is_circle;  // true = circle, false = ellipse
    std::vector<CSSGradientStop> stops;

    CSSRadialGradient() : center_x(0.5f), center_y(0.5f), is_circle(true) {}
};

/**
 * @brief CSS 背景重复模式
 */
enum class CSSBackgroundRepeat {
    REPEAT,
    NO_REPEAT,
    REPEAT_X,
    REPEAT_Y
};

/**
 * @brief CSS 背景尺寸
 */
struct CSSBackgroundSize {
    enum class Type {
        AUTO,
        COVER,
        CONTAIN,
        LENGTH
    };

    Type type;
    CSSLength width;
    CSSLength height;

    CSSBackgroundSize() : type(Type::AUTO) {}
};

/**
 * @brief CSS 值解析器
 */
class CSSValue {
public:
    /**
     * @brief 解析长度值
     * @param str CSS 长度字符串（如 "10px", "50%", "2em", "auto"）
     * @return CSSLength 对象
     */
    static CSSLength ParseLength(const std::string& str);
    
    /**
     * @brief 解析颜色值
     * @param str CSS 颜色字符串
     * @return SkColor
     */
    static SkColor ParseColor(const std::string& str);
    
    /**
     * @brief 解析边距/内边距值
     * @param str CSS 边距字符串（支持 1-4 个值）
     * @return CSSEdges 对象
     */
    static CSSEdges ParseEdges(const std::string& str);
    
    /**
     * @brief 解析边框样式
     * @param str CSS 边框样式字符串
     * @return CSSBorderStyle
     */
    static CSSBorderStyle ParseBorderStyle(const std::string& str);
    
    /**
     * @brief 解析边框定义
     * @param width_str 宽度字符串
     * @param style_str 样式字符串
     * @param color_str 颜色字符串
     * @return CSSBorder 对象
     */
    static CSSBorder ParseBorder(const std::string& width_str, 
                                  const std::string& style_str, 
                                  const std::string& color_str);
    
    /**
     * @brief 解析浮点数
     * @param str 数字字符串
     * @param default_value 默认值
     * @return 浮点数
     */
    static float ParseFloat(const std::string& str, float default_value = 0.0f);
    
    /**
     * @brief 解析整数
     * @param str 数字字符串
     * @param default_value 默认值
     * @return 整数
     */
    static int ParseInt(const std::string& str, int default_value = 0);

    /**
     * @brief 解析圆角值
     * @param str CSS 圆角字符串（支持 1-4 个值）
     * @return CSSBorderRadius 对象
     */
    static CSSBorderRadius ParseBorderRadius(const std::string& str);

    /**
     * @brief 解析阴影值
     * @param str CSS box-shadow 字符串
     * @return CSSBoxShadow 对象列表（支持多重阴影）
     */
    static std::vector<CSSBoxShadow> ParseBoxShadow(const std::string& str);

    /**
     * @brief 解析线性渐变
     * @param str CSS linear-gradient 字符串
     * @return CSSLinearGradient 对象
     */
    static std::optional<CSSLinearGradient> ParseLinearGradient(const std::string& str);

    /**
     * @brief 解析径向渐变
     * @param str CSS radial-gradient 字符串
     * @return CSSRadialGradient 对象
     */
    static std::optional<CSSRadialGradient> ParseRadialGradient(const std::string& str);

    /**
     * @brief 解析背景重复模式
     * @param str CSS background-repeat 字符串
     * @return CSSBackgroundRepeat
     */
    static CSSBackgroundRepeat ParseBackgroundRepeat(const std::string& str);

    /**
     * @brief 解析背景尺寸
     * @param str CSS background-size 字符串
     * @return CSSBackgroundSize 对象
     */
    static CSSBackgroundSize ParseBackgroundSize(const std::string& str);

private:
    /**
     * @brief 去除字符串首尾空格
     */
    static std::string Trim(const std::string& str);
    
    /**
     * @brief 分割字符串
     */
    static std::vector<std::string> Split(const std::string& str, char delimiter);
};

} // namespace lightui

