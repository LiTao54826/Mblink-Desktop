/**
 * @file animation.h
 * @brief CSS animation 属性定义和解析
 * @author MBink Development Team
 * @date 2025-11-14
 */

#ifndef LIGHTUI_CORE_RENDER_ANIMATION_H_
#define LIGHTUI_CORE_RENDER_ANIMATION_H_

#include "transition.h"  // 复用 TimingFunction 和 CubicBezier
#include <string>
#include <vector>

namespace lightui {

/**
 * @brief 动画播放方向
 */
enum class AnimationDirection {
    ANIM_NORMAL,            ///< 正常播放 (0% → 100%)
    ANIM_REVERSE,           ///< 反向播放 (100% → 0%)
    ANIM_ALTERNATE,         ///< 交替播放 (奇数次正向，偶数次反向)
    ANIM_ALTERNATE_REVERSE  ///< 反向交替播放 (奇数次反向，偶数次正向)
};

/**
 * @brief 动画填充模式
 * 
 * 控制动画在执行前后如何应用样式。
 */
enum class AnimationFillMode {
    NONE,      ///< 不填充 (动画前后不应用任何样式)
    FORWARDS,  ///< 向前填充 (保持最后一帧的样式)
    BACKWARDS, ///< 向后填充 (应用第一帧的样式)
    BOTH       ///< 双向填充 (应用第一帧和最后一帧的样式)
};

/**
 * @brief CSS animation 属性
 * 
 * 表示一个完整的 CSS animation 配置。
 * 
 * 支持的 CSS 语法:
 * @code{.css}
 * // 简写属性
 * animation: slide-in 0.5s ease-in-out;
 * animation: fade-in 1s ease 0.5s infinite alternate;
 * animation: bounce 0.3s ease-in-out 3;
 * 
 * // 分解属性
 * animation-name: slide-in;
 * animation-duration: 0.5s;
 * animation-timing-function: ease-in-out;
 * animation-delay: 0.1s;
 * animation-iteration-count: infinite;
 * animation-direction: alternate;
 * animation-fill-mode: forwards;
 * animation-play-state: paused;
 * @endcode
 */
struct CSSAnimation {
    std::string name;                   ///< 动画名称 (对应 @keyframes 规则)
    float duration;                     ///< 持续时间 (秒)
    TimingFunction timing_function;     ///< 缓动函数类型
    CubicBezier bezier;                ///< 自定义贝塞尔曲线 (当 timing_function == CUBIC_BEZIER 时使用)
    float delay;                        ///< 延迟时间 (秒)
    int iteration_count;                ///< 迭代次数 (-1 表示 infinite)
    AnimationDirection direction;       ///< 播放方向
    AnimationFillMode fill_mode;        ///< 填充模式
    bool paused;                        ///< 是否暂停
    
    /**
     * @brief 默认构造函数
     */
    CSSAnimation()
        : name("")
        , duration(0)
        , timing_function(TimingFunction::EASE)
        , bezier()
        , delay(0)
        , iteration_count(1)
        , direction(AnimationDirection::ANIM_NORMAL)
        , fill_mode(AnimationFillMode::NONE)
        , paused(false) {}
    
    /**
     * @brief 解析 animation 简写属性
     * 
     * 解析 CSS animation 简写属性字符串，提取所有动画配置。
     * 
     * 支持的语法:
     * - animation: name duration timing-function delay iteration-count direction fill-mode;
     * - animation: name1 1s, name2 2s;  // 多个动画
     * 
     * @param str CSS animation 字符串
     * @return 解析后的 CSSAnimation 列表
     * 
     * @note 如果解析失败，返回空列表
     * 
     * 示例:
     * @code
     * std::string css = "slide-in 0.5s ease-in-out 0.1s 3 alternate forwards";
     * std::vector<CSSAnimation> animations = CSSAnimation::Parse(css);
     * @endcode
     */
    static std::vector<CSSAnimation> Parse(const std::string& str);
    
    /**
     * @brief 解析 animation-name 属性
     * @param str CSS animation-name 字符串
     * @return 动画名称列表
     */
    static std::vector<std::string> ParseName(const std::string& str);
    
    /**
     * @brief 解析 animation-duration 属性
     * @param str CSS animation-duration 字符串
     * @return 持续时间列表 (秒)
     */
    static std::vector<float> ParseDuration(const std::string& str);
    
    /**
     * @brief 解析 animation-timing-function 属性
     * @param str CSS animation-timing-function 字符串
     * @return timing function 和 bezier 的列表
     */
    static std::vector<std::pair<TimingFunction, CubicBezier>> ParseTimingFunction(const std::string& str);
    
    /**
     * @brief 解析 animation-delay 属性
     * @param str CSS animation-delay 字符串
     * @return 延迟时间列表 (秒)
     */
    static std::vector<float> ParseDelay(const std::string& str);
    
    /**
     * @brief 解析 animation-iteration-count 属性
     * @param str CSS animation-iteration-count 字符串
     * @return 迭代次数列表 (-1 表示 infinite)
     */
    static std::vector<int> ParseIterationCount(const std::string& str);
    
    /**
     * @brief 解析 animation-direction 属性
     * @param str CSS animation-direction 字符串
     * @return 播放方向列表
     */
    static std::vector<AnimationDirection> ParseDirection(const std::string& str);
    
    /**
     * @brief 解析 animation-fill-mode 属性
     * @param str CSS animation-fill-mode 字符串
     * @return 填充模式列表
     */
    static std::vector<AnimationFillMode> ParseFillMode(const std::string& str);
    
    /**
     * @brief 解析 animation-play-state 属性
     * @param str CSS animation-play-state 字符串
     * @return true 如果是 paused，false 如果是 running
     */
    static bool ParsePlayState(const std::string& str);
    
    /**
     * @brief 检查动画配置是否有效
     * @return true 如果有效 (有名称且持续时间 > 0)
     */
    bool IsValid() const {
        return !name.empty() && duration > 0.0f;
    }
    
private:
    /**
     * @brief 去除字符串首尾空白
     */
    static std::string Trim(const std::string& str);
    
    /**
     * @brief 解析时间值 (支持 s 和 ms)
     * @param str 时间字符串 (如 "0.5s", "500ms")
     * @return 时间值 (秒)
     */
    static float ParseTime(const std::string& str);
    
    /**
     * @brief 解析单个 timing function
     * @param str timing function 字符串
     * @return timing function 和 bezier
     */
    static std::pair<TimingFunction, CubicBezier> ParseSingleTimingFunction(const std::string& str);
    
    /**
     * @brief 解析单个 direction
     * @param str direction 字符串
     * @return AnimationDirection
     */
    static AnimationDirection ParseSingleDirection(const std::string& str);
    
    /**
     * @brief 解析单个 fill-mode
     * @param str fill-mode 字符串
     * @return AnimationFillMode
     */
    static AnimationFillMode ParseSingleFillMode(const std::string& str);
};

} // namespace lightui

#endif // LIGHTUI_CORE_RENDER_ANIMATION_H_

