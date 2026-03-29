/**
 * @file select_dropdown.cpp
 * @brief Select 下拉菜单管理器实现
 */

#include "select_dropdown.h"
#include "core/render/text/text_renderer.h"
#include "core/render/text/font_manager.h"
#include "core/render/utils/color.h"
#include "core/dom/element.h"
#include "core/dom/text.h"
#include "include/core/SkPaint.h"
#include "include/core/SkPath.h"
#include "include/core/SkRRect.h"
#include "include/core/SkMaskFilter.h"
#include "include/effects/SkBlurMaskFilter.h"

namespace mbink {

SelectDropdownManager& SelectDropdownManager::Instance() {
    static SelectDropdownManager instance;
    return instance;
}

void SelectDropdownManager::OpenDropdown(std::shared_ptr<HTMLSelectElement> select, const SkRect& trigger_rect) {
    if (!select) return;
    
    // 关闭之前的下拉菜单
    if (current_dropdown_.is_open) {
        auto old_select = current_dropdown_.select_element.lock();
        if (old_select) {
            old_select->SetDropdownOpen(false);
        }
    }
    
    current_dropdown_.select_element = select;
    current_dropdown_.trigger_rect = trigger_rect;
    current_dropdown_.is_open = true;
    
    // 计算下拉菜单尺寸（宽度与 select 元素相同）
    auto options = select->GetOptions();
    float dropdown_width = trigger_rect.width();
    float dropdown_height = std::min(options.size() * ITEM_HEIGHT, DROPDOWN_MAX_HEIGHT);
    
    // 考虑 optgroup 标签的额外高度
    const auto& children = select->GetChildNodes();
    int optgroup_count = 0;
    for (const auto& child : children) {
        if (child->GetNodeType() == NodeType::ELEMENT_NODE) {
            auto elem = std::static_pointer_cast<Element>(child);
            if (elem->GetTagName() == "optgroup") {
                optgroup_count++;
            }
        }
    }
    dropdown_height += optgroup_count * ITEM_HEIGHT;
    
    // 下拉菜单位置（在触发器下方）
    float dropdown_x = trigger_rect.left();
    float dropdown_y = trigger_rect.bottom() + 1;
    
    current_dropdown_.dropdown_rect = SkRect::MakeXYWH(
        dropdown_x, dropdown_y, dropdown_width, dropdown_height
    );
    
    // 设置悬停索引为当前选中的索引
    select->SetHoveredIndex(select->GetSelectedIndex());
}

void SelectDropdownManager::CloseDropdown() {
    if (!current_dropdown_.is_open) return;

    auto select = current_dropdown_.select_element.lock();
    if (select) {
        select->SetDropdownOpen(false);
        select->SetHoveredIndex(-1);
    }

    current_dropdown_.is_open = false;
}

std::shared_ptr<HTMLSelectElement> SelectDropdownManager::GetActiveSelect() const {
    return current_dropdown_.select_element.lock();
}

void SelectDropdownManager::UpdatePosition(const SkRect& new_trigger_rect) {
    if (!current_dropdown_.is_open) return;

    // 保持下拉菜单的尺寸不变，只更新位置
    float dropdown_width = current_dropdown_.dropdown_rect.width();
    float dropdown_height = current_dropdown_.dropdown_rect.height();

    current_dropdown_.trigger_rect = new_trigger_rect;

    float dropdown_x = new_trigger_rect.left();
    float dropdown_y = new_trigger_rect.bottom() + 1;

    current_dropdown_.dropdown_rect = SkRect::MakeXYWH(
        dropdown_x, dropdown_y, dropdown_width, dropdown_height
    );
}

void SelectDropdownManager::UpdatePositionFromRenderTree(std::shared_ptr<RenderObject> root_render) {
    if (!current_dropdown_.is_open || !root_render) return;

    auto select = current_dropdown_.select_element.lock();
    if (!select) return;

    // 递归遍历渲染树找到 select 元素对应的 RenderObject
    std::function<std::shared_ptr<RenderObject>(std::shared_ptr<RenderObject>)> findSelectRenderObject;
    findSelectRenderObject = [&](std::shared_ptr<RenderObject> render_obj) -> std::shared_ptr<RenderObject> {
        if (!render_obj) return nullptr;

        auto node = render_obj->GetNode();
        if (node && node.get() == select.get()) {
            return render_obj;
        }

        for (const auto& child : render_obj->GetChildren()) {
            auto result = findSelectRenderObject(child);
            if (result) return result;
        }

        return nullptr;
    };

    auto select_render = findSelectRenderObject(root_render);
    if (!select_render) return;

    // 计算绝对位置（需要考虑滚动偏移）
    float abs_x = 0, abs_y = 0;
    auto current = select_render;
    while (current) {
        const auto& layout = current->GetLayoutInfo();
        abs_x += layout.x;
        abs_y += layout.y;

        // 减去父元素的滚动偏移
        auto parent = current->GetParent();
        if (parent) {
            abs_x -= parent->GetScrollX();
            abs_y -= parent->GetScrollY();
        }

        current = parent;
    }

    const auto& layout = select_render->GetLayoutInfo();
    SkRect new_trigger_rect = SkRect::MakeXYWH(abs_x, abs_y, layout.width, layout.height);

    UpdatePosition(new_trigger_rect);
}

void SelectDropdownManager::Paint(SkCanvas* canvas) {
    if (!current_dropdown_.is_open) return;
    
    auto select = current_dropdown_.select_element.lock();
    if (!select) {
        current_dropdown_.is_open = false;
        return;
    }
    
    const SkRect& rect = current_dropdown_.dropdown_rect;
    
    // 绘制阴影
    SkPaint shadow_paint;
    shadow_paint.setColor(SkColorSetARGB(40, 0, 0, 0));
    shadow_paint.setAntiAlias(true);
    shadow_paint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, DROPDOWN_SHADOW_BLUR));
    canvas->drawRRect(SkRRect::MakeRectXY(rect.makeOffset(0, 2), 4, 4), shadow_paint);
    
    // 绘制背景
    SkPaint bg_paint;
    bg_paint.setColor(SK_ColorWHITE);
    bg_paint.setAntiAlias(true);
    canvas->drawRRect(SkRRect::MakeRectXY(rect, 4, 4), bg_paint);
    
    // 绘制边框
    SkPaint border_paint;
    border_paint.setColor(SkColorSetRGB(200, 200, 200));
    border_paint.setAntiAlias(true);
    border_paint.setStyle(SkPaint::kStroke_Style);
    border_paint.setStrokeWidth(1);
    canvas->drawRRect(SkRRect::MakeRectXY(rect, 4, 4), border_paint);
    
    // 裁剪到下拉菜单区域
    canvas->save();
    canvas->clipRRect(SkRRect::MakeRectXY(rect, 4, 4));
    
    // 获取字体（使用系统默认字体以支持中文）
    FontDescriptor desc;
    desc.family = "Microsoft YaHei, Arial, sans-serif";
    desc.size = 13.0f;
    desc.weight = FontWeight::NORMAL;
    desc.style = FontStyle::NORMAL;
    SkFont font = FontManager::GetInstance().LoadFont(desc);

    // optgroup 标签使用加粗字体
    FontDescriptor bold_desc;
    bold_desc.family = "Microsoft YaHei, Arial, sans-serif";
    bold_desc.size = 13.0f;
    bold_desc.weight = FontWeight::BOLD;
    bold_desc.style = FontStyle::NORMAL;
    SkFont bold_font = FontManager::GetInstance().LoadFont(bold_desc);
    
    SkFontMetrics font_metrics;
    font.getMetrics(&font_metrics);
    
    TextRenderer text_renderer(canvas);
    
    float y = rect.top();
    long item_index = 0;
    long hovered_index = select->GetHoveredIndex();
    long selected_index = select->GetSelectedIndex();
    auto options = select->GetOptions();
    
    // 辅助函数：获取 option 文本
    auto getOptionText = [](const std::shared_ptr<Element>& option) -> std::string {
        const auto& children = option->GetChildNodes();
        for (const auto& child : children) {
            if (child->GetNodeType() == NodeType::TEXT_NODE) {
                auto text = std::static_pointer_cast<Text>(child);
                return text->GetData();
            }
        }
        return "";
    };
    
    // 遍历子元素，支持 optgroup
    const auto& children = select->GetChildNodes();
    for (const auto& child : children) {
        if (child->GetNodeType() != NodeType::ELEMENT_NODE) continue;
        
        auto elem = std::static_pointer_cast<Element>(child);
        std::string tag = elem->GetTagName();
        
        if (tag == "optgroup") {
            // 绘制 optgroup 标签 (Chrome 风格：加粗黑色文字，无背景)
            std::string label = elem->GetAttribute("label");
            if (!label.empty()) {
                // optgroup 文本 - 加粗黑色
                mbink::Paint text_paint;
                text_paint.SetColor(SK_ColorBLACK);
                float text_y = y + (ITEM_HEIGHT - (-font_metrics.fAscent + font_metrics.fDescent)) / 2 - font_metrics.fAscent;
                text_renderer.DrawText(label, rect.left() + ITEM_PADDING_X, text_y, bold_font, text_paint);

                y += ITEM_HEIGHT;
            }
            
            // 绘制 optgroup 内的 options
            const auto& optgroup_children = elem->GetChildNodes();
            for (const auto& opt_child : optgroup_children) {
                if (opt_child->GetNodeType() != NodeType::ELEMENT_NODE) continue;
                
                auto opt_elem = std::static_pointer_cast<Element>(opt_child);
                if (opt_elem->GetTagName() != "option") continue;
                
                // 检查是否在可见区域内
                if (y + ITEM_HEIGHT < rect.top() || y > rect.bottom()) {
                    y += ITEM_HEIGHT;
                    item_index++;
                    continue;
                }
                
                // 绘制选项
                SkRect item_rect = SkRect::MakeXYWH(rect.left(), y, rect.width(), ITEM_HEIGHT);
                
                // 悬停或选中背景
                if (item_index == hovered_index) {
                    SkPaint hover_paint;
                    hover_paint.setColor(SkColorSetRGB(0, 120, 215));  // Windows 蓝色
                    canvas->drawRect(item_rect, hover_paint);
                } else if (item_index == selected_index) {
                    SkPaint selected_paint;
                    selected_paint.setColor(SkColorSetRGB(230, 230, 230));
                    canvas->drawRect(item_rect, selected_paint);
                }
                
                // 选项文本
                std::string option_text = getOptionText(opt_elem);
                bool is_disabled = opt_elem->HasAttribute("disabled");
                
                mbink::Paint text_paint;
                if (item_index == hovered_index) {
                    text_paint.SetColor(SK_ColorWHITE);
                } else if (is_disabled) {
                    text_paint.SetColor(SkColorSetRGB(160, 160, 160));
                } else {
                    text_paint.SetColor(SK_ColorBLACK);
                }
                
                float text_y = y + (ITEM_HEIGHT - (-font_metrics.fAscent + font_metrics.fDescent)) / 2 - font_metrics.fAscent;
                text_renderer.DrawText(option_text, rect.left() + ITEM_PADDING_X + OPTGROUP_INDENT, text_y, font, text_paint);
                
                y += ITEM_HEIGHT;
                item_index++;
            }
        } else if (tag == "option") {
            // 直接的 option（不在 optgroup 内）
            if (y + ITEM_HEIGHT < rect.top() || y > rect.bottom()) {
                y += ITEM_HEIGHT;
                item_index++;
                continue;
            }
            
            SkRect item_rect = SkRect::MakeXYWH(rect.left(), y, rect.width(), ITEM_HEIGHT);
            
            if (item_index == hovered_index) {
                SkPaint hover_paint;
                hover_paint.setColor(SkColorSetRGB(0, 120, 215));
                canvas->drawRect(item_rect, hover_paint);
            } else if (item_index == selected_index) {
                SkPaint selected_paint;
                selected_paint.setColor(SkColorSetRGB(230, 230, 230));
                canvas->drawRect(item_rect, selected_paint);
            }
            
            std::string option_text = getOptionText(elem);
            bool is_disabled = elem->HasAttribute("disabled");
            
            mbink::Paint text_paint;
            if (item_index == hovered_index) {
                text_paint.SetColor(SK_ColorWHITE);
            } else if (is_disabled) {
                text_paint.SetColor(SkColorSetRGB(160, 160, 160));
            } else {
                text_paint.SetColor(SK_ColorBLACK);
            }
            
            float text_y = y + (ITEM_HEIGHT - (-font_metrics.fAscent + font_metrics.fDescent)) / 2 - font_metrics.fAscent;
            text_renderer.DrawText(option_text, rect.left() + ITEM_PADDING_X, text_y, font, text_paint);
            
            y += ITEM_HEIGHT;
            item_index++;
        }
    }
    
    canvas->restore();
}

bool SelectDropdownManager::HandleMouseMove(float x, float y) {
    if (!current_dropdown_.is_open) return false;
    
    auto select = current_dropdown_.select_element.lock();
    if (!select) return false;
    
    if (!current_dropdown_.dropdown_rect.contains(x, y)) {
        return false;
    }
    
    // 计算悬停的索引
    const SkRect& rect = current_dropdown_.dropdown_rect;
    float rel_y = y - rect.top();
    
    // 遍历计算实际的索引（考虑 optgroup）
    float item_y = 0;
    long item_index = 0;
    
    const auto& children = select->GetChildNodes();
    for (const auto& child : children) {
        if (child->GetNodeType() != NodeType::ELEMENT_NODE) continue;
        
        auto elem = std::static_pointer_cast<Element>(child);
        std::string tag = elem->GetTagName();
        
        if (tag == "optgroup") {
            // optgroup 标签
            if (!elem->GetAttribute("label").empty()) {
                if (rel_y >= item_y && rel_y < item_y + ITEM_HEIGHT) {
                    // 悬停在 optgroup 标签上，不选择
                    select->SetHoveredIndex(-1);
                    return true;
                }
                item_y += ITEM_HEIGHT;
            }
            
            const auto& optgroup_children = elem->GetChildNodes();
            for (const auto& opt_child : optgroup_children) {
                if (opt_child->GetNodeType() != NodeType::ELEMENT_NODE) continue;
                
                auto opt_elem = std::static_pointer_cast<Element>(opt_child);
                if (opt_elem->GetTagName() != "option") continue;
                
                if (rel_y >= item_y && rel_y < item_y + ITEM_HEIGHT) {
                    if (!opt_elem->HasAttribute("disabled")) {
                        select->SetHoveredIndex(item_index);
                    }
                    return true;
                }
                item_y += ITEM_HEIGHT;
                item_index++;
            }
        } else if (tag == "option") {
            if (rel_y >= item_y && rel_y < item_y + ITEM_HEIGHT) {
                if (!elem->HasAttribute("disabled")) {
                    select->SetHoveredIndex(item_index);
                }
                return true;
            }
            item_y += ITEM_HEIGHT;
            item_index++;
        }
    }
    
    return true;
}

bool SelectDropdownManager::HandleClick(float x, float y) {
    if (!current_dropdown_.is_open) return false;
    
    auto select = current_dropdown_.select_element.lock();
    if (!select) {
        CloseDropdown();
        return false;
    }
    
    if (current_dropdown_.dropdown_rect.contains(x, y)) {
        // 点击在下拉菜单内，选择当前悬停的选项
        select->SelectHoveredOption();
        CloseDropdown();
        return true;
    }
    
    // 点击在下拉菜单外，关闭
    CloseDropdown();
    return false;
}

bool SelectDropdownManager::HitTest(float x, float y) const {
    if (!current_dropdown_.is_open) return false;
    return current_dropdown_.dropdown_rect.contains(x, y);
}

} // namespace mbink

