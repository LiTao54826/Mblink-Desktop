/**
 * @file canvas_gradient.cpp
 * @brief Canvas渐变对象实现
 */

#include "canvas_gradient.h"
#include "core/render/utils/color.h"
#include <algorithm>

namespace mbink {

CanvasGradient::CanvasGradient(CanvasGradientType type)
    : type_(type)
    , x0_(0), y0_(0), x1_(0), y1_(0)
    , r0_(0), r1_(0)
    , start_angle_(0) {
}

CanvasGradient* CanvasGradient::CreateLinear(double x0, double y0, double x1, double y1) {
    CanvasGradient* gradient = new CanvasGradient(CanvasGradientType::LINEAR);
    gradient->x0_ = x0;
    gradient->y0_ = y0;
    gradient->x1_ = x1;
    gradient->y1_ = y1;
    return gradient;
}

CanvasGradient* CanvasGradient::CreateRadial(double x0, double y0, double r0, double x1, double y1, double r1) {
    CanvasGradient* gradient = new CanvasGradient(CanvasGradientType::RADIAL);
    gradient->x0_ = x0;
    gradient->y0_ = y0;
    gradient->r0_ = r0;
    gradient->x1_ = x1;
    gradient->y1_ = y1;
    gradient->r1_ = r1;
    return gradient;
}

CanvasGradient* CanvasGradient::CreateConic(double startAngle, double x, double y) {
    CanvasGradient* gradient = new CanvasGradient(CanvasGradientType::CONIC);
    gradient->start_angle_ = startAngle;
    gradient->x0_ = x;
    gradient->y0_ = y;
    return gradient;
}

void CanvasGradient::AddColorStop(double offset, const std::string& color) {
    // 限制offset在0-1范围
    offset = std::max(0.0, std::min(1.0, offset));
    
    ColorStop stop;
    stop.offset = offset;
    stop.color = Color::Parse(color);
    
    // 按offset顺序插入
    auto it = color_stops_.begin();
    while (it != color_stops_.end() && it->offset < offset) {
        ++it;
    }
    color_stops_.insert(it, stop);
}

sk_sp<SkShader> CanvasGradient::GetShader() const {
    if (color_stops_.empty()) {
        return nullptr;
    }
    
    // 准备颜色和位置数组
    std::vector<SkColor> colors;
    std::vector<SkScalar> positions;
    
    for (const auto& stop : color_stops_) {
        colors.push_back(stop.color);
        positions.push_back(static_cast<SkScalar>(stop.offset));
    }
    
    if (type_ == CanvasGradientType::LINEAR) {
        SkPoint pts[2] = {
            SkPoint::Make(x0_, y0_),
            SkPoint::Make(x1_, y1_)
        };
        return SkGradientShader::MakeLinear(
            pts, colors.data(), positions.data(), colors.size(),
            SkTileMode::kClamp
        );
    } else if (type_ == CanvasGradientType::RADIAL) {
        // 径向渐变
        return SkGradientShader::MakeTwoPointConical(
            SkPoint::Make(x0_, y0_), r0_,
            SkPoint::Make(x1_, y1_), r1_,
            colors.data(), positions.data(), colors.size(),
            SkTileMode::kClamp
        );
    } else {
        // 锥形渐变
        // Skia的Sweep渐变从0度开始，需要转换startAngle
        // HTML5 Canvas锥形渐变：startAngle是起始角度（弧度），0度在右侧，顺时针旋转
        // Skia Sweep渐变：从0度开始（3点钟方向），顺时针
        float startDegrees = static_cast<float>(start_angle_ * 180.0 / 3.14159265358979323846);
        
        // 使用MakeSweep创建锥形渐变
        return SkGradientShader::MakeSweep(
            x0_, y0_,
            colors.data(), positions.data(), colors.size(),
            SkTileMode::kClamp,
            startDegrees, startDegrees + 360.0f,  // 完整360度旋转
            0, nullptr
        );
    }
}

} // namespace mbink
