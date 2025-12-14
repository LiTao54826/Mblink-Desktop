/**
 * @file styles_panel.cpp
 * @brief 样式面板容器实现
 */

#include "styles_panel.h"
#include "inline_styles_view.h"
#include "box_model_view.h"
#include "core/dom/event.h"
#include "core/render/text/font_manager.h"

#include "include/core/SkCanvas.h"
#include "include/core/SkPaint.h"
#include "include/core/SkRect.h"
#include "include/core/SkFont.h"

#include <algorithm>

namespace lightui {

namespace {
    const float TAB_HEIGHT = 28.0f;
}

StylesPanel::StylesPanel() {
    inline_styles_view_ = std::make_unique<InlineStylesView>();
    box_model_view_ = std::make_unique<BoxModelView>();
}

void StylesPanel::SetOnBoxModelHover(BoxModelHoverCallback callback) {
    on_box_model_hover_ = callback;
    
    // 设置 BoxModelView 的回调
    if (box_model_view_) {
        box_model_view_->SetOnHoverChanged([this](std::shared_ptr<Element> element, BoxAreaType area) {
            if (on_box_model_hover_) {
                on_box_model_hover_(element, static_cast<int>(area));
            }
        });
    }
}

StylesPanel::~StylesPanel() = default;

void StylesPanel::SetElement(std::shared_ptr<Element> element) {
    element_ = element;

    if (inline_styles_view_) {
        inline_styles_view_->SetElement(element);
    }
    if (box_model_view_) {
        box_model_view_->SetElement(element);
    }
}

void StylesPanel::Refresh() {
    auto element = element_.lock();
    if (!element) {
        return;
    }

    // 重新设置元素以刷新所有子视图
    if (inline_styles_view_) {
        inline_styles_view_->SetElement(element);
    }
    if (box_model_view_) {
        box_model_view_->SetElement(element);
    }
}

void StylesPanel::Render(SkCanvas* canvas, float x, float y, float width, float height) {
    // 保存位置和尺寸
    last_x_ = x;
    last_y_ = y;
    last_width_ = width;
    last_height_ = height;
    
    // 背景
    SkPaint bg_paint;
    bg_paint.setColor(SkColorSetRGB(36, 36, 36));
    canvas->drawRect(SkRect::MakeXYWH(x, y, width, height), bg_paint);

    // 渲染标签页
    RenderTabs(canvas, x, y, width, TAB_HEIGHT);

    // 内容区域
    float content_y = y + TAB_HEIGHT;
    float content_height = height - TAB_HEIGHT;

    // 根据活动标签页渲染内容
    switch (active_tab_) {
        case StylesTab::Styles:
            if (inline_styles_view_) {
                // 应用裁剪区域
                canvas->save();
                canvas->clipRect(SkRect::MakeXYWH(x, content_y, width, content_height));
                
                // 应用滚动偏移
                inline_styles_view_->Render(canvas, x, content_y - scroll_offset_, width, content_height + scroll_offset_);
                
                // 获取内容高度用于滚动计算
                content_height_ = inline_styles_view_->GetContentHeight();
                
                canvas->restore();
            }
            break;
        case StylesTab::BoxModel:
            if (box_model_view_) {
                box_model_view_->Render(canvas, x, content_y, width, content_height);
            }
            // BoxModel 不需要滚动，重置滚动偏移
            scroll_offset_ = 0;
            break;
    }
}

bool StylesPanel::HandleEvent(const Event& event) {
    // TODO: 实现事件处理（标签页切换等）
    return false;
}

bool StylesPanel::HandleMouseEvent(int x, int y, int button, bool pressed) {
    if (!pressed || button != 0) {
        return false;
    }
    
    // 检查是否点击了标签页
    if (y < TAB_HEIGHT) {
        float tab_width = last_width_ / 2;  // 只有2个标签页
        int tab_index = static_cast<int>(x / tab_width);
        if (tab_index >= 0 && tab_index < 2) {
            active_tab_ = static_cast<StylesTab>(tab_index);
            // 切换标签页时重置 BoxModelView 的悬停状态
            if (box_model_view_) {
                box_model_view_->ResetHover();
            }
            return true;
        }
    }
    
    return false;
}

bool StylesPanel::HandleMouseMove(int x, int y) {
    // 只有在 BoxModel 标签页激活时才处理鼠标移动
    if (active_tab_ != StylesTab::BoxModel || !box_model_view_) {
        return false;
    }
    
    // x, y 已经是相对于 styles panel 的坐标（由 DevToolsPanel 转换）
    // 只需要减去标签页高度
    int rel_y = y - static_cast<int>(TAB_HEIGHT);
    
    // 传递给 BoxModelView 处理
    return box_model_view_->HandleMouseMove(x, rel_y);
}

bool StylesPanel::HandleMouseWheel(float delta_y) {
    // 只有在 Styles 标签页激活时才处理滚轮
    if (active_tab_ != StylesTab::Styles) {
        return false;
    }
    
    const float scroll_speed = 30.0f;
    float new_offset = scroll_offset_ - delta_y * scroll_speed;
    
    // 限制滚动范围
    float content_height = last_height_ - TAB_HEIGHT;
    float max_scroll = std::max(0.0f, content_height_ - content_height);
    new_offset = std::max(0.0f, std::min(max_scroll, new_offset));
    
    if (new_offset != scroll_offset_) {
        scroll_offset_ = new_offset;
        return true;
    }
    return false;
}

void StylesPanel::ClearBoxModelHover() {
    if (box_model_view_) {
        box_model_view_->ResetHover();
    }
    // 通知回调清除高亮
    if (on_box_model_hover_) {
        on_box_model_hover_(nullptr, 0);
    }
}

void StylesPanel::RenderTabs(SkCanvas* canvas, float x, float y, float width, float height) {
    const char* tab_names[] = { "Styles", "Box Model" };
    const int tab_count = 2;
    float tab_width = width / tab_count;

    // 使用 FontManager 获取字体
    FontDescriptor font_desc;
    font_desc.family = "Arial";
    font_desc.size = 11.0f;
    font_desc.weight = FontWeight::NORMAL;
    font_desc.style = FontStyle::NORMAL;
    SkFont font = FontManager::GetInstance().LoadFont(font_desc);

    for (int i = 0; i < tab_count; ++i) {
        float tab_x = x + i * tab_width;
        bool is_active = (static_cast<int>(active_tab_) == i);

        // 标签背景
        SkPaint tab_paint;
        if (is_active) {
            tab_paint.setColor(SkColorSetRGB(36, 36, 36));
        } else {
            tab_paint.setColor(SkColorSetRGB(45, 45, 45));
        }
        canvas->drawRect(SkRect::MakeXYWH(tab_x, y, tab_width, height), tab_paint);

        // 活动标签底部高亮
        if (is_active) {
            SkPaint highlight_paint;
            highlight_paint.setColor(SkColorSetRGB(66, 133, 244));
            canvas->drawRect(SkRect::MakeXYWH(tab_x, y + height - 2, tab_width, 2), highlight_paint);
        }

        // 标签文本
        SkPaint text_paint;
        text_paint.setColor(is_active ? SkColorSetRGB(200, 200, 200) : SkColorSetRGB(128, 128, 128));
        text_paint.setAntiAlias(true);

        float text_width = font.measureText(tab_names[i], strlen(tab_names[i]), SkTextEncoding::kUTF8);
        float text_x = tab_x + (tab_width - text_width) / 2;
        float text_y = y + height / 2 + 4;

        canvas->drawString(tab_names[i], text_x, text_y, font, text_paint);

        // 分隔线
        if (i < tab_count - 1) {
            SkPaint sep_paint;
            sep_paint.setColor(SkColorSetRGB(60, 60, 60));
            canvas->drawRect(SkRect::MakeXYWH(tab_x + tab_width - 1, y + 4, 1, height - 8), sep_paint);
        }
    }

    // 底部边框
    SkPaint border_paint;
    border_paint.setColor(SkColorSetRGB(60, 60, 60));
    canvas->drawRect(SkRect::MakeXYWH(x, y + height - 1, width, 1), border_paint);
}

} // namespace lightui
