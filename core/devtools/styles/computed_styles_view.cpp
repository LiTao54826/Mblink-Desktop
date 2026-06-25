/**
 * @file computed_styles_view.cpp
 * @brief 计算样式视图实现
 */

#include "computed_styles_view.h"
#include "core/window/window_manager.h"
#include "core/render/objects/render_object.h"
#include "core/render/text/font_manager.h"

#include "include/core/SkCanvas.h"
#include "include/core/SkPaint.h"
#include "include/core/SkRect.h"
#include "include/core/SkFont.h"

#include <functional>
#include <sstream>
#include <iomanip>

namespace mblink {

namespace {
    const float ROW_HEIGHT = 18.0f;
    const float CATEGORY_HEIGHT = 24.0f;
    const float PADDING = 8.0f;

    // 样式类别定义
    const std::vector<std::pair<std::string, std::vector<std::string>>> CATEGORY_PROPERTIES = {
        {"Layout", {"display", "position", "top", "right", "bottom", "left", "z-index", "overflow", "visibility"}},
        {"Box Model", {"width", "height", "min-width", "max-width", "min-height", "max-height", "margin-top", "margin-right", "margin-bottom", "margin-left", "padding-top", "padding-right", "padding-bottom", "padding-left", "border-top-width", "border-right-width", "border-bottom-width", "border-left-width"}},
        {"Typography", {"font-family", "font-size", "font-weight", "font-style", "line-height", "text-align", "text-decoration", "white-space"}},
        {"Colors", {"color", "background-color", "opacity"}},
        {"Flexbox", {"flex-direction", "flex-wrap", "justify-content", "align-items", "align-content", "align-self", "flex-grow", "flex-shrink", "flex-basis", "order"}},
        {"Grid", {"grid-template-columns", "grid-template-rows", "grid-column-gap", "grid-row-gap", "grid-column", "grid-row"}},
        {"Effects", {"box-shadow", "transform"}}
    };
    
    // RenderObjectType 转字符串
    std::string RenderObjectTypeToString(RenderObjectType type) {
        switch (type) {
            case RenderObjectType::BLOCK: return "block";
            case RenderObjectType::INLINE: return "inline";
            case RenderObjectType::TEXT: return "text";
            case RenderObjectType::INLINE_BLOCK: return "inline-block";
            case RenderObjectType::FLEX: return "flex";
            case RenderObjectType::GRID: return "grid";
            case RenderObjectType::TABLE: return "table";
            case RenderObjectType::TABLE_ROW: return "table-row";
            case RenderObjectType::TABLE_CELL: return "table-cell";
            case RenderObjectType::NONE: return "none";
            default: return "block";
        }
    }
    
    // 格式化浮点数（去除多余小数位）
    std::string FormatFloat(float value) {
        if (value == static_cast<int>(value)) {
            return std::to_string(static_cast<int>(value));
        }
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << value;
        std::string str = oss.str();
        // 去除尾部的0
        str.erase(str.find_last_not_of('0') + 1, std::string::npos);
        if (str.back() == '.') str.pop_back();
        return str;
    }
    
    // CSSLength 转字符串
    std::string CSSLengthToString(const CSSLength& len) {
        if (len.IsAuto()) {
            return "auto";
        }
        if (len.is_calc) {
            return "calc(" + FormatFloat(len.calc_percent) + "% + " + FormatFloat(len.calc_px) + "px)";
        }
        switch (len.unit) {
            case CSSUnit::PX: return FormatFloat(len.value) + "px";
            case CSSUnit::PERCENT: return FormatFloat(len.value) + "%";
            case CSSUnit::EM: return FormatFloat(len.value) + "em";
            case CSSUnit::REM: return FormatFloat(len.value) + "rem";
            case CSSUnit::NONE: return FormatFloat(len.value);
            default: return FormatFloat(len.value) + "px";
        }
    }
    
    // CSS 属性默认值映射
    const std::unordered_map<std::string, std::string> DEFAULT_VALUES = {
        {"display", "block"},
        {"position", "static"},
        {"top", "auto"},
        {"right", "auto"},
        {"bottom", "auto"},
        {"left", "auto"},
        {"z-index", "0"},
        {"overflow", "visible"},
        {"visibility", "visible"},
        {"width", "auto"},
        {"height", "auto"},
        {"min-width", "auto"},
        {"max-width", "none"},
        {"min-height", "auto"},
        {"max-height", "none"},
        {"margin-top", "0px"},
        {"margin-right", "0px"},
        {"margin-bottom", "0px"},
        {"margin-left", "0px"},
        {"padding-top", "0px"},
        {"padding-right", "0px"},
        {"padding-bottom", "0px"},
        {"padding-left", "0px"},
        {"border-top-width", "0px"},
        {"border-right-width", "0px"},
        {"border-bottom-width", "0px"},
        {"border-left-width", "0px"},
        {"font-family", ""},
        {"font-size", "16px"},
        {"font-weight", "normal"},
        {"font-style", "normal"},
        {"line-height", "1.2"},
        {"text-align", ""},
        {"text-decoration", ""},
        {"white-space", "normal"},
        {"color", ""},
        {"background-color", ""},
        {"opacity", "1"},
        {"flex-direction", "row"},
        {"flex-wrap", "nowrap"},
        {"justify-content", "flex-start"},
        {"align-items", "normal"},
        {"align-content", "normal"},
        {"align-self", "auto"},
        {"flex-grow", "0"},
        {"flex-shrink", "1"},
        {"flex-basis", "auto"},
        {"order", "0"},
        {"grid-template-columns", ""},
        {"grid-template-rows", ""},
        {"grid-column-gap", "0px"},
        {"grid-row-gap", "0px"},
        {"grid-column", ""},
        {"grid-row", ""},
        {"box-shadow", "none"},
        {"transform", "none"},
    };
}

ComputedStylesView::ComputedStylesView() = default;

ComputedStylesView::~ComputedStylesView() = default;

void ComputedStylesView::SetElement(std::shared_ptr<Element> element) {
    element_ = element;
    RefreshStyles();
}

void ComputedStylesView::Render(SkCanvas* canvas, float x, float y, float width, float height) {
    // 裁剪区域
    canvas->save();
    canvas->clipRect(SkRect::MakeXYWH(x, y, width, height));

    float current_y = y + PADDING;

    for (const auto& category : categories_) {
        RenderCategory(canvas, category, x, current_y, width);
    }

    canvas->restore();
}

std::vector<ComputedStyleCategory> ComputedStylesView::GetComputedStyles() const {
    return categories_;
}

void ComputedStylesView::RefreshStyles() {
    categories_.clear();

    auto element = element_.lock();
    if (!element) return;

    // 查找对应的 RenderObject
    auto render_obj = FindRenderObject(element);
    
    for (const auto& [cat_name, props] : CATEGORY_PROPERTIES) {
        ComputedStyleCategory category;
        category.name = cat_name;
        category.expanded = true;

        for (const auto& prop_name : props) {
            ComputedStyleProperty prop;
            prop.name = prop_name;
            
            if (render_obj) {
                // 从 RenderObject 获取真实的计算样式
                const auto& style = render_obj->GetComputedStyle();
                prop.value = GetPropertyValue(style, prop_name);
                prop.is_default = IsDefaultValue(prop_name, prop.value);
            } else {
                // RenderObject 未找到，显示 N/A
                prop.value = "N/A";
                prop.is_default = true;
            }
            
            prop.is_inherited = false;  // TODO: 实现继承检测
            category.properties.push_back(prop);
        }

        categories_.push_back(category);
    }
}

void ComputedStylesView::RenderCategory(SkCanvas* canvas, const ComputedStyleCategory& category,
                                         float x, float& y, float width) {
    // 使用 FontManager 获取字体（使用支持中文的字体）
    FontDescriptor font_desc;
    font_desc.family = "Microsoft YaHei";  // 微软雅黑同时支持中英文
    font_desc.size = 11.0f;
    font_desc.weight = FontWeight::NORMAL;
    font_desc.style = FontStyle::NORMAL;
    SkFont font = FontManager::GetInstance().LoadFont(font_desc);

    // 类别标题
    SkPaint title_bg;
    title_bg.setColor(SkColorSetRGB(45, 45, 45));
    canvas->drawRect(SkRect::MakeXYWH(x, y, width, CATEGORY_HEIGHT), title_bg);

    SkPaint title_paint;
    title_paint.setColor(SkColorSetRGB(200, 200, 200));
    title_paint.setAntiAlias(true);

    // 展开/折叠图标
    std::string icon = category.expanded ? "▼ " : "▶ ";
    canvas->drawString((icon + category.name).c_str(), x + PADDING, y + 16, font, title_paint);

    y += CATEGORY_HEIGHT;

    // 如果展开，显示属性
    if (category.expanded) {
        for (const auto& prop : category.properties) {
            // 属性名
            SkPaint name_paint;
            name_paint.setColor(prop.is_default ?
                SkColorSetRGB(100, 100, 100) : SkColorSetRGB(136, 106, 168));
            name_paint.setAntiAlias(true);
            canvas->drawString(prop.name.c_str(), x + PADDING + 12, y + 13, font, name_paint);

            // 属性值
            float name_width = font.measureText(prop.name.c_str(), prop.name.length(), SkTextEncoding::kUTF8);
            SkPaint value_paint;
            value_paint.setColor(prop.is_default ?
                SkColorSetRGB(100, 100, 100) : SkColorSetRGB(86, 156, 214));
            value_paint.setAntiAlias(true);
            canvas->drawString((": " + prop.value).c_str(), x + PADDING + 12 + name_width, y + 13, font, value_paint);

            y += ROW_HEIGHT;
        }
    }
}

std::shared_ptr<RenderObject> ComputedStylesView::FindRenderObject(std::shared_ptr<Element> element) const {
    if (!element) return nullptr;
    
    // 从 WindowManager 获取渲染树
    auto& wm = WindowManager::Instance();
    auto windows = wm.GetAllWindows();
    if (windows.empty()) return nullptr;
    
    auto window = windows[0];
    auto root_render = window->GetCachedRenderTree();
    if (!root_render) return nullptr;
    
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
    
    return find_render_object(root_render);
}

std::string ComputedStylesView::GetPropertyValue(const ComputedStyle& style, const std::string& property_name) const {
    // Layout 属性
    if (property_name == "display") {
        return RenderObjectTypeToString(style.display);
    }
    if (property_name == "position") {
        return style.position.empty() ? "static" : style.position;
    }
    if (property_name == "top") {
        return CSSLengthToString(style.top);
    }
    if (property_name == "right") {
        return CSSLengthToString(style.right);
    }
    if (property_name == "bottom") {
        return CSSLengthToString(style.bottom);
    }
    if (property_name == "left") {
        return CSSLengthToString(style.left);
    }
    if (property_name == "z-index") {
        return std::to_string(style.z_index);
    }
    if (property_name == "overflow") {
        return style.overflow.empty() ? "visible" : style.overflow;
    }
    if (property_name == "visibility") {
        return style.visibility;
    }
    
    // Box Model 属性
    if (property_name == "width") {
        return CSSLengthToString(style.width);
    }
    if (property_name == "height") {
        return CSSLengthToString(style.height);
    }
    if (property_name == "min-width") {
        return CSSLengthToString(style.min_width);
    }
    if (property_name == "max-width") {
        return CSSLengthToString(style.max_width);
    }
    if (property_name == "min-height") {
        return CSSLengthToString(style.min_height);
    }
    if (property_name == "max-height") {
        return CSSLengthToString(style.max_height);
    }
    if (property_name == "margin-top") {
        // 优先使用 CSSEdges，如果为0则尝试单独字段
        auto val = style.margin.top.value != 0 ? style.margin.top : style.margin_top;
        return CSSLengthToString(val);
    }
    if (property_name == "margin-right") {
        auto val = style.margin.right.value != 0 ? style.margin.right : style.margin_right;
        return CSSLengthToString(val);
    }
    if (property_name == "margin-bottom") {
        auto val = style.margin.bottom.value != 0 ? style.margin.bottom : style.margin_bottom;
        return CSSLengthToString(val);
    }
    if (property_name == "margin-left") {
        auto val = style.margin.left.value != 0 ? style.margin.left : style.margin_left;
        return CSSLengthToString(val);
    }
    if (property_name == "padding-top") {
        auto val = style.padding.top.value != 0 ? style.padding.top : style.padding_top;
        return CSSLengthToString(val);
    }
    if (property_name == "padding-right") {
        auto val = style.padding.right.value != 0 ? style.padding.right : style.padding_right;
        return CSSLengthToString(val);
    }
    if (property_name == "padding-bottom") {
        auto val = style.padding.bottom.value != 0 ? style.padding.bottom : style.padding_bottom;
        return CSSLengthToString(val);
    }
    if (property_name == "padding-left") {
        auto val = style.padding.left.value != 0 ? style.padding.left : style.padding_left;
        return CSSLengthToString(val);
    }
    if (property_name == "border-top-width") {
        return FormatFloat(style.border_top_width) + "px";
    }
    if (property_name == "border-right-width") {
        return FormatFloat(style.border_right_width) + "px";
    }
    if (property_name == "border-bottom-width") {
        return FormatFloat(style.border_bottom_width) + "px";
    }
    if (property_name == "border-left-width") {
        return FormatFloat(style.border_left_width) + "px";
    }
    
    // Typography 属性
    if (property_name == "font-family") {
        return style.font_family.empty() ? "inherit" : style.font_family;
    }
    if (property_name == "font-size") {
        return FormatFloat(style.font_size) + "px";
    }
    if (property_name == "font-weight") {
        return style.font_weight.empty() ? "normal" : style.font_weight;
    }
    if (property_name == "font-style") {
        return style.font_style.empty() ? "normal" : style.font_style;
    }
    if (property_name == "line-height") {
        return FormatFloat(style.line_height);
    }
    if (property_name == "text-align") {
        return style.text_align.empty() ? "start" : style.text_align;
    }
    if (property_name == "text-decoration") {
        return style.text_decoration.empty() ? "none" : style.text_decoration;
    }
    if (property_name == "white-space") {
        return style.white_space;
    }
    
    // Colors 属性
    if (property_name == "color") {
        return style.color.empty() ? "inherit" : style.color;
    }
    if (property_name == "background-color") {
        return style.background_color.empty() ? "transparent" : style.background_color;
    }
    if (property_name == "opacity") {
        return FormatFloat(style.opacity);
    }
    
    // Flexbox 属性
    if (property_name == "flex-direction") {
        return style.flex_direction;
    }
    if (property_name == "flex-wrap") {
        return style.flex_wrap;
    }
    if (property_name == "justify-content") {
        return style.justify_content;
    }
    if (property_name == "align-items") {
        return style.align_items;
    }
    if (property_name == "align-content") {
        return style.align_content;
    }
    if (property_name == "align-self") {
        return style.align_self;
    }
    if (property_name == "flex-grow") {
        return FormatFloat(style.flex_grow);
    }
    if (property_name == "flex-shrink") {
        return FormatFloat(style.flex_shrink);
    }
    if (property_name == "flex-basis") {
        return CSSLengthToString(style.flex_basis);
    }
    if (property_name == "order") {
        return std::to_string(style.order);
    }
    
    // Grid 属性
    if (property_name == "grid-template-columns") {
        return style.grid_template_columns.empty() ? "none" : style.grid_template_columns;
    }
    if (property_name == "grid-template-rows") {
        return style.grid_template_rows.empty() ? "none" : style.grid_template_rows;
    }
    if (property_name == "grid-column-gap") {
        return CSSLengthToString(style.grid_column_gap);
    }
    if (property_name == "grid-row-gap") {
        return CSSLengthToString(style.grid_row_gap);
    }
    if (property_name == "grid-column") {
        return style.grid_column.empty() ? "auto" : style.grid_column;
    }
    if (property_name == "grid-row") {
        return style.grid_row.empty() ? "auto" : style.grid_row;
    }
    
    // Effects 属性
    if (property_name == "box-shadow") {
        return style.box_shadow.empty() ? "none" : "...";  // 简化显示
    }
    if (property_name == "transform") {
        return style.transform_str.empty() ? "none" : style.transform_str;
    }
    
    return "unknown";
}

bool ComputedStylesView::IsDefaultValue(const std::string& property_name, const std::string& value) const {
    auto it = DEFAULT_VALUES.find(property_name);
    if (it == DEFAULT_VALUES.end()) {
        return true;  // 未知属性视为默认值
    }
    
    // 特殊处理空字符串
    if (it->second.empty()) {
        return value.empty() || value == "inherit" || value == "none" || value == "auto" || value == "start" || value == "transparent";
    }
    
    return value == it->second;
}

} // namespace mblink
