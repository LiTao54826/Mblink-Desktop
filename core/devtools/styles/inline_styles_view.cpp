/**
 * @file inline_styles_view.cpp
 * @brief 内联样式视图实现
 */

#include "inline_styles_view.h"
#include "core/dom/style/css_style_declaration.h"
#include "core/dom/document.h"
#include "core/lexbor/style_manager.h"
#include "core/render/text/font_manager.h"
#include "core/render/objects/render_object.h"
#include "core/window/window_manager.h"

#include "include/core/SkCanvas.h"
#include "include/core/SkPaint.h"
#include "include/core/SkRect.h"
#include "include/core/SkFont.h"

#include <sstream>
#include <functional>

namespace mblink {

namespace {
    const float ROW_HEIGHT = 20.0f;
    const float PADDING = 8.0f;
}

InlineStylesView::InlineStylesView() = default;

InlineStylesView::~InlineStylesView() = default;

void InlineStylesView::SetElement(std::shared_ptr<Element> element) {
    element_ = element;
}

void InlineStylesView::Render(SkCanvas* canvas, float x, float y, float width, float height) {
    auto element = element_.lock();
    auto properties = GetStyleProperties();

    if (properties.empty()) {
        RenderEmptyState(canvas, x, y, width, height);
    } else {
        RenderStyleList(canvas, x, y, width, height, properties);
    }
}

std::vector<InlineStylesView::StyleProperty> InlineStylesView::GetStyleProperties() const {
    std::vector<StyleProperty> properties;

    auto element = element_.lock();
    if (!element) return properties;

    // 获取 style 属性
    std::string style_attr = element->GetAttribute("style");
    if (style_attr.empty()) return properties;

    // 解析 style 属性
    // 格式: "property1: value1; property2: value2;"
    size_t pos = 0;
    while (pos < style_attr.length()) {
        // 找到冒号
        size_t colon_pos = style_attr.find(':', pos);
        if (colon_pos == std::string::npos) break;

        // 找到分号
        size_t semicolon_pos = style_attr.find(';', colon_pos);
        if (semicolon_pos == std::string::npos) {
            semicolon_pos = style_attr.length();
        }

        // 提取属性名和值
        std::string name = style_attr.substr(pos, colon_pos - pos);
        std::string value = style_attr.substr(colon_pos + 1, semicolon_pos - colon_pos - 1);

        // 去除空白
        auto trim = [](std::string& s) {
            size_t start = s.find_first_not_of(" \t\n\r");
            size_t end = s.find_last_not_of(" \t\n\r");
            if (start != std::string::npos && end != std::string::npos) {
                s = s.substr(start, end - start + 1);
            } else {
                s.clear();
            }
        };

        trim(name);
        trim(value);

        if (!name.empty()) {
            properties.push_back({name, value});
        }

        pos = semicolon_pos + 1;
    }

    return properties;
}

void InlineStylesView::RenderEmptyState(SkCanvas* canvas, float x, float y, float width, float height) {
    // 使用 FontManager 获取字体（使用支持中文的字体）
    FontDescriptor font_desc;
    font_desc.family = "Microsoft YaHei";  // 微软雅黑同时支持中英文
    font_desc.size = 12.0f;
    font_desc.weight = FontWeight::NORMAL;
    font_desc.style = FontStyle::NORMAL;
    SkFont font = FontManager::GetInstance().LoadFont(font_desc);

    auto element = element_.lock();
    if (!element) {
        SkPaint text_paint;
        text_paint.setColor(SkColorSetRGB(128, 128, 128));
        text_paint.setAntiAlias(true);
        canvas->drawString("No element selected", x + PADDING, y + height / 2, font, text_paint);
        content_height_ = height;
        return;
    }

    float start_y = y + PADDING;
    float current_y = start_y;

    // 显示元素信息
    SkPaint title_paint;
    title_paint.setColor(SkColorSetRGB(200, 200, 200));
    title_paint.setAntiAlias(true);
    
    std::string element_info = "<" + element->GetTagName() + ">";
    canvas->drawString(element_info.c_str(), x + PADDING, current_y + 14, font, title_paint);
    current_y += ROW_HEIGHT;

    // 显示元素的所有属性
    SkPaint label_paint;
    label_paint.setColor(SkColorSetRGB(128, 128, 128));
    label_paint.setAntiAlias(true);
    
    canvas->drawString("Attributes:", x + PADDING, current_y + 14, font, label_paint);
    current_y += ROW_HEIGHT;

    // 获取所有属性
    const auto& all_attrs = element->GetAllAttributes();
    std::vector<std::pair<std::string, std::string>> attrs;
    
    // 优先显示常见属性
    std::vector<std::string> priority_attrs = {"id", "class", "style", "href", "src", "type", "name", "value"};
    for (const auto& attr_name : priority_attrs) {
        auto it = all_attrs.find(attr_name);
        if (it != all_attrs.end() && !it->second.empty()) {
            attrs.push_back({it->first, it->second});
        }
    }
    
    // 添加其他属性
    for (const auto& [name, value] : all_attrs) {
        // 跳过已添加的优先属性
        bool is_priority = false;
        for (const auto& p : priority_attrs) {
            if (name == p) {
                is_priority = true;
                break;
            }
        }
        if (!is_priority && !value.empty()) {
            attrs.push_back({name, value});
        }
    }

    if (attrs.empty()) {
        canvas->drawString("  (no attributes)", x + PADDING, current_y + 14, font, label_paint);
        current_y += ROW_HEIGHT;
    } else {
        for (const auto& [name, value] : attrs) {
            // 属性名（紫色）
            SkPaint name_paint;
            name_paint.setColor(SkColorSetRGB(136, 106, 168));
            name_paint.setAntiAlias(true);
            canvas->drawString(("  " + name + "=").c_str(), x + PADDING, current_y + 14, font, name_paint);

            // 属性值（绿色）
            float name_width = font.measureText(("  " + name + "=").c_str(),
                                                name.length() + 3, SkTextEncoding::kUTF8);
            SkPaint value_paint;
            value_paint.setColor(SkColorSetRGB(206, 145, 120));
            value_paint.setAntiAlias(true);
            
            // 截断长值
            std::string display_value = value;
            if (display_value.length() > 30) {
                display_value = display_value.substr(0, 27) + "...";
            }
            canvas->drawString(("\"" + display_value + "\"").c_str(), x + PADDING + name_width, current_y + 14, font, value_paint);

            current_y += ROW_HEIGHT;
        }
    }

    // 显示内联样式部分
    current_y += ROW_HEIGHT / 2;  // 间隔
    canvas->drawString("element.style {}", x + PADDING, current_y + 14, font, label_paint);
    current_y += ROW_HEIGHT;

    // 显示匹配的 CSS 规则
    current_y += ROW_HEIGHT / 2;
    canvas->drawString("Matched CSS Rules:", x + PADDING, current_y + 14, font, label_paint);
    current_y += ROW_HEIGHT;

    // 获取匹配的规则
    auto owner_doc = element->GetOwnerDocument();
    if (owner_doc) {
        auto style_manager = owner_doc->GetStyleManager();
        if (style_manager) {
            auto rules = style_manager->GetMatchingRules(element.get());
            if (rules.empty()) {
                canvas->drawString("  (no matching rules)", x + PADDING, current_y + 14, font, label_paint);
            } else {
                for (const auto* rule : rules) {
                    if (!rule) continue;
                    
                    // 显示选择器
                    SkPaint selector_paint;
                    selector_paint.setColor(SkColorSetRGB(86, 156, 214));
                    selector_paint.setAntiAlias(true);
                    canvas->drawString((rule->selector + " {").c_str(), x + PADDING, current_y + 14, font, selector_paint);
                    current_y += ROW_HEIGHT;

                    // 显示声明
                    for (const auto& [prop_name, prop_value] : rule->declarations) {
                        // 属性名（紫色）
                        SkPaint name_paint;
                        name_paint.setColor(SkColorSetRGB(136, 106, 168));
                        name_paint.setAntiAlias(true);
                        canvas->drawString(("  " + prop_name + ":").c_str(), x + PADDING, current_y + 14, font, name_paint);

                        // 属性值（橙色）
                        float name_width = font.measureText(("  " + prop_name + ": ").c_str(),
                                                            prop_name.length() + 4, SkTextEncoding::kUTF8);
                        SkPaint value_paint;
                        value_paint.setColor(SkColorSetRGB(206, 145, 120));
                        value_paint.setAntiAlias(true);
                        canvas->drawString((prop_value + ";").c_str(), x + PADDING + name_width, current_y + 14, font, value_paint);

                        current_y += ROW_HEIGHT;
                    }

                    // 结束括号
                    canvas->drawString("}", x + PADDING, current_y + 14, font, selector_paint);
                    current_y += ROW_HEIGHT + 4;  // 规则之间的间隔
                }
            }
        }
    }

    // 显示计算后的布局信息
    current_y += ROW_HEIGHT / 2;
    canvas->drawString("Computed Layout:", x + PADDING, current_y + 14, font, label_paint);
    current_y += ROW_HEIGHT;

    // 从渲染树获取实际布局数据
    auto& wm = WindowManager::Instance();
    auto windows = wm.GetAllWindows();
    if (!windows.empty()) {
        auto window = windows[0];
        auto root_render = window->GetCachedRenderTree();
        if (root_render) {
            // 递归查找对应元素的 RenderObject
            std::function<std::shared_ptr<RenderObject>(std::shared_ptr<RenderObject>)> find_render_object;
            find_render_object = [&](std::shared_ptr<RenderObject> obj) -> std::shared_ptr<RenderObject> {
                if (!obj) return nullptr;
                
                auto node = obj->GetNode();
                if (node && node.get() == element.get()) {
                    return obj;
                }
                
                for (const auto& child : obj->GetChildren()) {
                    auto found = find_render_object(child);
                    if (found) return found;
                }
                
                return nullptr;
            };
            
            auto render_obj = find_render_object(root_render);
            if (render_obj) {
                const auto& layout = render_obj->GetLayoutInfo();
                const auto& style = render_obj->GetComputedStyle();
                
                SkPaint value_paint;
                value_paint.setColor(SkColorSetRGB(206, 145, 120));
                value_paint.setAntiAlias(true);
                
                // 显示尺寸
                std::string size_str = "  size: " + std::to_string(static_cast<int>(layout.width)) + 
                                       " x " + std::to_string(static_cast<int>(layout.height)) + " px";
                canvas->drawString(size_str.c_str(), x + PADDING, current_y + 14, font, value_paint);
                current_y += ROW_HEIGHT;
                
                // 显示位置
                std::string pos_str = "  position: (" + std::to_string(static_cast<int>(layout.x)) + 
                                      ", " + std::to_string(static_cast<int>(layout.y)) + ")";
                canvas->drawString(pos_str.c_str(), x + PADDING, current_y + 14, font, value_paint);
                current_y += ROW_HEIGHT;
                
                // 显示 margin
                float margin_top = style.margin_top.ToPx(layout.width, style.font_size);
                float margin_right = style.margin_right.ToPx(layout.width, style.font_size);
                float margin_bottom = style.margin_bottom.ToPx(layout.width, style.font_size);
                float margin_left = style.margin_left.ToPx(layout.width, style.font_size);
                std::string margin_str = "  margin: " + std::to_string(static_cast<int>(margin_top)) + " " +
                                         std::to_string(static_cast<int>(margin_right)) + " " +
                                         std::to_string(static_cast<int>(margin_bottom)) + " " +
                                         std::to_string(static_cast<int>(margin_left));
                canvas->drawString(margin_str.c_str(), x + PADDING, current_y + 14, font, value_paint);
                current_y += ROW_HEIGHT;
                
                // 显示 padding
                float padding_top = style.padding_top.ToPx(layout.width, style.font_size);
                float padding_right = style.padding_right.ToPx(layout.width, style.font_size);
                float padding_bottom = style.padding_bottom.ToPx(layout.width, style.font_size);
                float padding_left = style.padding_left.ToPx(layout.width, style.font_size);
                std::string padding_str = "  padding: " + std::to_string(static_cast<int>(padding_top)) + " " +
                                          std::to_string(static_cast<int>(padding_right)) + " " +
                                          std::to_string(static_cast<int>(padding_bottom)) + " " +
                                          std::to_string(static_cast<int>(padding_left));
                canvas->drawString(padding_str.c_str(), x + PADDING, current_y + 14, font, value_paint);
                current_y += ROW_HEIGHT;
                
                // 显示 border
                std::string border_str = "  border: " + std::to_string(static_cast<int>(style.border_top_width)) + " " +
                                         std::to_string(static_cast<int>(style.border_right_width)) + " " +
                                         std::to_string(static_cast<int>(style.border_bottom_width)) + " " +
                                         std::to_string(static_cast<int>(style.border_left_width));
                canvas->drawString(border_str.c_str(), x + PADDING, current_y + 14, font, value_paint);
                current_y += ROW_HEIGHT;
            } else {
                canvas->drawString("  (no render object)", x + PADDING, current_y + 14, font, label_paint);
            }
        } else {
            canvas->drawString("  (no render tree)", x + PADDING, current_y + 14, font, label_paint);
        }
    } else {
        canvas->drawString("  (no window)", x + PADDING, current_y + 14, font, label_paint);
    }
    
    // 计算内容总高度
    content_height_ = current_y - start_y + ROW_HEIGHT + PADDING;
}

void InlineStylesView::RenderStyleList(SkCanvas* canvas, float x, float y, float width, float height,
                                        const std::vector<StyleProperty>& properties) {
    // 使用 FontManager 获取字体（使用支持中文的字体）
    FontDescriptor font_desc;
    font_desc.family = "Microsoft YaHei";  // 微软雅黑同时支持中英文
    font_desc.size = 12.0f;
    font_desc.weight = FontWeight::NORMAL;
    font_desc.style = FontStyle::NORMAL;
    SkFont font = FontManager::GetInstance().LoadFont(font_desc);

    float start_y = y + PADDING;
    float current_y = start_y;

    // 标题
    SkPaint title_paint;
    title_paint.setColor(SkColorSetRGB(200, 200, 200));
    title_paint.setAntiAlias(true);
    canvas->drawString("element.style {", x + PADDING, current_y + 14, font, title_paint);
    current_y += ROW_HEIGHT;

    // 样式属性
    for (const auto& prop : properties) {
        // 属性名（紫色）
        SkPaint name_paint;
        name_paint.setColor(SkColorSetRGB(136, 106, 168));
        name_paint.setAntiAlias(true);
        canvas->drawString(("  " + prop.name + ":").c_str(), x + PADDING, current_y + 14, font, name_paint);

        // 属性值（蓝色）
        float name_width = font.measureText(("  " + prop.name + ": ").c_str(),
                                            prop.name.length() + 4, SkTextEncoding::kUTF8);
        SkPaint value_paint;
        value_paint.setColor(SkColorSetRGB(86, 156, 214));
        value_paint.setAntiAlias(true);
        canvas->drawString(prop.value.c_str(), x + PADDING + name_width, current_y + 14, font, value_paint);

        // 分号
        float value_width = font.measureText(prop.value.c_str(), prop.value.length(), SkTextEncoding::kUTF8);
        SkPaint semi_paint;
        semi_paint.setColor(SkColorSetRGB(200, 200, 200));
        semi_paint.setAntiAlias(true);
        canvas->drawString(";", x + PADDING + name_width + value_width, current_y + 14, font, semi_paint);

        current_y += ROW_HEIGHT;
    }

    // 结束括号
    canvas->drawString("}", x + PADDING, current_y + 14, font, title_paint);
    
    // 计算内容总高度
    content_height_ = current_y - start_y + ROW_HEIGHT + PADDING;
}

} // namespace mblink
