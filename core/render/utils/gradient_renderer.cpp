/**
 * @file gradient_renderer.cpp
 * @brief CSS 渐变渲染器实现
 */

#include "gradient_renderer.h"
#include <cmath>
#include <algorithm>
#include "include/core/SkPictureRecorder.h"
#include "include/core/SkPicture.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace mbink {

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

void GradientRenderer::RenderTiledLinearGradient(SkCanvas* canvas,
                                                  const SkRect& rect,
                                                  const CSSLinearGradient& gradient,
                                                  float tile_width,
                                                  float tile_height) {
    if (!canvas || gradient.stops.empty() || tile_width <= 0 || tile_height <= 0) {
        return;
    }

    // 根据渐变角度确定渐变方向的长度
    // 0/180度：垂直方向，使用 tile_height
    // 90/270度：水平方向，使用 tile_width
    float gradient_length = tile_height;  // 默认垂直方向
    float angle = gradient.angle;

    // 标准化角度到 0-360
    while (angle < 0) angle += 360;
    while (angle >= 360) angle -= 360;

    // 判断主要方向
    if ((angle > 45 && angle < 135) || (angle > 225 && angle < 315)) {
        // 主要是水平方向
        gradient_length = tile_width;
    }

    // 处理像素值的 stops，根据实际 tile 尺寸计算相对位置
    std::vector<CSSGradientStop> adjusted_stops;
    for (const auto& stop : gradient.stops) {
        CSSGradientStop adjusted = stop;
        if (stop.is_pixel) {
            // 将像素值转换为相对位置
            adjusted.position = stop.pixel_value / gradient_length;
            // 确保位置在 0-1 范围内
            if (adjusted.position > 1.0f) adjusted.position = 1.0f;
            if (adjusted.position < 0.0f) adjusted.position = 0.0f;
        }
        adjusted_stops.push_back(adjusted);
    }

    // 提取颜色和位置
    std::vector<SkColor> colors;
    std::vector<SkScalar> positions;
    ExtractColorsAndPositions(adjusted_stops, colors, positions);

    if (colors.size() < 2) {
        return;
    }

    // 使用 SkPictureRecorder 创建一个 tile 图案
    // 这样可以实现真正的二维平铺，而不是只沿渐变线方向重复
    SkPictureRecorder recorder;
    SkRect tile_rect = SkRect::MakeWH(tile_width, tile_height);
    SkCanvas* tile_canvas = recorder.beginRecording(tile_rect);

    // 计算渐变起点和终点（在平铺单元内）
    SkPoint pts[2];
    CalculateLinearGradientPoints(tile_rect, gradient.angle, pts[0], pts[1]);

    // 创建线性渐变着色器（使用 kClamp 模式，因为我们会用 Picture 来平铺）
    sk_sp<SkShader> gradient_shader = SkGradientShader::MakeLinear(
        pts,
        colors.data(),
        positions.data(),
        static_cast<int>(colors.size()),
        SkTileMode::kClamp
    );

    // 在 tile canvas 上绘制渐变
    SkPaint tile_paint;
    tile_paint.setShader(gradient_shader);
    tile_paint.setAntiAlias(true);
    tile_canvas->drawRect(tile_rect, tile_paint);

    // 完成录制并创建 Picture
    sk_sp<SkPicture> picture = recorder.finishRecordingAsPicture();

    // 使用 Picture 创建平铺 shader
    // SkTileMode::kRepeat 会在水平和垂直两个方向都重复
    sk_sp<SkShader> tiled_shader = picture->makeShader(
        SkTileMode::kRepeat,
        SkTileMode::kRepeat,
        SkFilterMode::kNearest,  // 使用最近邻采样，保持锐利的线条
        nullptr,  // 不需要额外的矩阵变换
        &tile_rect
    );

    // 创建画笔并设置平铺 shader
    SkPaint paint;
    paint.setShader(tiled_shader);
    paint.setAntiAlias(false);  // 对于网格线，关闭抗锯齿以保持锐利

    // 绘制整个区域
    canvas->drawRect(rect, paint);
}

void GradientRenderer::RenderMultipleLinearGradients(SkCanvas* canvas,
                                                      const SkRect& rect,
                                                      const std::vector<CSSLinearGradient>& gradients,
                                                      const std::vector<CSSBackgroundSize>& sizes) {
    if (!canvas || gradients.empty()) {
        return;
    }

    // CSS 规范：多层背景从后往前绘制（最后一个先绘制，第一个最后绘制在最上面）
    for (int i = static_cast<int>(gradients.size()) - 1; i >= 0; i--) {
        const auto& gradient = gradients[i];

        // 获取对应的 background-size（如果有）
        float tile_width = rect.width();
        float tile_height = rect.height();

        if (i < static_cast<int>(sizes.size())) {
            const auto& size = sizes[i];
            if (size.type == CSSBackgroundSize::Type::LENGTH) {
                // 使用指定的尺寸
                if (size.width.unit != CSSUnit::AUTO) {
                    tile_width = size.width.ToPx();
                }
                if (size.height.unit != CSSUnit::AUTO) {
                    tile_height = size.height.ToPx();
                }
            }
            // COVER 和 CONTAIN 暂不处理，保持默认
        }

        // 如果平铺尺寸小于整个区域，使用平铺渲染
        if (tile_width < rect.width() || tile_height < rect.height()) {
            RenderTiledLinearGradient(canvas, rect, gradient, tile_width, tile_height);
        } else {
            // 否则使用普通渲染
            RenderLinearGradient(canvas, rect, gradient);
        }
    }
}

} // namespace mbink

