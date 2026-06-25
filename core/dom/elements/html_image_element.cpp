/**
 * @file html_image_element.cpp
 * @brief HTML Image元素实现
 */

#include "html_image_element.h"
#include "../document.h"
#include "../event.h"
#include "../render/image/image_loader.h"
#include <algorithm>
#include <cctype>
#include <iostream>

namespace mblink {

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
    , sk_image_(nullptr)
    , load_state_(ImageLoadState::IDLE)
    , error_message_("") {
}

// ========== IDL属性实现 ==========

void HTMLImageElement::SetSrc(const std::string& src) {
    if (src_ == src) {
        return;  // 相同的 src，不重复加载
    }
    
    src_ = src;
    SetAttribute("src", src);
    
    // 重置状态
    complete_ = false;
    natural_width_ = 0;
    natural_height_ = 0;
    sk_image_ = nullptr;
    load_state_ = ImageLoadState::IDLE;
    error_message_ = "";
    
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
        if (src_ != value) {
            src_ = value;
            // 重置状态
            complete_ = false;
            natural_width_ = 0;
            natural_height_ = 0;
            sk_image_ = nullptr;
            load_state_ = ImageLoadState::IDLE;
            error_message_ = "";
            
            if (!value.empty()) {
                LoadImage();
            }
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
        sk_image_ = nullptr;
        load_state_ = ImageLoadState::IDLE;
        error_message_ = "";
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
    
    
    // 设置加载状态
    load_state_ = ImageLoadState::LOADING;
    
    // 触发loadstart事件
    TriggerLoadStartEvent();
    
    // ImageLoader 会自动处理相对路径
    std::string url = src_;
    
    // 使用同步加载，确保事件在主线程中触发
    // 这对于本地文件来说是安全的，因为加载很快
    ImageLoadResult result = ImageLoader::LoadFromUrlWithResult(url);
    
    
    if (result.success && result.image) {
        OnImageLoaded(result.image, "");
    } else {
        OnImageLoaded(nullptr, result.error);
    }
}

bool HTMLImageElement::LoadImageSync() {
    if (src_.empty()) {
        return false;
    }
    
    // 设置加载状态
    load_state_ = ImageLoadState::LOADING;
    
    // 触发loadstart事件
    TriggerLoadStartEvent();
    
    // ImageLoader 会自动处理相对路径
    ImageLoadResult result = ImageLoader::LoadFromUrlWithResult(src_);
    
    if (result.success && result.image) {
        OnImageLoaded(result.image, "");
        return true;
    } else {
        OnImageLoaded(nullptr, result.error);
        return false;
    }
}

void HTMLImageElement::OnImageLoaded(sk_sp<SkImage> image, const std::string& error) {
    if (image) {
        // 加载成功
        sk_image_ = image;
        natural_width_ = image->width();
        natural_height_ = image->height();
        complete_ = true;
        load_state_ = ImageLoadState::COMPLETE;
        error_message_ = "";
        
        // 如果没有设置显示尺寸，使用原始尺寸
        if (width_ == 0) {
            width_ = natural_width_;
        }
        if (height_ == 0) {
            height_ = natural_height_;
        }
        
        // 触发load事件
        TriggerLoadEvent();
    } else {
        // 加载失败
        sk_image_ = nullptr;
        natural_width_ = 0;
        natural_height_ = 0;
        complete_ = true;  // complete 在加载失败时也为 true
        load_state_ = ImageLoadState::ERROR;
        error_message_ = error.empty() ? "Failed to load image" : error;
        
        // 触发error事件
        TriggerErrorEvent();
    }
    
    // 触发loadend事件
    TriggerLoadEndEvent();
    
    // 标记需要重绘
    MarkDirty();
}

void HTMLImageElement::SetImageData(void* data, unsigned long width, unsigned long height) {
    // 旧接口兼容 - 不推荐使用
    natural_width_ = width;
    natural_height_ = height;
    complete_ = true;
    load_state_ = ImageLoadState::COMPLETE;
    
    // 如果没有设置显示尺寸，使用原始尺寸
    if (width_ == 0) {
        width_ = width;
    }
    if (height_ == 0) {
        height_ = height;
    }
}

void HTMLImageElement::SetSkImage(sk_sp<SkImage> image) {
    if (image) {
        sk_image_ = image;
        natural_width_ = image->width();
        natural_height_ = image->height();
        complete_ = true;
        load_state_ = ImageLoadState::COMPLETE;
        error_message_ = "";
        
        // 如果没有设置显示尺寸，使用原始尺寸
        if (width_ == 0) {
            width_ = natural_width_;
        }
        if (height_ == 0) {
            height_ = natural_height_;
        }
        
        // 标记需要重绘
        MarkDirty();
    }
}

// ========== 事件触发 ==========

void HTMLImageElement::TriggerLoadStartEvent() {
    auto event = std::make_shared<Event>("loadstart");
    DispatchEvent(event);
}

void HTMLImageElement::TriggerLoadEvent() {
    
    auto event = std::make_shared<Event>("load");
    bool result = DispatchEvent(event);
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

} // namespace mblink

