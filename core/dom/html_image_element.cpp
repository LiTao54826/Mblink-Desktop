/**
 * @file html_image_element.cpp
 * @brief HTML Image元素实现
 */

#include "html_image_element.h"
#include "document.h"
#include "event.h"
#include <algorithm>
#include <cctype>

namespace lightui {

HTMLImageElement::HTMLImageElement()
    : Element("img")
    , src_("")
    , alt_("")
    , cross_origin_("")
    , width_(0)
    , height_(0)
    , natural_width_(0)
    , natural_height_(0)
    , complete_(false)
    , image_data_(nullptr) {
}

// ========== IDL属性实现 ==========

void HTMLImageElement::SetSrc(const std::string& src) {
    src_ = src;
    SetAttribute("src", src);
    
    // 设置src后自动触发加载
    if (!src.empty()) {
        LoadImage();
    }
}

void HTMLImageElement::SetAlt(const std::string& alt) {
    alt_ = alt;
    SetAttribute("alt", alt);
}

void HTMLImageElement::SetWidth(unsigned long width) {
    width_ = width;
    SetAttribute("width", std::to_string(width));
}

void HTMLImageElement::SetHeight(unsigned long height) {
    height_ = height;
    SetAttribute("height", std::to_string(height));
}

void HTMLImageElement::SetCrossOrigin(const std::string& cross_origin) {
    cross_origin_ = cross_origin;
    SetAttribute("crossorigin", cross_origin);
}

// ========== 重写方法 ==========

void HTMLImageElement::SetAttribute(const std::string& name, const std::string& value) {
    // 调用基类方法
    Element::SetAttribute(name, value);
    
    // 处理特殊属性
    if (name == "src") {
        src_ = value;
        if (!value.empty()) {
            LoadImage();
        }
    } else if (name == "alt") {
        alt_ = value;
    } else if (name == "width") {
        width_ = ParseDimension(value);
    } else if (name == "height") {
        height_ = ParseDimension(value);
    } else if (name == "crossorigin") {
        cross_origin_ = value;
    }
}

void HTMLImageElement::RemoveAttribute(const std::string& name) {
    // 调用基类方法
    Element::RemoveAttribute(name);
    
    // 处理特殊属性
    if (name == "src") {
        src_ = "";
        complete_ = false;
        natural_width_ = 0;
        natural_height_ = 0;
        image_data_ = nullptr;
    } else if (name == "alt") {
        alt_ = "";
    } else if (name == "width") {
        width_ = 0;
    } else if (name == "height") {
        height_ = 0;
    } else if (name == "crossorigin") {
        cross_origin_ = "";
    }
}

// ========== 图片加载 ==========

void HTMLImageElement::LoadImage() {
    if (src_.empty()) {
        return;
    }
    
    // 触发loadstart事件
    TriggerLoadStartEvent();
    
    // TODO: 实际的图片加载逻辑
    // 这里需要集成图片加载库（如stb_image）或使用Skia的图片加载功能
    // 目前只是模拟加载流程
    
    // 模拟加载成功
    // 在实际实现中，这里应该异步加载图片
    // 加载成功后设置natural_width_和natural_height_
    
    // 示例：假设加载成功
    // natural_width_ = 实际图片宽度;
    // natural_height_ = 实际图片高度;
    // complete_ = true;
    // TriggerLoadEvent();
    // TriggerLoadEndEvent();
    
    // 目前只标记为完成（用于测试）
    complete_ = true;
    TriggerLoadEvent();
    TriggerLoadEndEvent();
}

void HTMLImageElement::SetImageData(void* data, unsigned long width, unsigned long height) {
    image_data_ = data;
    natural_width_ = width;
    natural_height_ = height;
    complete_ = true;
    
    // 如果没有设置显示尺寸，使用原始尺寸
    if (width_ == 0) {
        width_ = width;
    }
    if (height_ == 0) {
        height_ = height;
    }
}

// ========== 事件触发 ==========

void HTMLImageElement::TriggerLoadStartEvent() {
    auto event = std::make_shared<Event>("loadstart");
    DispatchEvent(event);
}

void HTMLImageElement::TriggerLoadEvent() {
    auto event = std::make_shared<Event>("load");
    DispatchEvent(event);
}

void HTMLImageElement::TriggerErrorEvent() {
    auto event = std::make_shared<Event>("error");
    DispatchEvent(event);
}

void HTMLImageElement::TriggerLoadEndEvent() {
    auto event = std::make_shared<Event>("loadend");
    DispatchEvent(event);
}

// ========== 辅助方法 ==========

unsigned long HTMLImageElement::ParseDimension(const std::string& value) const {
    if (value.empty()) {
        return 0;
    }
    
    try {
        // 解析数字部分（忽略单位）
        size_t pos = 0;
        unsigned long result = std::stoul(value, &pos);
        return result;
    } catch (...) {
        return 0;
    }
}

} // namespace lightui

