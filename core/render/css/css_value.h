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

#include <memory>
#include <optional>
#include <string>
#include <vector>
#include "include/core/SkColor.h"

namespace mblink {

/**
 * @brief CSS 单位类型
 */
enum class CSSUnit {
    PX,      // 像素
    PERCENT, // 百分比
    EM,      // 相对于字体大小
    REM,     // 相对于根元素字体大小
    VW,      // 视口宽度百分比
    VH,      // 视口高度百分比
    VMIN,    // 视口最小尺寸百分比 (min(vw, vh))
    VMAX,    // 视口最大尺寸百分比 (max(vw, vh))
    AUTO,    // 自动
    NONE     // 无单位
};

/**
 * @brief CSS 长度值
 */
struct CSSLength {
    float value;
    CSSUnit unit;

    enum class FunctionType {
        NONE,
        MIN,
        MAX,
        CLAMP
    };

    // calc() 表达式支持
    bool is_calc = false;
    float calc_percent = 0.0f;  // 百分比部分 (如 100%)
    float calc_px = 0.0f;       // 像素部分 (如 -40px)

    // min()/max()/clamp() 表达式支持
    FunctionType function_type = FunctionType::NONE;
    std::shared_ptr<CSSLength> func_a;
    std::shared_ptr<CSSLength> func_b;
    std::shared_ptr<CSSLength> func_c;

    CSSLength() : value(0.0f), unit(CSSUnit::PX) {}
    CSSLength(float v, CSSUnit u) : value(v), unit(u) {}

    /**
     * @brief 创建 calc 表达式
     * @param percent 百分比值 (如 100 表示 100%)
     * @param px 像素值 (如 -40 表示 -40px)
     */
    static CSSLength Calc(float percent, float px) {
        CSSLength len;
        len.is_calc = true;
        len.calc_percent = percent;
        len.calc_px = px;
        len.unit = CSSUnit::PX;  // calc 结果是像素
        return len;
    }

    static CSSLength Min(const CSSLength& a, const CSSLength& b) {
        CSSLength len;
        len.function_type = FunctionType::MIN;
        len.func_a = std::make_shared<CSSLength>(a);
        len.func_b = std::make_shared<CSSLength>(b);
        return len;
    }

    static CSSLength Max(const CSSLength& a, const CSSLength& b) {
        CSSLength len;
        len.function_type = FunctionType::MAX;
        len.func_a = std::make_shared<CSSLength>(a);
        len.func_b = std::make_shared<CSSLength>(b);
        return len;
    }

    static CSSLength Clamp(const CSSLength& min_value,
                           const CSSLength& preferred_value,
                           const CSSLength& max_value) {
        CSSLength len;
        len.function_type = FunctionType::CLAMP;
        len.func_a = std::make_shared<CSSLength>(min_value);
        len.func_b = std::make_shared<CSSLength>(preferred_value);
        len.func_c = std::make_shared<CSSLength>(max_value);
        return len;
    }

    /**
     * @brief 转换为像素值
     * @param base_value 基准值（用于百分比和 em/rem 计算）
     * @param font_size 字体大小（用于 em 计算）
     * @param root_font_size 根字体大小（用于 rem 计算）
     * @return 像素值
     * @note vh/vw 单位会自动从 RenderObject::GetViewportWidth/Height() 获取视口尺寸
     */
    float ToPx(float base_value = 0.0f, float font_size = 16.0f, float root_font_size = 16.0f) const;

    /**
     * @brief 是否为自动值
     */
    bool IsAuto() const { return unit == CSSUnit::AUTO; }

    /**
     * @brief 是否为零值
     */
    bool IsZero() const {
        return value == 0.0f && unit != CSSUnit::AUTO && !is_calc && function_type == FunctionType::NONE;
    }

    /**
     * @brief 比较运算符
     */
    bool operator==(const CSSLength& other) const {
        if (is_calc != other.is_calc) return false;
        if (is_calc) {
            return calc_percent == other.calc_percent && calc_px == other.calc_px;
        }
        if (function_type != other.function_type) return false;
        if (function_type != FunctionType::NONE) {
            auto ptr_equal = [](const std::shared_ptr<CSSLength>& lhs,
                                const std::shared_ptr<CSSLength>& rhs) {
                if (!lhs || !rhs) {
                    return lhs == rhs;
                }
                return *lhs == *rhs;
            };
            return ptr_equal(func_a, other.func_a) &&
                   ptr_equal(func_b, other.func_b) &&
                   ptr_equal(func_c, other.func_c);
        }
        return value == other.value && unit == other.unit;
    }

    bool operator!=(const CSSLength& other) const {
        return !(*this == other);
    }
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
 * @brief CSS 文本阴影定义
 */
struct CSSTextShadow {
    float offset_x;      // X 偏移
    float offset_y;      // Y 偏移
    float blur_radius;   // 模糊半径
    SkColor color;       // 阴影颜色

    CSSTextShadow()
        : offset_x(0), offset_y(0), blur_radius(0),
          color(SK_ColorBLACK) {}
};

/**
 * @brief CSS 渐变色停止点
 */
struct CSSGradientStop {
    SkColor color;
    float position; // 0.0 到 1.0（相对位置）

    // 像素值支持：当使用像素单位时，需要在渲染时根据 background-size 动态计算
    bool is_pixel = false;      // 是否使用像素单位
    float pixel_value = 0.0f;   // 原始像素值

    CSSGradientStop() : color(SK_ColorBLACK), position(0.0f) {}
    CSSGradientStop(SkColor c, float p) : color(c), position(p) {}
};

/**
 * @brief CSS 线性渐变定义
 */
struct CSSLinearGradient {
    float angle;  // 角度（度数），CSS 默认是 180度（从上到下）
    std::vector<CSSGradientStop> stops;

    CSSLinearGradient() : angle(180.0f) {}  // CSS 默认方向是从上到下
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
 * @brief 视口尺寸管理（用于 vh/vw 单位计算）
 */
class ViewportSize {
public:
    static void Set(float width, float height);
    static float GetWidth() { return width_; }
    static float GetHeight() { return height_; }
private:
    static float width_;
    static float height_;
};

// 便捷函数
inline float GetViewportWidth() { return ViewportSize::GetWidth(); }
inline float GetViewportHeight() { return ViewportSize::GetHeight(); }

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
     * @brief 解析 calc() 表达式
     * @param str calc() 表达式字符串（如 "calc(100% - 40px)"）
     * @return CSSLength 对象
     */
    static CSSLength ParseCalc(const std::string& str);

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
     * @brief 解析文本阴影值
     * @param str CSS text-shadow 字符串
     * @return CSSTextShadow 对象列表（支持多重阴影）
     */
    static std::vector<CSSTextShadow> ParseTextShadow(const std::string& str);

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

    /**
     * @brief 解析多个线性渐变（逗号分隔）
     * @param str CSS background-image 字符串，包含多个 linear-gradient
     * @return CSSLinearGradient 对象列表
     *
     * 示例: "linear-gradient(red, blue), linear-gradient(90deg, green, yellow)"
     * 注意: 正确处理渐变函数内部的逗号（不作为分隔符）
     */
    static std::vector<CSSLinearGradient> ParseMultipleLinearGradients(const std::string& str);

    /**
     * @brief 解析多个背景尺寸（逗号分隔）
     * @param str CSS background-size 字符串，包含多个尺寸值
     * @return CSSBackgroundSize 对象列表
     *
     * 示例: "8px 8px, cover, 100% 50%"
     */
    static std::vector<CSSBackgroundSize> ParseMultipleBackgroundSizes(const std::string& str);

    static std::vector<std::string> SplitTopLevel(const std::string& str, char delimiter);

    /**
     * @brief 去除字符串首尾空格
     */
    static std::string Trim(const std::string& str);

    /**
     * @brief 分割字符串
     */
    static std::vector<std::string> Split(const std::string& str, char delimiter);

private:
};

} // namespace mblink

