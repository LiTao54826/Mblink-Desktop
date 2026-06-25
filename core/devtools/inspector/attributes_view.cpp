/**
 * @file attributes_view.cpp
 * @brief 属性视图组件实现
 */

#include "attributes_view.h"
#include "core/render/text/font_manager.h"

#include "include/core/SkCanvas.h"
#include "include/core/SkPaint.h"
#include "include/core/SkRect.h"
#include "include/core/SkFont.h"

namespace mblink {

namespace {
    const float ROW_HEIGHT = 20.0f;
    const float PADDING = 8.0f;
}

AttributesView::AttributesView() = default;

AttributesView::~AttributesView() = default;

void AttributesView::SetElement(std::shared_ptr<Element> element) {
    element_ = element;
}

std::vector<AttributeInfo> AttributesView::GetAttributes() const {
    std::vector<AttributeInfo> attributes;

    auto element = element_.lock();
    if (!element) return attributes;

    // 获取所有属性
    // 注意：这里需要 Element 类提供获取所有属性的接口
    // 暂时使用常见属性列表
    
    // id 属性
    std::string id = element->GetAttribute("id");
    if (!id.empty()) {
        AttributeInfo info;
        info.name = "id";
        info.value = id;
        info.is_id = true;
        attributes.push_back(info);
    }

    // class 属性
    std::string cls = element->GetAttribute("class");
    if (!cls.empty()) {
        AttributeInfo info;
        info.name = "class";
        info.value = cls;
        info.is_class = true;
        attributes.push_back(info);
    }

    // style 属性
    std::string style = element->GetAttribute("style");
    if (!style.empty()) {
        AttributeInfo info;
        info.name = "style";
        info.value = style;
        attributes.push_back(info);
    }

    // 常见的 data-* 属性
    std::vector<std::string> data_attrs = {
        "data-id", "data-value", "data-type", "data-name", "data-index",
        "data-src", "data-target", "data-toggle", "data-action"
    };
    
    for (const auto& attr : data_attrs) {
        std::string value = element->GetAttribute(attr);
        if (!value.empty()) {
            AttributeInfo info;
            info.name = attr;
            info.value = value;
            info.is_data = true;
            attributes.push_back(info);
        }
    }

    // 其他常见属性
    std::vector<std::string> common_attrs = {
        "href", "src", "alt", "title", "name", "type", "value",
        "placeholder", "disabled", "readonly", "checked", "selected",
        "aria-label", "aria-hidden", "role", "tabindex"
    };
    
    for (const auto& attr : common_attrs) {
        std::string value = element->GetAttribute(attr);
        if (!value.empty()) {
            AttributeInfo info;
            info.name = attr;
            info.value = value;
            attributes.push_back(info);
        }
    }

    return attributes;
}

void AttributesView::Render(SkCanvas* canvas, float x, float y, float width, float height) {
    auto attributes = GetAttributes();

    if (attributes.empty()) {
        RenderEmptyState(canvas, x, y, width, height);
    } else {
        RenderAttributeList(canvas, x, y, width, height, attributes);
    }
}

void AttributesView::RenderEmptyState(SkCanvas* canvas, float x, float y, float width, float height) {
    // 使用 FontManager 获取字体（使用支持中文的字体）
    FontDescriptor font_desc;
    font_desc.family = "Microsoft YaHei";  // 微软雅黑同时支持中英文
    font_desc.size = 12.0f;
    font_desc.weight = FontWeight::NORMAL;
    font_desc.style = FontStyle::NORMAL;
    SkFont font = FontManager::GetInstance().LoadFont(font_desc);

    SkPaint text_paint;
    text_paint.setColor(SkColorSetRGB(128, 128, 128));
    text_paint.setAntiAlias(true);

    const char* message = "No attributes";
    float text_width = font.measureText(message, strlen(message), SkTextEncoding::kUTF8);
    float text_x = x + (width - text_width) / 2;
    float text_y = y + height / 2;

    canvas->drawString(message, text_x, text_y, font, text_paint);
}

void AttributesView::RenderAttributeList(SkCanvas* canvas, float x, float y, float width, float height,
                                          const std::vector<AttributeInfo>& attributes) {
    // 使用 FontManager 获取字体（使用支持中文的字体）
    FontDescriptor font_desc;
    font_desc.family = "Microsoft YaHei";  // 微软雅黑同时支持中英文
    font_desc.size = 12.0f;
    font_desc.weight = FontWeight::NORMAL;
    font_desc.style = FontStyle::NORMAL;
    SkFont font = FontManager::GetInstance().LoadFont(font_desc);

    float current_y = y + PADDING;

    for (const auto& attr : attributes) {
        // 属性名颜色
        SkPaint name_paint;
        name_paint.setAntiAlias(true);
        
        if (attr.is_id) {
            // id 属性用橙色高亮
            name_paint.setColor(SkColorSetRGB(255, 136, 0));
        } else if (attr.is_class) {
            // class 属性用绿色
            name_paint.setColor(SkColorSetRGB(86, 156, 86));
        } else if (attr.is_data) {
            // data-* 属性用紫色
            name_paint.setColor(SkColorSetRGB(136, 106, 168));
        } else {
            // 其他属性用默认颜色
            name_paint.setColor(SkColorSetRGB(156, 220, 254));
        }

        // 绘制属性名
        std::string name_text = attr.name + "=";
        canvas->drawString(name_text.c_str(), x + PADDING, current_y + 14, font, name_paint);

        // 属性值（蓝色）
        float name_width = font.measureText(name_text.c_str(), name_text.length(), SkTextEncoding::kUTF8);
        SkPaint value_paint;
        value_paint.setColor(SkColorSetRGB(206, 145, 120));
        value_paint.setAntiAlias(true);
        
        std::string value_text = "\"" + attr.value + "\"";
        canvas->drawString(value_text.c_str(), x + PADDING + name_width, current_y + 14, font, value_paint);

        current_y += ROW_HEIGHT;

        // 超出高度则停止
        if (current_y > y + height - ROW_HEIGHT) {
            break;
        }
    }
}

} // namespace mblink
