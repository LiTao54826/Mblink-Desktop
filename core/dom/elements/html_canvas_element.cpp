/**
 * @file html_canvas_element.cpp
 * @brief HTML Canvas元素类实现
 */

#include "html_canvas_element.h"
#include "core/render/canvas/canvas_rendering_context_2d.h"
#include <stdexcept>
#include <sstream>

namespace mbink {

// ========== 构造函数 ==========

HTMLCanvasElement::HTMLCanvasElement()
    : Element("canvas")
    , width_(300)   // HTML标准默认宽度
    , height_(150)  // HTML标准默认高度
    , context_2d_(nullptr) {
}

// 析构函数需要在cpp文件中定义，因为这里CanvasRenderingContext2D是完整类型
HTMLCanvasElement::~HTMLCanvasElement() = default;

// ========== IDL属性实现 ==========

void HTMLCanvasElement::SetWidth(unsigned long width) {
    if (width_ != width) {
        width_ = width;
        Element::SetAttribute("width", std::to_string(width));
        ResizeCanvas();
    }
}

void HTMLCanvasElement::SetHeight(unsigned long height) {
    if (height_ != height) {
        height_ = height;
        Element::SetAttribute("height", std::to_string(height));
        ResizeCanvas();
    }
}

// ========== IDL方法实现 ==========

void* HTMLCanvasElement::GetContext(const std::string& context_id) {
    if (context_id == "2d") {
        // 延迟创建2D上下文
        if (!context_2d_) {
            context_2d_ = std::make_unique<CanvasRenderingContext2D>(width_, height_);
        }
        return context_2d_.get();
    }
    // 其他上下文类型（webgl, webgl2等）暂不支持
    return nullptr;
}

std::string HTMLCanvasElement::ToDataURL(const std::string& type, double quality) {
    if (!context_2d_) {
        // 空画布返回透明图片的Data URL
        return "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mNk+M9QDwADhgGAWjR9awAAAABJRU5ErkJggg==";
    }
    
    return context_2d_->ToDataURL(type, quality);
}

// ========== 重写方法 ==========

void HTMLCanvasElement::SetAttribute(const std::string& name, const std::string& value) {
    if (name == "width") {
        unsigned long w = ParseDimension(value, 300);
        SetWidth(w);
    } else if (name == "height") {
        unsigned long h = ParseDimension(value, 150);
        SetHeight(h);
    } else {
        Element::SetAttribute(name, value);
    }
}

void HTMLCanvasElement::RemoveAttribute(const std::string& name) {
    if (name == "width") {
        SetWidth(300);  // 恢复默认值
    } else if (name == "height") {
        SetHeight(150); // 恢复默认值
    } else {
        Element::RemoveAttribute(name);
    }
}

// ========== 私有辅助方法 ==========

unsigned long HTMLCanvasElement::ParseDimension(const std::string& value, unsigned long default_value) const {
    if (value.empty()) {
        return default_value;
    }

    try {
        // 尝试解析为无符号整数
        size_t pos;
        unsigned long result = std::stoul(value, &pos);
        
        // 如果整个字符串都是数字，返回结果
        if (pos == value.length()) {
            return result > 0 ? result : default_value;
        }
    } catch (...) {
        // 解析失败，返回默认值
    }

    return default_value;
}

void HTMLCanvasElement::ResizeCanvas() {
    // 如果2D上下文已创建，需要调整大小
    if (context_2d_) {
        context_2d_->Resize(width_, height_);
    }
}

} // namespace mbink
