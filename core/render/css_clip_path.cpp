/**
 * @file css_clip_path.cpp
 * @brief CSS clip-path 属性解析和转换实现
 */

#include "css_clip_path.h"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <regex>

namespace lightui {

namespace {

// 辅助函数：去除首尾空格
std::string Trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(" \t\n\r");
    return str.substr(start, end - start + 1);
}

// 辅助函数：分割字符串
std::vector<std::string> Split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    while (std::getline(ss, token, delimiter)) {
        std::string trimmed = Trim(token);
        if (!trimmed.empty()) {
            tokens.push_back(trimmed);
        }
    }
    return tokens;
}

// 辅助函数：按空格分割（处理多个空格）
std::vector<std::string> SplitBySpace(const std::string& str) {
    std::vector<std::string> tokens;
    std::istringstream iss(str);
    std::string token;
    while (iss >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

// 辅助函数：解析长度值
CSSLength ParseLengthValue(const std::string& str) {
    std::string s = Trim(str);
    if (s.empty()) {
        return CSSLength(0, CSSUnit::PX);
    }
    
    // 检查百分比
    if (s.back() == '%') {
        float value = std::stof(s.substr(0, s.length() - 1));
        return CSSLength(value, CSSUnit::PERCENT);
    }
    
    // 检查 px
    if (s.length() > 2 && s.substr(s.length() - 2) == "px") {
        float value = std::stof(s.substr(0, s.length() - 2));
        return CSSLength(value, CSSUnit::PX);
    }
    
    // 检查 em
    if (s.length() > 2 && s.substr(s.length() - 2) == "em") {
        float value = std::stof(s.substr(0, s.length() - 2));
        return CSSLength(value, CSSUnit::EM);
    }
    
    // 检查 rem
    if (s.length() > 3 && s.substr(s.length() - 3) == "rem") {
        float value = std::stof(s.substr(0, s.length() - 3));
        return CSSLength(value, CSSUnit::REM);
    }
    
    // 纯数字，默认为像素
    try {
        float value = std::stof(s);
        return CSSLength(value, CSSUnit::PX);
    } catch (...) {
        return CSSLength(0, CSSUnit::PX);
    }
}

// 辅助函数：解析位置关键字
CSSLength ParsePositionKeyword(const std::string& keyword, bool is_x) {
    std::string k = Trim(keyword);
    std::transform(k.begin(), k.end(), k.begin(), ::tolower);
    
    if (k == "center") {
        return CSSLength(50.0f, CSSUnit::PERCENT);
    } else if (k == "left" && is_x) {
        return CSSLength(0.0f, CSSUnit::PERCENT);
    } else if (k == "right" && is_x) {
        return CSSLength(100.0f, CSSUnit::PERCENT);
    } else if (k == "top" && !is_x) {
        return CSSLength(0.0f, CSSUnit::PERCENT);
    } else if (k == "bottom" && !is_x) {
        return CSSLength(100.0f, CSSUnit::PERCENT);
    }
    
    // 尝试解析为长度值
    return ParseLengthValue(k);
}

// 辅助函数：提取函数参数
std::string ExtractFunctionParams(const std::string& value, const std::string& funcName) {
    size_t start = value.find(funcName + "(");
    if (start == std::string::npos) return "";
    
    start += funcName.length() + 1;
    size_t end = value.rfind(')');
    if (end == std::string::npos || end <= start) return "";
    
    return value.substr(start, end - start);
}

// 计算到最近边的距离
float CalculateClosestSide(float cx, float cy, float width, float height) {
    float dist_left = cx;
    float dist_right = width - cx;
    float dist_top = cy;
    float dist_bottom = height - cy;
    return std::min({dist_left, dist_right, dist_top, dist_bottom});
}

// 计算到最远边的距离
float CalculateFarthestSide(float cx, float cy, float width, float height) {
    float dist_left = cx;
    float dist_right = width - cx;
    float dist_top = cy;
    float dist_bottom = height - cy;
    return std::max({dist_left, dist_right, dist_top, dist_bottom});
}

} // anonymous namespace

// ============================================================================
// CSSClipPath::ToSkPath 实现
// ============================================================================

SkPath CSSClipPath::ToSkPath(const SkRect& bounds) const {
    SkPath path;
    float width = bounds.width();
    float height = bounds.height();
    
    switch (type) {
        case ClipPathType::NONE:
            // 返回覆盖整个区域的矩形
            path.addRect(bounds);
            break;
            
        case ClipPathType::INSET: {
            float top = inset.top.ToPx(height);
            float right_val = inset.right.ToPx(width);
            float bottom_val = inset.bottom.ToPx(height);
            float left_val = inset.left.ToPx(width);
            
            SkRect insetRect = SkRect::MakeLTRB(
                bounds.left() + left_val,
                bounds.top() + top,
                bounds.right() - right_val,
                bounds.bottom() - bottom_val
            );
            
            if (inset.has_round) {
                // 带圆角的矩形
                float tl = inset.border_radius.top_left.ToPx(std::min(width, height));
                float tr = inset.border_radius.top_right.ToPx(std::min(width, height));
                float br = inset.border_radius.bottom_right.ToPx(std::min(width, height));
                float bl = inset.border_radius.bottom_left.ToPx(std::min(width, height));
                
                SkVector radii[4] = {
                    {tl, tl}, {tr, tr}, {br, br}, {bl, bl}
                };
                
                SkRRect rrect;
                rrect.setRectRadii(insetRect, radii);
                path.addRRect(rrect);
            } else {
                path.addRect(insetRect);
            }
            break;
        }
        
        case ClipPathType::CIRCLE: {
            // 计算圆心位置
            float cx = bounds.left() + circle.position.x.ToPx(width);
            float cy = bounds.top() + circle.position.y.ToPx(height);
            
            // 计算半径
            float radius;
            switch (circle.radius_type) {
                case RadiusType::CLOSEST_SIDE:
                    radius = CalculateClosestSide(cx - bounds.left(), cy - bounds.top(), width, height);
                    break;
                case RadiusType::FARTHEST_SIDE:
                    radius = CalculateFarthestSide(cx - bounds.left(), cy - bounds.top(), width, height);
                    break;
                case RadiusType::LENGTH:
                default:
                    // 百分比相对于 sqrt(width^2 + height^2) / sqrt(2)
                    if (circle.radius.unit == CSSUnit::PERCENT) {
                        float ref = std::sqrt(width * width + height * height) / std::sqrt(2.0f);
                        radius = circle.radius.value * ref / 100.0f;
                    } else {
                        radius = circle.radius.ToPx(std::min(width, height));
                    }
                    break;
            }
            
            path.addCircle(cx, cy, radius);
            break;
        }
        
        case ClipPathType::ELLIPSE: {
            // 计算圆心位置
            float cx = bounds.left() + ellipse.position.x.ToPx(width);
            float cy = bounds.top() + ellipse.position.y.ToPx(height);
            
            // 计算半径
            float rx, ry;
            
            if (ellipse.radius_type_x == RadiusType::CLOSEST_SIDE) {
                rx = std::min(cx - bounds.left(), bounds.right() - cx);
            } else if (ellipse.radius_type_x == RadiusType::FARTHEST_SIDE) {
                rx = std::max(cx - bounds.left(), bounds.right() - cx);
            } else {
                rx = ellipse.radius_x.ToPx(width);
            }
            
            if (ellipse.radius_type_y == RadiusType::CLOSEST_SIDE) {
                ry = std::min(cy - bounds.top(), bounds.bottom() - cy);
            } else if (ellipse.radius_type_y == RadiusType::FARTHEST_SIDE) {
                ry = std::max(cy - bounds.top(), bounds.bottom() - cy);
            } else {
                ry = ellipse.radius_y.ToPx(height);
            }
            
            SkRect ellipseRect = SkRect::MakeXYWH(cx - rx, cy - ry, rx * 2, ry * 2);
            path.addOval(ellipseRect);
            break;
        }
        
        case ClipPathType::POLYGON: {
            if (polygon.points.empty()) {
                path.addRect(bounds);
                break;
            }
            
            // 设置填充规则
            if (polygon.fill_rule == ClipFillRule::EVENODD) {
                path.setFillType(SkPathFillType::kEvenOdd);
            } else {
                path.setFillType(SkPathFillType::kWinding);
            }
            
            // 添加多边形顶点
            bool first = true;
            for (const auto& point : polygon.points) {
                float x = bounds.left() + point.first.ToPx(width);
                float y = bounds.top() + point.second.ToPx(height);
                
                if (first) {
                    path.moveTo(x, y);
                    first = false;
                } else {
                    path.lineTo(x, y);
                }
            }
            path.close();
            break;
        }
    }
    
    return path;
}

// ============================================================================
// 解析函数实现
// ============================================================================

CSSClipPath ParseClipPath(const std::string& value) {
    CSSClipPath result;
    std::string v = Trim(value);
    
    if (v.empty() || v == "none") {
        result.type = ClipPathType::NONE;
        return result;
    }
    
    // 检查函数类型
    if (v.find("inset(") == 0) {
        result.type = ClipPathType::INSET;
        result.inset = ParseClipInset(ExtractFunctionParams(v, "inset"));
    } else if (v.find("circle(") == 0) {
        result.type = ClipPathType::CIRCLE;
        result.circle = ParseClipCircle(ExtractFunctionParams(v, "circle"));
    } else if (v.find("ellipse(") == 0) {
        result.type = ClipPathType::ELLIPSE;
        result.ellipse = ParseClipEllipse(ExtractFunctionParams(v, "ellipse"));
    } else if (v.find("polygon(") == 0) {
        result.type = ClipPathType::POLYGON;
        result.polygon = ParseClipPolygon(ExtractFunctionParams(v, "polygon"));
    }
    
    return result;
}

ClipInset ParseClipInset(const std::string& params) {
    ClipInset result;
    std::string p = Trim(params);
    
    // 检查是否有 round 关键字
    size_t roundPos = p.find(" round ");
    std::string insetPart = p;
    std::string roundPart;
    
    if (roundPos != std::string::npos) {
        insetPart = p.substr(0, roundPos);
        roundPart = p.substr(roundPos + 7);  // 跳过 " round "
        result.has_round = true;
    }
    
    // 解析 inset 值（1-4个值）
    auto values = SplitBySpace(insetPart);
    
    if (values.size() >= 1) {
        result.top = ParseLengthValue(values[0]);
        result.right = result.top;
        result.bottom = result.top;
        result.left = result.top;
    }
    if (values.size() >= 2) {
        result.right = ParseLengthValue(values[1]);
        result.left = result.right;
    }
    if (values.size() >= 3) {
        result.bottom = ParseLengthValue(values[2]);
    }
    if (values.size() >= 4) {
        result.left = ParseLengthValue(values[3]);
    }
    
    // 解析 round 值
    if (result.has_round && !roundPart.empty()) {
        auto roundValues = SplitBySpace(roundPart);
        if (roundValues.size() >= 1) {
            CSSLength r = ParseLengthValue(roundValues[0]);
            result.border_radius.top_left = r;
            result.border_radius.top_right = r;
            result.border_radius.bottom_right = r;
            result.border_radius.bottom_left = r;
        }
        if (roundValues.size() >= 2) {
            CSSLength r = ParseLengthValue(roundValues[1]);
            result.border_radius.top_right = r;
            result.border_radius.bottom_left = r;
        }
        if (roundValues.size() >= 3) {
            result.border_radius.bottom_right = ParseLengthValue(roundValues[2]);
        }
        if (roundValues.size() >= 4) {
            result.border_radius.bottom_left = ParseLengthValue(roundValues[3]);
        }
    }
    
    return result;
}

ClipCircle ParseClipCircle(const std::string& params) {
    ClipCircle result;
    std::string p = Trim(params);
    
    if (p.empty()) {
        // 默认值：closest-side at center
        result.radius_type = RadiusType::CLOSEST_SIDE;
        return result;
    }
    
    // 检查是否有 at 关键字
    size_t atPos = p.find(" at ");
    std::string radiusPart = p;
    std::string positionPart;
    
    if (atPos != std::string::npos) {
        radiusPart = Trim(p.substr(0, atPos));
        positionPart = Trim(p.substr(atPos + 4));
    }
    
    // 解析半径
    if (!radiusPart.empty()) {
        std::string r = Trim(radiusPart);
        std::transform(r.begin(), r.end(), r.begin(), ::tolower);
        
        if (r == "closest-side") {
            result.radius_type = RadiusType::CLOSEST_SIDE;
        } else if (r == "farthest-side") {
            result.radius_type = RadiusType::FARTHEST_SIDE;
        } else {
            result.radius_type = RadiusType::LENGTH;
            result.radius = ParseLengthValue(radiusPart);
        }
    }
    
    // 解析位置
    if (!positionPart.empty()) {
        auto posValues = SplitBySpace(positionPart);
        if (posValues.size() >= 1) {
            result.position.x = ParsePositionKeyword(posValues[0], true);
        }
        if (posValues.size() >= 2) {
            result.position.y = ParsePositionKeyword(posValues[1], false);
        } else if (posValues.size() == 1) {
            // 单个值时，y 也使用相同的值（如果是 center）
            std::string v = posValues[0];
            std::transform(v.begin(), v.end(), v.begin(), ::tolower);
            if (v == "center") {
                result.position.y = CSSLength(50.0f, CSSUnit::PERCENT);
            }
        }
    }
    
    return result;
}

ClipEllipse ParseClipEllipse(const std::string& params) {
    ClipEllipse result;
    std::string p = Trim(params);
    
    if (p.empty()) {
        // 默认值：closest-side closest-side at center
        result.radius_type_x = RadiusType::CLOSEST_SIDE;
        result.radius_type_y = RadiusType::CLOSEST_SIDE;
        return result;
    }
    
    // 检查是否有 at 关键字
    size_t atPos = p.find(" at ");
    std::string radiusPart = p;
    std::string positionPart;
    
    if (atPos != std::string::npos) {
        radiusPart = Trim(p.substr(0, atPos));
        positionPart = Trim(p.substr(atPos + 4));
    }
    
    // 解析半径（两个值）
    if (!radiusPart.empty()) {
        auto radii = SplitBySpace(radiusPart);
        
        if (radii.size() >= 1) {
            std::string rx = Trim(radii[0]);
            std::transform(rx.begin(), rx.end(), rx.begin(), ::tolower);
            
            if (rx == "closest-side") {
                result.radius_type_x = RadiusType::CLOSEST_SIDE;
            } else if (rx == "farthest-side") {
                result.radius_type_x = RadiusType::FARTHEST_SIDE;
            } else {
                result.radius_type_x = RadiusType::LENGTH;
                result.radius_x = ParseLengthValue(radii[0]);
            }
        }
        
        if (radii.size() >= 2) {
            std::string ry = Trim(radii[1]);
            std::transform(ry.begin(), ry.end(), ry.begin(), ::tolower);
            
            if (ry == "closest-side") {
                result.radius_type_y = RadiusType::CLOSEST_SIDE;
            } else if (ry == "farthest-side") {
                result.radius_type_y = RadiusType::FARTHEST_SIDE;
            } else {
                result.radius_type_y = RadiusType::LENGTH;
                result.radius_y = ParseLengthValue(radii[1]);
            }
        }
    }
    
    // 解析位置
    if (!positionPart.empty()) {
        auto posValues = SplitBySpace(positionPart);
        if (posValues.size() >= 1) {
            result.position.x = ParsePositionKeyword(posValues[0], true);
        }
        if (posValues.size() >= 2) {
            result.position.y = ParsePositionKeyword(posValues[1], false);
        }
    }
    
    return result;
}

ClipPolygon ParseClipPolygon(const std::string& params) {
    ClipPolygon result;
    std::string p = Trim(params);
    
    if (p.empty()) {
        return result;
    }
    
    // 检查填充规则
    std::string pointsPart = p;
    if (p.find("evenodd") == 0) {
        result.fill_rule = ClipFillRule::EVENODD;
        size_t commaPos = p.find(',');
        if (commaPos != std::string::npos) {
            pointsPart = Trim(p.substr(commaPos + 1));
        }
    } else if (p.find("nonzero") == 0) {
        result.fill_rule = ClipFillRule::NONZERO;
        size_t commaPos = p.find(',');
        if (commaPos != std::string::npos) {
            pointsPart = Trim(p.substr(commaPos + 1));
        }
    }
    
    // 按逗号分割点
    auto pointStrs = Split(pointsPart, ',');
    
    for (const auto& pointStr : pointStrs) {
        auto coords = SplitBySpace(pointStr);
        if (coords.size() >= 2) {
            CSSLength x = ParseLengthValue(coords[0]);
            CSSLength y = ParseLengthValue(coords[1]);
            result.points.emplace_back(x, y);
        }
    }
    
    return result;
}

} // namespace lightui
