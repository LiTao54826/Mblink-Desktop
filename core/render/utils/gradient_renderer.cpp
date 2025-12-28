/**
 * @file gradient_renderer.cpp
 * @brief CSS 渐变渲染器实现
 */

#include "gradient_renderer.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace lightui {

void GradientRenderer::RenderLinearGradient(SkCanvas* canvas,
                                           const SkRect& rect,
                                           const CSSLinearGradient& gradient) {
    if (!canvas || gradient.stops.empty()) {
        return;
    }

    // 提取颜色和位置
    std::vector<SkColor> colors;
    std::vector<SkScalar> positions;
    ExtractColorsAndPositions(gradient.stops, colors, positions);

    if (colors.size() < 2) {
        return; // 至少需要两个颜色
    }

    // 计算渐变起点和终点
    SkPoint pts[2];
    CalculateLinearGradientPoints(rect, gradient.angle, pts[0], pts[1]);

    // 创建线性渐变着色器
    sk_sp<SkShader> shader = SkGradientShader::MakeLinear(
        pts,                             // 起点和终点数组
        colors.data(),                   // 颜色数组
        positions.data(),                // 位置数组
        static_cast<int>(colors.size()), // 颜色数量
        SkTileMode::kClamp               // 平铺模式
    );

    // 创建画笔并设置着色器
    SkPaint paint;
    paint.setShader(shader);
    paint.setAntiAlias(true);

    // 绘制矩形
    canvas->drawRect(rect, paint);
}

void GradientRenderer::RenderRadialGradient(SkCanvas* canvas,
                                           const SkRect& rect,
                                           const CSSRadialGradient& gradient) {
    if (!canvas || gradient.stops.empty()) {
        return;
    }

    // 提取颜色和位置
    std::vector<SkColor> colors;
    std::vector<SkScalar> positions;
    ExtractColorsAndPositions(gradient.stops, colors, positions);

    if (colors.size() < 2) {
        return; // 至少需要两个颜色
    }

    // 计算中心点
    SkPoint center;
    center.fX = rect.left() + rect.width() * gradient.center_x;
    center.fY = rect.top() + rect.height() * gradient.center_y;

    // 计算半径
    SkScalar radius;
    if (gradient.is_circle) {
        // 圆形：使用较短边的一半作为半径
        radius = std::min(rect.width(), rect.height()) / 2.0f;
    } else {
        // 椭圆形：使用对角线的一半作为半径
        radius = std::sqrt(rect.width() * rect.width() + 
                          rect.height() * rect.height()) / 2.0f;
    }

    // 创建径向渐变着色器
    sk_sp<SkShader> shader = SkGradientShader::MakeRadial(
        center,                          // 中心点
        radius,                          // 半径
        colors.data(),                   // 颜色数组
        positions.data(),                // 位置数组
        static_cast<int>(colors.size()), // 颜色数量
        SkTileMode::kClamp               // 平铺模式
    );

    // 创建画笔并设置着色器
    SkPaint paint;
    paint.setShader(shader);
    paint.setAntiAlias(true);

    // 绘制矩形
    canvas->drawRect(rect, paint);
}

void GradientRenderer::CalculateLinearGradientPoints(const SkRect& rect,
                                                     float angle,
                                                     SkPoint& start,
                                                     SkPoint& end) {
    // 将角度转换为弧度
    // CSS 角度：0度 = 向上，90度 = 向右，180度 = 向下，270度 = 向左
    // 需要转换为 Skia 坐标系：0度 = 向右，90度 = 向下
    float radians = (angle - 90.0f) * M_PI / 180.0f;

    // 计算矩形中心
    float centerX = rect.centerX();
    float centerY = rect.centerY();

    // 计算半宽和半高
    float halfWidth = rect.width() / 2.0f;
    float halfHeight = rect.height() / 2.0f;

    // 计算渐变线的长度（从中心到边缘的距离）
    float dx = std::cos(radians);
    float dy = std::sin(radians);

    // 计算与矩形边界的交点距离
    float distance;
    if (std::abs(dx) < 0.0001f) {
        // 垂直方向
        distance = halfHeight;
    } else if (std::abs(dy) < 0.0001f) {
        // 水平方向
        distance = halfWidth;
    } else {
        // 计算到四条边的距离，取最小值
        float distX = std::abs(halfWidth / dx);
        float distY = std::abs(halfHeight / dy);
        distance = std::min(distX, distY);
    }

    // 计算起点和终点
    start.fX = centerX - dx * distance;
    start.fY = centerY - dy * distance;
    end.fX = centerX + dx * distance;
    end.fY = centerY + dy * distance;
}

void GradientRenderer::ExtractColorsAndPositions(const std::vector<CSSGradientStop>& stops,
                                                 std::vector<SkColor>& colors,
                                                 std::vector<SkScalar>& positions) {
    colors.clear();
    positions.clear();

    for (const auto& stop : stops) {
        colors.push_back(stop.color);
        positions.push_back(stop.position);
    }

    // 确保位置是递增的
    for (size_t i = 1; i < positions.size(); i++) {
        if (positions[i] < positions[i - 1]) {
            positions[i] = positions[i - 1];
        }
    }
}

} // namespace lightui

