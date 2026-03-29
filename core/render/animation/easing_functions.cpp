#include "easing_functions.h"
#include <algorithm>
#include <cmath>

namespace mbink {

float EasingFunctions::Linear(float t) {
    return t;
}

float EasingFunctions::Ease(float t) {
    // ease = cubic-bezier(0.25, 0.1, 0.25, 1.0)
    return CubicBezierEasing(t, 0.25f, 0.1f, 0.25f, 1.0f);
}

float EasingFunctions::EaseIn(float t) {
    // ease-in = cubic-bezier(0.42, 0, 1.0, 1.0)
    return CubicBezierEasing(t, 0.42f, 0.0f, 1.0f, 1.0f);
}

float EasingFunctions::EaseOut(float t) {
    // ease-out = cubic-bezier(0, 0, 0.58, 1.0)
    return CubicBezierEasing(t, 0.0f, 0.0f, 0.58f, 1.0f);
}

float EasingFunctions::EaseInOut(float t) {
    // ease-in-out = cubic-bezier(0.42, 0, 0.58, 1.0)
    return CubicBezierEasing(t, 0.42f, 0.0f, 0.58f, 1.0f);
}

float EasingFunctions::CubicBezierEasing(float t, float x1, float y1, float x2, float y2) {
    // 边界情况
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 1.0f;
    
    // 线性情况的优化
    if (x1 == y1 && x2 == y2) {
        return t;
    }
    
    // 使用牛顿迭代法求解贝塞尔曲线
    // 目标: 找到参数 s，使得 x(s) = t
    // 然后返回 y(s)
    
    float s = t;  // 初始猜测
    const int max_iterations = 8;
    const float epsilon = 1e-6f;
    
    for (int i = 0; i < max_iterations; ++i) {
        // 计算 x(s) 使用三次贝塞尔公式
        // B(s) = (1-s)^3 * P0 + 3*(1-s)^2*s * P1 + 3*(1-s)*s^2 * P2 + s^3 * P3
        // 其中 P0 = (0,0), P1 = (x1,y1), P2 = (x2,y2), P3 = (1,1)
        
        float s2 = s * s;
        float s3 = s2 * s;
        float one_minus_s = 1.0f - s;
        float one_minus_s2 = one_minus_s * one_minus_s;
        float one_minus_s3 = one_minus_s2 * one_minus_s;
        
        // x(s) = 3*(1-s)^2*s*x1 + 3*(1-s)*s^2*x2 + s^3
        float x = 3.0f * one_minus_s2 * s * x1 +
                  3.0f * one_minus_s * s2 * x2 +
                  s3;
        
        // 如果足够接近目标 x = t，退出
        if (std::abs(x - t) < epsilon) {
            break;
        }
        
        // 计算导数 dx/ds
        // dx/ds = 3*(1-s)^2*x1 + 6*(1-s)*s*(x2-x1) + 3*s^2*(1-x2)
        float dx = 3.0f * one_minus_s2 * x1 +
                   6.0f * one_minus_s * s * (x2 - x1) +
                   3.0f * s2 * (1.0f - x2);
        
        if (std::abs(dx) < epsilon) {
            break;
        }
        
        // 牛顿迭代: s_new = s - (x(s) - t) / dx
        s = s - (x - t) / dx;
        
        // 限制在 [0, 1] 范围内
        s = std::max(0.0f, std::min(1.0f, s));
    }
    
    // 计算 y(s)
    float s2 = s * s;
    float s3 = s2 * s;
    float one_minus_s = 1.0f - s;
    float one_minus_s2 = one_minus_s * one_minus_s;
    
    // y(s) = 3*(1-s)^2*s*y1 + 3*(1-s)*s^2*y2 + s^3
    float y = 3.0f * one_minus_s2 * s * y1 +
              3.0f * one_minus_s * s2 * y2 +
              s3;
    
    return y;
}

float EasingFunctions::Apply(float t, TimingFunction timing, const CubicBezier& bezier) {
    // 确保 t 在 [0, 1] 范围内
    t = std::max(0.0f, std::min(1.0f, t));
    
    switch (timing) {
        case TimingFunction::LINEAR:
            return Linear(t);
        
        case TimingFunction::EASE:
            return Ease(t);
        
        case TimingFunction::EASE_IN:
            return EaseIn(t);
        
        case TimingFunction::EASE_OUT:
            return EaseOut(t);
        
        case TimingFunction::EASE_IN_OUT:
            return EaseInOut(t);
        
        case TimingFunction::CUBIC_BEZIER:
            return CubicBezierEasing(t, bezier.x1, bezier.y1, bezier.x2, bezier.y2);
        
        default:
            return t;
    }
}

} // namespace mbink

