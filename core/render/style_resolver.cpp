/**
 * @file style_resolver.cpp
 * @brief 样式解析器实现
 */

#include "style_resolver.h"
#include "core/dom/text.h"
#include "color.h"
#include <algorithm>
#include <sstream>

namespace lightui {

// ========== StyleResolver 实现 ==========

StyleResolver::StyleResolver() {
    // 初始化可继承属性集合
    inheritable_properties_ = {
        "color",
        "font-family",
        "font-size",
        "font-weight",
        "font-style",
        "line-height",
        "text-align",
        "text-decoration",
        "opacity"
    };
}

ComputedStyle StyleResolver::ResolveStyle(std::shared_ptr<Element> element,
                                         const ComputedStyle* parent_style) {
    if (!element) {
        return ComputedStyle();
    }

    ComputedStyle style;

    // CSS 层叠顺序（从低到高优先级）：
    // 1. 用户代理样式表（默认样式）- 最低优先级
    ApplyDefaultStyle(style, element->GetTagName(), parent_style == nullptr);

    // 2. 继承的值 - 从父元素继承可继承属性
    if (parent_style) {
        ApplyInheritance(style, parent_style);
    }

    // 3. 元素特定的默认样式 - 覆盖继承（如 h1 的 font-size, strong 的 bold）
    // 这一步必须在继承之后，以确保元素自身的样式优先级高于继承
    ApplyElementSpecificStyle(style, element->GetTagName());

    // 4. 内联样式（最高优先级）- 覆盖所有
    ApplyInlineStyle(style, element);

    return style;
}

ComputedStyle StyleResolver::GetDefaultStyle(const std::string& tag_name) {
    // 检查缓存
    auto it = default_styles_.find(tag_name);
    if (it != default_styles_.end()) {
        return it->second;
    }

    ComputedStyle style;
    ApplyDefaultStyle(style, tag_name, true);
    ApplyElementSpecificStyle(style, tag_name);

    // 缓存
    default_styles_[tag_name] = style;

    return style;
}

void StyleResolver::ApplyDefaultStyle(ComputedStyle& style, const std::string& tag_name, bool is_root) {
    // 只在根元素时设置基础默认值
    if (is_root) {
        style.color = "#000000";
        style.font_family = "Arial";
        style.font_size = 16.0f;
        style.font_weight = "normal";
        style.font_style = "normal";
        style.text_align = "left";
        style.text_decoration = "none";
        style.line_height = 1.2f;
        style.opacity = 1.0f;
    }

    // 设置 display 属性（不可继承）
    style.display = RenderObjectType::BLOCK;

    if (tag_name == "div" || tag_name == "p" || tag_name == "section" ||
        tag_name == "article" || tag_name == "header" || tag_name == "footer" ||
        tag_name == "h1" || tag_name == "h2" || tag_name == "h3" ||
        tag_name == "h4" || tag_name == "h5" || tag_name == "h6") {
        style.display = RenderObjectType::BLOCK;
    }
    else if (tag_name == "span" || tag_name == "a" || tag_name == "strong" ||
             tag_name == "em" || tag_name == "b" || tag_name == "i" ||
             tag_name == "u" || tag_name == "s" || tag_name == "strike" || tag_name == "del" ||
             tag_name == "mark" || tag_name == "small" || tag_name == "big" ||
             tag_name == "sub" || tag_name == "sup" || tag_name == "code" ||
             tag_name == "kbd" || tag_name == "samp" || tag_name == "var" ||
             tag_name == "abbr" || tag_name == "cite" || tag_name == "dfn" ||
             tag_name == "q" || tag_name == "time") {
        style.display = RenderObjectType::INLINE;
    }
    else if (tag_name == "img" || tag_name == "button" || tag_name == "input") {
        style.display = RenderObjectType::INLINE_BLOCK;
    }
}

void StyleResolver::ApplyElementSpecificStyle(ComputedStyle& style, const std::string& tag_name) {
    // ========== 块级元素 ==========

    // HTML, BODY
    if (tag_name == "html" || tag_name == "body") {
        style.margin.top = CSSLength(0, CSSUnit::PX);
        style.margin.bottom = CSSLength(0, CSSUnit::PX);
        style.margin.left = CSSLength(0, CSSUnit::PX);
        style.margin.right = CSSLength(0, CSSUnit::PX);
    }

    // 标题 (Headings)
    if (tag_name == "h1") {
        style.font_size = 32.0f;  // 2em
        style.font_weight = "bold";
        style.margin.top = CSSLength(21, CSSUnit::PX);  // 0.67em
        style.margin.bottom = CSSLength(21, CSSUnit::PX);
    } else if (tag_name == "h2") {
        style.font_size = 24.0f;  // 1.5em
        style.font_weight = "bold";
        style.margin.top = CSSLength(19, CSSUnit::PX);  // 0.83em
        style.margin.bottom = CSSLength(19, CSSUnit::PX);
    } else if (tag_name == "h3") {
        style.font_size = 18.72f;  // 1.17em
        style.font_weight = "bold";
        style.margin.top = CSSLength(18, CSSUnit::PX);  // 1em
        style.margin.bottom = CSSLength(18, CSSUnit::PX);
    } else if (tag_name == "h4") {
        style.font_size = 16.0f;  // 1em
        style.font_weight = "bold";
        style.margin.top = CSSLength(21, CSSUnit::PX);  // 1.33em
        style.margin.bottom = CSSLength(21, CSSUnit::PX);
    } else if (tag_name == "h5") {
        style.font_size = 13.28f;  // 0.83em
        style.font_weight = "bold";
        style.margin.top = CSSLength(22, CSSUnit::PX);  // 1.67em
        style.margin.bottom = CSSLength(22, CSSUnit::PX);
    } else if (tag_name == "h6") {
        style.font_size = 10.72f;  // 0.67em
        style.font_weight = "bold";
        style.margin.top = CSSLength(24, CSSUnit::PX);  // 2.33em
        style.margin.bottom = CSSLength(24, CSSUnit::PX);
    }

    // 段落 (Paragraph)
    if (tag_name == "p") {
        style.margin.top = CSSLength(16, CSSUnit::PX);  // 1em
        style.margin.bottom = CSSLength(16, CSSUnit::PX);
        style.margin.left = CSSLength(0, CSSUnit::PX);
        style.margin.right = CSSLength(0, CSSUnit::PX);
    }

    // DIV - 无特殊样式，使用默认块级样式

    // 引用块 (Blockquote)
    if (tag_name == "blockquote") {
        style.margin.top = CSSLength(16, CSSUnit::PX);  // 1em
        style.margin.bottom = CSSLength(16, CSSUnit::PX);
        style.margin.left = CSSLength(40, CSSUnit::PX);
        style.margin.right = CSSLength(40, CSSUnit::PX);
    }

    // 预格式化文本 (Preformatted)
    if (tag_name == "pre") {
        style.font_family = "monospace";
        style.margin.top = CSSLength(16, CSSUnit::PX);  // 1em
        style.margin.bottom = CSSLength(16, CSSUnit::PX);
        // TODO: 添加 white-space: pre 支持
    }

    // 代码 (Code)
    if (tag_name == "code") {
        style.font_family = "monospace";
    }

    // 水平线 (Horizontal Rule)
    if (tag_name == "hr") {
        style.margin.top = CSSLength(8, CSSUnit::PX);  // 0.5em
        style.margin.bottom = CSSLength(8, CSSUnit::PX);
        style.border.width = CSSLength(1, CSSUnit::PX);
        style.border.style = CSSBorderStyle::SOLID;
        style.border.color = Color::FromRGB(128, 128, 128);
    }

    // ========== 列表 (Lists) ==========

    if (tag_name == "ul" || tag_name == "ol") {
        style.margin.top = CSSLength(16, CSSUnit::PX);  // 1em
        style.margin.bottom = CSSLength(16, CSSUnit::PX);
        style.padding.left = CSSLength(40, CSSUnit::PX);  // 左侧缩进
    }

    if (tag_name == "li") {
        style.display = RenderObjectType::BLOCK;  // 列表项是块级
    }

    if (tag_name == "dl") {  // Definition List
        style.margin.top = CSSLength(16, CSSUnit::PX);
        style.margin.bottom = CSSLength(16, CSSUnit::PX);
    }

    if (tag_name == "dt") {  // Definition Term
        style.font_weight = "bold";
    }

    if (tag_name == "dd") {  // Definition Description
        style.margin.left = CSSLength(40, CSSUnit::PX);
    }

    // ========== 表格 (Tables) ==========

    if (tag_name == "table") {
        style.border.style = CSSBorderStyle::SOLID;
        style.border.width = CSSLength(1, CSSUnit::PX);
        style.border.color = Color::FromRGB(128, 128, 128);
    }

    if (tag_name == "td" || tag_name == "th") {
        style.padding.top = CSSLength(2, CSSUnit::PX);
        style.padding.bottom = CSSLength(2, CSSUnit::PX);
        style.padding.left = CSSLength(2, CSSUnit::PX);
        style.padding.right = CSSLength(2, CSSUnit::PX);
    }

    if (tag_name == "th") {
        style.font_weight = "bold";
        style.text_align = "center";
    }

    // ========== 表单元素 (Form Elements) ==========

    if (tag_name == "form") {
        style.margin.top = CSSLength(0, CSSUnit::PX);
    }

    if (tag_name == "fieldset") {
        style.margin.left = CSSLength(2, CSSUnit::PX);
        style.margin.right = CSSLength(2, CSSUnit::PX);
        style.padding.top = CSSLength(10, CSSUnit::PX);
        style.padding.bottom = CSSLength(10, CSSUnit::PX);
        style.padding.left = CSSLength(10, CSSUnit::PX);
        style.padding.right = CSSLength(10, CSSUnit::PX);
        style.border.width = CSSLength(2, CSSUnit::PX);
        style.border.style = CSSBorderStyle::SOLID;
        style.border.color = Color::FromRGB(192, 192, 192);
    }

    if (tag_name == "legend") {
        style.padding.left = CSSLength(2, CSSUnit::PX);
        style.padding.right = CSSLength(2, CSSUnit::PX);
    }

    if (tag_name == "button" || tag_name == "input" || tag_name == "select" || tag_name == "textarea") {
        style.padding.top = CSSLength(2, CSSUnit::PX);
        style.padding.bottom = CSSLength(2, CSSUnit::PX);
        style.padding.left = CSSLength(6, CSSUnit::PX);
        style.padding.right = CSSLength(6, CSSUnit::PX);
        style.border.width = CSSLength(2, CSSUnit::PX);
        style.border.style = CSSBorderStyle::SOLID;
        style.border.color = Color::FromRGB(169, 169, 169);
    }

    if (tag_name == "button") {
        style.background_color = "#F0F0F0";
        style.border_radius.top_left = CSSLength(2, CSSUnit::PX);
        style.border_radius.top_right = CSSLength(2, CSSUnit::PX);
        style.border_radius.bottom_left = CSSLength(2, CSSUnit::PX);
        style.border_radius.bottom_right = CSSLength(2, CSSUnit::PX);
    }

    // ========== 内联元素 (Inline Elements) ==========

    // 粗体 (Bold)
    if (tag_name == "strong" || tag_name == "b") {
        style.font_weight = "bold";
    }

    // 斜体 (Italic)
    if (tag_name == "em" || tag_name == "i") {
        style.font_style = "italic";
    }

    // 下划线 (Underline)
    if (tag_name == "u") {
        style.text_decoration = "underline";
    }

    // 删除线 (Strikethrough)
    if (tag_name == "s" || tag_name == "strike" || tag_name == "del") {
        style.text_decoration = "line-through";
    }

    // 上标和下标 (Superscript & Subscript)
    if (tag_name == "sup" || tag_name == "sub") {
        style.font_size = 12.0f;  // 0.75em
    }

    // 小字体 (Small)
    if (tag_name == "small") {
        style.font_size = 13.28f;  // 0.83em
    }

    // 大字体 (Big)
    if (tag_name == "big") {
        style.font_size = 18.72f;  // 1.17em
    }

    // 链接 (Anchor)
    if (tag_name == "a") {
        style.color = "#0000EE";  // 蓝色
        style.text_decoration = "underline";
    }

    // 标记/高亮 (Mark)
    if (tag_name == "mark") {
        style.background_color = "#FFFF00";  // 黄色背景
        style.color = "#000000";
    }

    // 引用 (Quote)
    if (tag_name == "q") {
        // 浏览器通常会自动添加引号，这里简化处理
    }

    // 缩写 (Abbreviation)
    if (tag_name == "abbr") {
        style.text_decoration = "underline dotted";
    }
}

void StyleResolver::ApplyInlineStyle(ComputedStyle& style, std::shared_ptr<Element> element) {
    // 获取style属性
    std::string style_attr = element->GetAttribute("style");
    if (style_attr.empty()) {
        return;
    }

    // 解析内联样式（格式：property: value; property: value;）
    std::istringstream iss(style_attr);
    std::string declaration;

    while (std::getline(iss, declaration, ';')) {
        // 去除前后空格
        size_t start = declaration.find_first_not_of(" \t\n\r");
        if (start == std::string::npos) {
            continue;
        }

        size_t end = declaration.find_last_not_of(" \t\n\r");
        declaration = declaration.substr(start, end - start + 1);

        // 查找冒号
        size_t colon_pos = declaration.find(':');
        if (colon_pos == std::string::npos) {
            continue;
        }

        // 提取属性名和值
        std::string property = declaration.substr(0, colon_pos);
        std::string value = declaration.substr(colon_pos + 1);

        // 去除属性名和值的空格
        start = property.find_first_not_of(" \t\n\r");
        if (start != std::string::npos) {
            end = property.find_last_not_of(" \t\n\r");
            property = property.substr(start, end - start + 1);
        }

        start = value.find_first_not_of(" \t\n\r");
        if (start != std::string::npos) {
            end = value.find_last_not_of(" \t\n\r");
            value = value.substr(start, end - start + 1);
        }

        // 应用样式属性
        if (!property.empty() && !value.empty()) {
            ParseStyleProperty(style, property, value);
        }
    }
}

void StyleResolver::ApplyInheritance(ComputedStyle& style, const ComputedStyle* parent_style) {
    if (!parent_style) {
        return;
    }
    
    // 继承可继承属性
    style.color = parent_style->color;
    style.font_family = parent_style->font_family;
    style.font_size = parent_style->font_size;
    style.font_weight = parent_style->font_weight;
    style.font_style = parent_style->font_style;
    style.line_height = parent_style->line_height;
    style.text_align = parent_style->text_align;
    style.text_decoration = parent_style->text_decoration;
}

void StyleResolver::ParseStyleProperty(ComputedStyle& style, 
                                       const std::string& property, 
                                       const std::string& value) {
    if (property == "display") {
        style.display = ParseDisplay(value);
    }
    else if (property == "width") {
        style.width = CSSValue::ParseLength(value);
    }
    else if (property == "height") {
        style.height = CSSValue::ParseLength(value);
    }
    else if (property == "min-width") {
        style.min_width = CSSValue::ParseLength(value);
    }
    else if (property == "max-width") {
        style.max_width = CSSValue::ParseLength(value);
    }
    else if (property == "min-height") {
        style.min_height = CSSValue::ParseLength(value);
    }
    else if (property == "max-height") {
        style.max_height = CSSValue::ParseLength(value);
    }
    else if (property == "margin") {
        style.margin = CSSValue::ParseEdges(value);
    }
    else if (property == "margin-top") {
        style.margin.top = CSSValue::ParseLength(value);
    }
    else if (property == "margin-right") {
        style.margin.right = CSSValue::ParseLength(value);
    }
    else if (property == "margin-bottom") {
        style.margin.bottom = CSSValue::ParseLength(value);
    }
    else if (property == "margin-left") {
        style.margin.left = CSSValue::ParseLength(value);
    }
    else if (property == "padding") {
        style.padding = CSSValue::ParseEdges(value);
    }
    else if (property == "padding-top") {
        style.padding.top = CSSValue::ParseLength(value);
    }
    else if (property == "padding-right") {
        style.padding.right = CSSValue::ParseLength(value);
    }
    else if (property == "padding-bottom") {
        style.padding.bottom = CSSValue::ParseLength(value);
    }
    else if (property == "padding-left") {
        style.padding.left = CSSValue::ParseLength(value);
    }
    else if (property == "border-width") {
        style.border.width = CSSValue::ParseLength(value);
    }
    else if (property == "border-style") {
        style.border.style = CSSValue::ParseBorderStyle(value);
    }
    else if (property == "border-color") {
        style.border.color = CSSValue::ParseColor(value);
    }
    else if (property == "border-radius") {
        style.border_radius = CSSValue::ParseBorderRadius(value);
    }
    else if (property == "background-color") {
        style.background_color = value;
    }
    else if (property == "background-image") {
        style.background_image = value;
    }
    else if (property == "background-repeat") {
        style.background_repeat = CSSValue::ParseBackgroundRepeat(value);
    }
    else if (property == "background-size") {
        style.background_size = CSSValue::ParseBackgroundSize(value);
    }
    else if (property == "color") {
        style.color = value;
    }
    else if (property == "font-family") {
        style.font_family = value;
    }
    else if (property == "font-size") {
        auto length = CSSValue::ParseLength(value);
        style.font_size = length.ToPx(16.0f, 16.0f); // 默认基准 16px
    }
    else if (property == "font-weight") {
        style.font_weight = value;
    }
    else if (property == "font-style") {
        style.font_style = value;
    }
    else if (property == "text-align") {
        style.text_align = value;
    }
    else if (property == "text-decoration") {
        style.text_decoration = value;
    }
    else if (property == "line-height") {
        auto length = CSSValue::ParseLength(value);
        if (length.unit == CSSUnit::NONE) {
            style.line_height = length.value; // 无单位表示倍数
        } else {
            style.line_height = length.ToPx(style.font_size, style.font_size) / style.font_size;
        }
    }
    else if (property == "box-shadow") {
        style.box_shadow = CSSValue::ParseBoxShadow(value);
    }
    else if (property == "opacity") {
        try {
            style.opacity = std::stof(value);
            style.opacity = std::max(0.0f, std::min(1.0f, style.opacity));
        } catch (...) {
            style.opacity = 1.0f;
        }
    }
}

bool StyleResolver::IsInheritableProperty(const std::string& property) {
    return inheritable_properties_.find(property) != inheritable_properties_.end();
}

RenderObjectType StyleResolver::ParseDisplay(const std::string& value) {
    if (value == "block") return RenderObjectType::BLOCK;
    if (value == "inline") return RenderObjectType::INLINE;
    if (value == "inline-block") return RenderObjectType::INLINE_BLOCK;
    if (value == "flex") return RenderObjectType::FLEX;
    if (value == "none") return RenderObjectType::NONE;
    return RenderObjectType::BLOCK;
}

// ========== RenderTreeBuilder 实现 ==========

RenderTreeBuilder::RenderTreeBuilder()
    : style_resolver_() {
}

std::shared_ptr<RenderObject> RenderTreeBuilder::BuildRenderTree(
    std::shared_ptr<Node> node,
    const ComputedStyle* parent_style) {
    
    if (!node) {
        return nullptr;
    }
    
    std::shared_ptr<RenderObject> render_obj;
    
    // 根据节点类型创建渲染对象
    if (node->GetNodeType() == NodeType::ELEMENT_NODE) {
        auto element = std::static_pointer_cast<Element>(node);
        render_obj = CreateRenderObjectForElement(element, parent_style);
    }
    else if (node->GetNodeType() == NodeType::TEXT_NODE) {
        auto text = std::static_pointer_cast<Text>(node);
        render_obj = CreateRenderObjectForText(text, parent_style);
    }
    
    if (!render_obj) {
        return nullptr;
    }
    
    // 如果是 display: none，不创建渲染对象
    if (render_obj->GetComputedStyle().display == RenderObjectType::NONE) {
        return nullptr;
    }
    
    // 递归构建子树
    const auto& children = node->GetChildNodes();
    for (const auto& child : children) {
        auto child_render_obj = BuildRenderTree(child, &render_obj->GetComputedStyle());
        if (child_render_obj) {
            render_obj->AppendChild(child_render_obj);
        }
    }
    
    return render_obj;
}

std::shared_ptr<RenderObject> RenderTreeBuilder::CreateRenderObjectForElement(
    std::shared_ptr<Element> element,
    const ComputedStyle* parent_style) {
    
    // 计算样式
    auto style = style_resolver_.ResolveStyle(element, parent_style);
    
    // 创建渲染对象
    auto render_obj = CreateRenderObjectByType(style.display);
    if (render_obj) {
        render_obj->SetNode(element);
        render_obj->SetComputedStyle(style);
    }
    
    return render_obj;
}

std::shared_ptr<RenderObject> RenderTreeBuilder::CreateRenderObjectForText(
    std::shared_ptr<Text> text,
    const ComputedStyle* parent_style) {
    
    auto render_obj = std::make_shared<RenderText>();
    render_obj->SetNode(text);
    render_obj->SetText(text->GetData());
    
    // 文本节点继承父元素样式
    if (parent_style) {
        render_obj->SetComputedStyle(*parent_style);
    }
    
    return render_obj;
}

std::shared_ptr<RenderObject> RenderTreeBuilder::CreateRenderObjectByType(RenderObjectType type) {
    switch (type) {
        case RenderObjectType::BLOCK:
            return std::make_shared<RenderBlock>();
        case RenderObjectType::INLINE:
            return std::make_shared<RenderInline>();
        case RenderObjectType::TEXT:
            return std::make_shared<RenderText>();
        case RenderObjectType::INLINE_BLOCK:
            return std::make_shared<RenderBlock>(); // 简化：暂时用 Block
        case RenderObjectType::FLEX:
            return std::make_shared<RenderBlock>(); // 简化：暂时用 Block
        case RenderObjectType::NONE:
            return nullptr;
        default:
            return std::make_shared<RenderBlock>();
    }
}

} // namespace lightui

