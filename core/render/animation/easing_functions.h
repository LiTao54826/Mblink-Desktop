#pragma once

#include "transition.h"

namespace mbink {

/**
 * @brief 缓动函数类
 * 
 * 实现各种标准的缓动函数，用于 CSS transition 和 animation
 * 参考: https://www.w3.org/TR/css-easing-1/
 */
class EasingFunctions {
public:
    /**
     * @brief 线性缓动 (无缓动)
     * @param t 时间参数 (0.0 到 1.0)
     * @return 插值结果 (0.0 到 1.0)
     */
    static float Linear(float t);
    
    /**
     * @brief 标准缓动 (ease)
     * 等价于 cubic-bezier(0.25, 0.1, 0.25, 1.0)
     */
    static float Ease(float t);
    
    /**
     * @brief 缓入 (ease-in)
     * 等价于 cubic-bezier(0.42, 0, 1.0, 1.0)
     */
    static float EaseIn(float t);
    
    /**
     * @brief 缓出 (ease-out)
     * 等价于 cubic-bezier(0, 0, 0.58, 1.0)
     */
    static float EaseOut(float t);
    
    /**
     * @brief 缓入缓出 (ease-in-out)
     * 等价于 cubic-bezier(0.42, 0, 0.58, 1.0)
     */
    static float EaseInOut(float t);
    
    /**
     * @brief 自定义贝塞尔曲线
     * @param t 时间参数 (0.0 到 1.0)
     * @param x1 第一个控制点的 x 坐标
     * @param y1 第一个控制点的 y 坐标
     * @param x2 第二个控制点的 x 坐标
     * @param y2 第二个控制点的 y 坐标
     * @return 插值结果
     */
    static float CubicBezierEasing(float t, float x1, float y1, float x2, float y2);
    
    /**
     * @brief 根据 TimingFunction 类型应用缓动
     * @param t 时间参数 (0.0 到 1.0)
     * @param timing 缓动函数类型
     * @param bezier 贝塞尔曲线参数 (仅当 timing 为 CUBIC_BEZIER 时使用)
     * @return 缓动后的值
     */
    static float Apply(float t, TimingFunction timing, const CubicBezier& bezier = CubicBezier());
};

} // namespace mbink

