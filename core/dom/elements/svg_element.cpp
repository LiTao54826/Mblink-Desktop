/**
 * @file svg_element.cpp
 * @brief SVG元素实现
 */

#include "svg_element.h"
#include <sstream>
#include <cstdlib>

namespace mblink {

// ========== SVGElement 基类实现 ==========

SVGElement::SVGElement(const std::string& tag_name)
    : Element(tag_name) {
}

std::string SVGElement::GetFill() const {
    std::string value = GetAttribute("fill");
    return value.empty() ? "black" : value;
}

void SVGElement::SetFill(const std::string& value) {
    SetAttribute("fill", value);
}

std::string SVGElement::GetStroke() const {
    return GetAttribute("stroke");
}

void SVGElement::SetStroke(const std::string& value) {
    SetAttribute("stroke", value);
}

float SVGElement::GetStrokeWidth() const {
    std::string value = GetAttribute("stroke-width");
    if (value.empty()) return 1.0f;
    try {
        return std::stof(value);
    } catch (...) {
        return 1.0f;
    }
}

void SVGElement::SetStrokeWidth(float value) {
    SetAttribute("stroke-width", std::to_string(value));
}

float SVGElement::GetOpacity() const {
    std::string value = GetAttribute("opacity");
    if (value.empty()) return 1.0f;
    try {
        return std::stof(value);
    } catch (...) {
        return 1.0f;
    }
}

void SVGElement::SetOpacity(float value) {
    SetAttribute("opacity", std::to_string(value));
}

std::string SVGElement::GetTransform() const {
    return GetAttribute("transform");
}

void SVGElement::SetTransform(const std::string& value) {
    SetAttribute("transform", value);
}

float SVGElement::GetFillOpacity() const {
    std::string value = GetAttribute("fill-opacity");
    if (value.empty()) return 1.0f;
    try {
        return std::stof(value);
    } catch (...) {
        return 1.0f;
    }
}

void SVGElement::SetFillOpacity(float value) {
    SetAttribute("fill-opacity", std::to_string(value));
}

float SVGElement::GetStrokeOpacity() const {
    std::string value = GetAttribute("stroke-opacity");
    if (value.empty()) return 1.0f;
    try {
        return std::stof(value);
    } catch (...) {
        return 1.0f;
    }
}

void SVGElement::SetStrokeOpacity(float value) {
    SetAttribute("stroke-opacity", std::to_string(value));
}

// ========== SVGSVGElement 实现 ==========

SVGSVGElement::SVGSVGElement()
    : SVGElement("svg") {
}

std::string SVGSVGElement::GetWidth() const {
    return GetAttribute("width");
}

void SVGSVGElement::SetWidth(const std::string& value) {
    SetAttribute("width", value);
}

std::string SVGSVGElement::GetHeight() const {
    return GetAttribute("height");
}

void SVGSVGElement::SetHeight(const std::string& value) {
    SetAttribute("height", value);
}

std::string SVGSVGElement::GetViewBox() const {
    return GetAttribute("viewBox");
}

void SVGSVGElement::SetViewBox(const std::string& value) {
    SetAttribute("viewBox", value);
}

bool SVGSVGElement::ParseViewBox(float& min_x, float& min_y, float& width, float& height) const {
    std::string viewbox = GetViewBox();
    if (viewbox.empty()) return false;

    // viewBox格式：minX minY width height（可用空格或逗号分隔）
    std::istringstream iss(viewbox);
    std::vector<float> values;
    std::string token;
    
    while (std::getline(iss, token, ' ') || std::getline(iss, token, ',')) {
        if (token.empty()) continue;
        try {
            values.push_back(std::stof(token));
        } catch (...) {
            return false;
        }
    }

    if (values.size() >= 4) {
        min_x = values[0];
        min_y = values[1];
        width = values[2];
        height = values[3];
        return true;
    }
    return false;
}

// ========== SVGPathElement 实现 ==========

SVGPathElement::SVGPathElement()
    : SVGElement("path") {
}

std::string SVGPathElement::GetD() const {
    return GetAttribute("d");
}

void SVGPathElement::SetD(const std::string& value) {
    SetAttribute("d", value);
}

float SVGPathElement::GetTotalLength() const {
    // TODO: 实现路径长度计算
    return 0.0f;
}

// ========== SVGGElement 实现 ==========

SVGGElement::SVGGElement()
    : SVGElement("g") {
}

// ========== SVGCircleElement 实现 ==========

SVGCircleElement::SVGCircleElement()
    : SVGElement("circle") {
}

float SVGCircleElement::GetCx() const {
    std::string value = GetAttribute("cx");
    if (value.empty()) return 0.0f;
    try { return std::stof(value); } catch (...) { return 0.0f; }
}

void SVGCircleElement::SetCx(float value) {
    SetAttribute("cx", std::to_string(value));
}

float SVGCircleElement::GetCy() const {
    std::string value = GetAttribute("cy");
    if (value.empty()) return 0.0f;
    try { return std::stof(value); } catch (...) { return 0.0f; }
}

void SVGCircleElement::SetCy(float value) {
    SetAttribute("cy", std::to_string(value));
}

float SVGCircleElement::GetR() const {
    std::string value = GetAttribute("r");
    if (value.empty()) return 0.0f;
    try { return std::stof(value); } catch (...) { return 0.0f; }
}

void SVGCircleElement::SetR(float value) {
    SetAttribute("r", std::to_string(value));
}

// ========== SVGRectElement 实现 ==========

SVGRectElement::SVGRectElement()
    : SVGElement("rect") {
}

float SVGRectElement::GetX() const {
    std::string value = GetAttribute("x");
    if (value.empty()) return 0.0f;
    try { return std::stof(value); } catch (...) { return 0.0f; }
}

void SVGRectElement::SetX(float value) {
    SetAttribute("x", std::to_string(value));
}

float SVGRectElement::GetY() const {
    std::string value = GetAttribute("y");
    if (value.empty()) return 0.0f;
    try { return std::stof(value); } catch (...) { return 0.0f; }
}

void SVGRectElement::SetY(float value) {
    SetAttribute("y", std::to_string(value));
}

float SVGRectElement::GetWidth() const {
    std::string value = GetAttribute("width");
    if (value.empty()) return 0.0f;
    try { return std::stof(value); } catch (...) { return 0.0f; }
}

void SVGRectElement::SetWidth(float value) {
    SetAttribute("width", std::to_string(value));
}

float SVGRectElement::GetHeight() const {
    std::string value = GetAttribute("height");
    if (value.empty()) return 0.0f;
    try { return std::stof(value); } catch (...) { return 0.0f; }
}

void SVGRectElement::SetHeight(float value) {
    SetAttribute("height", std::to_string(value));
}

float SVGRectElement::GetRx() const {
    std::string value = GetAttribute("rx");
    if (value.empty()) return 0.0f;
    try { return std::stof(value); } catch (...) { return 0.0f; }
}

void SVGRectElement::SetRx(float value) {
    SetAttribute("rx", std::to_string(value));
}

float SVGRectElement::GetRy() const {
    std::string value = GetAttribute("ry");
    if (value.empty()) return 0.0f;
    try { return std::stof(value); } catch (...) { return 0.0f; }
}

void SVGRectElement::SetRy(float value) {
    SetAttribute("ry", std::to_string(value));
}

// ========== SVGEllipseElement 实现 ==========

SVGEllipseElement::SVGEllipseElement()
    : SVGElement("ellipse") {
}

float SVGEllipseElement::GetCx() const {
    std::string value = GetAttribute("cx");
    if (value.empty()) return 0.0f;
    try { return std::stof(value); } catch (...) { return 0.0f; }
}

void SVGEllipseElement::SetCx(float value) {
    SetAttribute("cx", std::to_string(value));
}

float SVGEllipseElement::GetCy() const {
    std::string value = GetAttribute("cy");
    if (value.empty()) return 0.0f;
    try { return std::stof(value); } catch (...) { return 0.0f; }
}

void SVGEllipseElement::SetCy(float value) {
    SetAttribute("cy", std::to_string(value));
}

float SVGEllipseElement::GetRx() const {
    std::string value = GetAttribute("rx");
    if (value.empty()) return 0.0f;
    try { return std::stof(value); } catch (...) { return 0.0f; }
}

void SVGEllipseElement::SetRx(float value) {
    SetAttribute("rx", std::to_string(value));
}

float SVGEllipseElement::GetRy() const {
    std::string value = GetAttribute("ry");
    if (value.empty()) return 0.0f;
    try { return std::stof(value); } catch (...) { return 0.0f; }
}

void SVGEllipseElement::SetRy(float value) {
    SetAttribute("ry", std::to_string(value));
}

// ========== SVGLineElement 实现 ==========

SVGLineElement::SVGLineElement()
    : SVGElement("line") {
}

float SVGLineElement::GetX1() const {
    std::string value = GetAttribute("x1");
    if (value.empty()) return 0.0f;
    try { return std::stof(value); } catch (...) { return 0.0f; }
}

void SVGLineElement::SetX1(float value) {
    SetAttribute("x1", std::to_string(value));
}

float SVGLineElement::GetY1() const {
    std::string value = GetAttribute("y1");
    if (value.empty()) return 0.0f;
    try { return std::stof(value); } catch (...) { return 0.0f; }
}

void SVGLineElement::SetY1(float value) {
    SetAttribute("y1", std::to_string(value));
}

float SVGLineElement::GetX2() const {
    std::string value = GetAttribute("x2");
    if (value.empty()) return 0.0f;
    try { return std::stof(value); } catch (...) { return 0.0f; }
}

void SVGLineElement::SetX2(float value) {
    SetAttribute("x2", std::to_string(value));
}

float SVGLineElement::GetY2() const {
    std::string value = GetAttribute("y2");
    if (value.empty()) return 0.0f;
    try { return std::stof(value); } catch (...) { return 0.0f; }
}

void SVGLineElement::SetY2(float value) {
    SetAttribute("y2", std::to_string(value));
}

// ========== SVGPolylineElement 实现 ==========

SVGPolylineElement::SVGPolylineElement()
    : SVGElement("polyline") {
}

std::string SVGPolylineElement::GetPoints() const {
    return GetAttribute("points");
}

void SVGPolylineElement::SetPoints(const std::string& value) {
    SetAttribute("points", value);
}

// ========== SVGPolygonElement 实现 ==========

SVGPolygonElement::SVGPolygonElement()
    : SVGElement("polygon") {
}

std::string SVGPolygonElement::GetPoints() const {
    return GetAttribute("points");
}

void SVGPolygonElement::SetPoints(const std::string& value) {
    SetAttribute("points", value);
}

// ========== SVGTextElement 实现 ==========

SVGTextElement::SVGTextElement()
    : SVGElement("text") {
}

float SVGTextElement::GetX() const {
    std::string value = GetAttribute("x");
    if (value.empty()) return 0.0f;
    try { return std::stof(value); } catch (...) { return 0.0f; }
}

void SVGTextElement::SetX(float value) {
    SetAttribute("x", std::to_string(value));
}

float SVGTextElement::GetY() const {
    std::string value = GetAttribute("y");
    if (value.empty()) return 0.0f;
    try { return std::stof(value); } catch (...) { return 0.0f; }
}

void SVGTextElement::SetY(float value) {
    SetAttribute("y", std::to_string(value));
}

std::string SVGTextElement::GetTextAnchor() const {
    return GetAttribute("text-anchor");
}

float SVGTextElement::GetFontSize() const {
    std::string value = GetAttribute("font-size");
    if (value.empty()) return 16.0f;  // 默认字体大小
    try { return std::stof(value); } catch (...) { return 16.0f; }
}

} // namespace mblink

