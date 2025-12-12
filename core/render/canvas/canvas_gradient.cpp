/**
 * @file canvas_gradient.cpp
 * @brief Canvas渐变对象实现
 */

#include "canvas_gradient.h"
#include "core/render/color.h"
#include <algorithm>

namespace lightui {

CanvasGradient::CanvasGradient(CanvasGradientType type)
    : type_(type)
    , x0_(0), y0_(0), x1_(0), y1_(0)
    , r0_(0), r1_(0) {
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
    } else {
        // 径向渐变
        return SkGradientShader::MakeTwoPointConical(
            SkPoint::Make(x0_, y0_), r0_,
            SkPoint::Make(x1_, y1_), r1_,
            colors.data(), positions.data(), colors.size(),
            SkTileMode::kClamp
        );
    }
}

} // namespace lightui
