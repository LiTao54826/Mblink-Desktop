#pragma once

#include <string>
#include <vector>
#include <optional>

namespace mbink {

/**
 * @brief CSS transition-timing-function 类型
 */
enum class TimingFunction {
    LINEAR,
    EASE,
    EASE_IN,
    EASE_OUT,
    EASE_IN_OUT,
    CUBIC_BEZIER
};

/**
 * @brief 贝塞尔曲线参数
 * 用于 cubic-bezier(x1, y1, x2, y2) 缓动函数
 */
struct CubicBezier {
    float x1, y1, x2, y2;
    
    CubicBezier() : x1(0), y1(0), x2(1), y2(1) {}
    CubicBezier(float x1, float y1, float x2, float y2)
        : x1(x1), y1(y1), x2(x2), y2(y2) {}
    
    /**
     * @brief 计算给定时间点的值
     * @param t 时间参数 (0.0 到 1.0)
     * @return 插值结果
     */
    float Evaluate(float t) const;
};

/**
 * @brief CSS transition 属性
 * 
 * 支持的语法:
 * - transition: all 0.3s ease;
 * - transition: opacity 0.5s ease-in-out;
 * - transition: transform 0.3s cubic-bezier(0.4, 0, 0.2, 1);
 * - transition: width 0.3s, height 0.3s;
 */
struct CSSTransition {
    std::string property;           // 过渡属性名 (e.g., "opacity", "all")
    float duration;                 // 持续时间 (秒)
    TimingFunction timing_function; // 缓动函数类型
    CubicBezier bezier;            // 自定义贝塞尔曲线参数
    float delay;                    // 延迟时间 (秒)
    
    CSSTransition()
        : property("all")
        , duration(0)
        , timing_function(TimingFunction::EASE)
        , delay(0) {}
    
    /**
     * @brief 解析 transition 简写属性
     * @param str CSS transition 字符串
     * @return 解析后的 transition 列表
     * 
     * 示例:
     * - "all 0.3s ease" -> [{property: "all", duration: 0.3, timing: EASE, delay: 0}]
     * - "width 0.3s, height 0.5s" -> [{property: "width", ...}, {property: "height", ...}]
     */
    static std::vector<CSSTransition> Parse(const std::string& str);
    
    /**
     * @brief 解析 transition-property
     * @param str CSS transition-property 字符串
     * @return 属性名列表
     * 
     * 示例:
     * - "all" -> ["all"]
     * - "width, height" -> ["width", "height"]
     */
    static std::vector<std::string> ParseProperty(const std::string& str);
    
    /**
     * @brief 解析 transition-duration
     * @param str CSS transition-duration 字符串
     * @return 持续时间列表 (秒)
     * 
     * 示例:
     * - "0.3s" -> [0.3]
     * - "300ms" -> [0.3]
     * - "0.3s, 0.5s" -> [0.3, 0.5]
     */
    static std::vector<float> ParseDuration(const std::string& str);
    
    /**
     * @brief 解析 transition-timing-function
     * @param str CSS transition-timing-function 字符串
     * @return 缓动函数列表 (类型 + 贝塞尔参数)
     * 
     * 示例:
     * - "ease" -> [(EASE, default_bezier)]
     * - "cubic-bezier(0.4, 0, 0.2, 1)" -> [(CUBIC_BEZIER, {0.4, 0, 0.2, 1})]
     */
    static std::vector<std::pair<TimingFunction, CubicBezier>> ParseTimingFunction(const std::string& str);
    
    /**
     * @brief 解析 transition-delay
     * @param str CSS transition-delay 字符串
     * @return 延迟时间列表 (秒)
     * 
     * 示例:
     * - "0s" -> [0]
     * - "0.5s" -> [0.5]
     * - "0s, 0.2s" -> [0, 0.2]
     */
    static std::vector<float> ParseDelay(const std::string& str);

private:
    /**
     * @brief 解析单个时间值 (支持 s 和 ms)
     */
    static float ParseTime(const std::string& str);
    
    /**
     * @brief 解析单个 timing function
     */
    static std::pair<TimingFunction, CubicBezier> ParseSingleTimingFunction(const std::string& str);
};

} // namespace mbink

