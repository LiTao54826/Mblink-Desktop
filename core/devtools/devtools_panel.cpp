/**
 * @file devtools_panel.cpp
 * @brief DevTools 面板容器实现
 */

#include "devtools_panel.h"
#include "inspector/dom_tree_view.h"
#include "styles/styles_panel.h"
#include "search/element_search.h"
#include "core/dom/event.h"

#include "include/core/SkCanvas.h"
#include "include/core/SkPaint.h"
#include "include/core/SkRect.h"
#include "include/core/SkPath.h"

#include <iostream>
#include <algorithm>

namespace mbink {

DevToolsPanel::DevToolsPanel(Document* document)
    : document_(document) {
    dom_tree_view_ = std::make_unique<DOMTreeView>(document);
    styles_panel_ = std::make_unique<StylesPanel>();
    element_search_ = std::make_unique<ElementSearch>(document);

    // 设置选择回调
    dom_tree_view_->SetOnSelectionChanged([this](std::shared_ptr<Node> node) {
        auto element = std::dynamic_pointer_cast<Element>(node);
        if (element) {
            styles_panel_->SetElement(element);
        } else {
        }
    });
}

DevToolsPanel::~DevToolsPanel() = default;

void DevToolsPanel::Refresh() {
    RefreshDOMTree();
    RefreshStyles();
}

void DevToolsPanel::RefreshDOMTree() {
    if (dom_tree_view_ && document_) {
        // 设置根节点为 document 的 body 或 documentElement
        auto body = document_->GetBody();
        if (body) {
            dom_tree_view_->SetRootNode(body);
        } else {
            auto doc_element = document_->GetDocumentElement();
            if (doc_element) {
                dom_tree_view_->SetRootNode(doc_element);
            }
        }
        dom_tree_view_->Refresh();
    }
}

void DevToolsPanel::RefreshAttributes() {
    // 属性显示在 DOM 树视图中，刷新 DOM 树即可
    RefreshDOMTree();
}

void DevToolsPanel::RefreshStyles() {
    if (styles_panel_) {
        styles_panel_->Refresh();
    }
}

void DevToolsPanel::SetSelectedElement(std::shared_ptr<Element> element) {
    if (dom_tree_view_) {
        dom_tree_view_->SelectNode(element);
    }
    if (styles_panel_) {
        styles_panel_->SetElement(element);
    }
}

void DevToolsPanel::Render(SkCanvas* canvas, float x, float y, float width, float height) {
    // 保存位置和尺寸（用于事件处理）
    last_x_ = x;
    last_y_ = y;
    last_width_ = width;
    last_height_ = height;
    
    // 渲染背景
    RenderBackground(canvas, x, y, width, height);

    // 工具栏高度
    const float toolbar_height = 32.0f;
    RenderToolbar(canvas, x, y, width, toolbar_height);

    // 内容区域
    float content_y = y + toolbar_height;
    float content_height = height - toolbar_height;

    // 分隔 DOM 树和样式面板
    float dom_tree_width = width * splitter_position_;
    float styles_width = width - dom_tree_width;

    // 渲染 DOM 树视图
    if (dom_tree_view_) {
        dom_tree_view_->Render(canvas, x, content_y, dom_tree_width, content_height);
    }

    // 渲染分隔线（可拖动区域）
    const float splitter_width = 6.0f;
    SkPaint splitter_paint;
    splitter_paint.setColor(dragging_splitter_ ? SkColorSetRGB(100, 100, 100) : SkColorSetRGB(60, 60, 60));
    canvas->drawRect(SkRect::MakeXYWH(x + dom_tree_width - splitter_width / 2, content_y, splitter_width, content_height), splitter_paint);

    // 渲染样式面板
    if (styles_panel_) {
        styles_panel_->Render(canvas, x + dom_tree_width, content_y, styles_width, content_height);
    }
}

bool DevToolsPanel::HandleEvent(const Event& event) {
    // TODO: 实现事件处理
    return false;
}

bool DevToolsPanel::HandleMouseEvent(int x, int y, int button, bool pressed) {
    const float toolbar_height = 32.0f;
    const float splitter_width = 6.0f;
    
    // 优先处理分隔线拖动结束（无论鼠标在哪里）
    if (!pressed && button == 0 && dragging_splitter_) {
        dragging_splitter_ = false;
        return true;
    }
    
    // 检查是否在工具栏区域
    if (y >= 0 && y < toolbar_height) {
        if (pressed && button == 0) {
            const float btn_size = 24.0f;
            
            // 检查是否点击了元素选择器按钮（工具栏左侧第一个按钮）
            if (x >= 4 && x <= 4 + btn_size) {
                TogglePicker();
                return true;
            }
            
            // 检查是否点击了停靠位置切换按钮（工具栏右侧）
            float dock_btn_x = last_width_ - btn_size - 4;
            if (x >= dock_btn_x && x <= dock_btn_x + btn_size) {
                if (on_dock_toggled_) {
                    on_dock_toggled_();
                }
                return true;
            }
        }
        return true;
    }
    
    // 内容区域
    float content_y = toolbar_height;
    float dom_tree_width = last_width_ * splitter_position_;
    
    // 检查是否在分隔线区域
    if (IsOnSplitter(x)) {
        if (pressed && button == 0) {
            dragging_splitter_ = true;
        }
        return true;
    }
    
    // 检查是否在 DOM 树区域
    if (x < dom_tree_width - splitter_width / 2) {
        if (dom_tree_view_) {
            return dom_tree_view_->HandleMouseEvent(x, y - static_cast<int>(content_y), button, pressed);
        }
    } else if (x > dom_tree_width + splitter_width / 2) {
        // 在样式面板区域
        if (styles_panel_) {
            return styles_panel_->HandleMouseEvent(
                x - static_cast<int>(dom_tree_width),
                y - static_cast<int>(content_y),
                button, pressed
            );
        }
    }
    
    return false;
}

bool DevToolsPanel::IsOnSplitter(int x) const {
    float dom_tree_width = last_width_ * splitter_position_;
    const float splitter_width = 6.0f;
    return x >= dom_tree_width - splitter_width / 2 && x <= dom_tree_width + splitter_width / 2;
}

bool DevToolsPanel::IsMouseOnSplitter(int x, int y) const {
    const float toolbar_height = 32.0f;
    // 只有在工具栏下方才检查分割线
    if (y < toolbar_height) {
        return false;
    }
    return IsOnSplitter(x);
}

bool DevToolsPanel::HandleMouseMove(int x, int y) {
    const float toolbar_height = 32.0f;
    
    // 处理分隔线拖动
    if (dragging_splitter_) {
        float new_position = static_cast<float>(x) / last_width_;
        // 限制分隔线位置在 20% - 80% 之间
        new_position = std::max(0.2f, std::min(0.8f, new_position));
        if (new_position != splitter_position_) {
            splitter_position_ = new_position;
            return true;
        }
        return false;
    }
    
    // 内容区域
    float content_y = toolbar_height;
    float dom_tree_width = last_width_ * splitter_position_;
    
    // 检查是否在样式面板区域
    if (y >= toolbar_height && x >= dom_tree_width && styles_panel_) {
        return styles_panel_->HandleMouseMove(
            x - static_cast<int>(dom_tree_width),
            y - static_cast<int>(content_y)
        );
    }
    
    // 鼠标不在样式面板区域时，清除 Box Model 悬停状态
    if (styles_panel_) {
        styles_panel_->ClearBoxModelHover();
    }
    
    return false;
}

void DevToolsPanel::SetOnBoxModelHover(BoxModelHoverCallback callback) {
    if (styles_panel_) {
        styles_panel_->SetOnBoxModelHover(callback);
    }
}

void DevToolsPanel::ClearBoxModelHover() {
    if (styles_panel_) {
        styles_panel_->ClearBoxModelHover();
    }
}

bool DevToolsPanel::HandleMouseWheel(int x, int y, float delta_x, float delta_y) {
    const float toolbar_height = 32.0f;
    
    // 检查是否在工具栏区域
    if (y < toolbar_height) {
        return false;
    }
    
    float dom_tree_width = last_width_ * splitter_position_;
    
    // 检查是否在 DOM 树区域
    if (x < dom_tree_width && dom_tree_view_) {
        return dom_tree_view_->HandleMouseWheel(delta_y);
    }
    
    // 检查是否在样式面板区域
    if (x >= dom_tree_width && styles_panel_) {
        return styles_panel_->HandleMouseWheel(delta_y);
    }
    
    return false;
}

void DevToolsPanel::TogglePicker() {
    picker_active_ = !picker_active_;
    if (on_picker_toggled_) {
        on_picker_toggled_(picker_active_);
    }
}

void DevToolsPanel::RenderBackground(SkCanvas* canvas, float x, float y, float width, float height) {
    SkPaint paint;
    paint.setColor(SkColorSetRGB(36, 36, 36));  // 深色背景
    canvas->drawRect(SkRect::MakeXYWH(x, y, width, height), paint);

    // 顶部边框
    paint.setColor(SkColorSetRGB(60, 60, 60));
    canvas->drawRect(SkRect::MakeXYWH(x, y, width, 1), paint);
}

void DevToolsPanel::RenderToolbar(SkCanvas* canvas, float x, float y, float width, float height) {
    SkPaint paint;
    paint.setColor(SkColorSetRGB(45, 45, 45));
    canvas->drawRect(SkRect::MakeXYWH(x, y, width, height), paint);

    // 底部边框
    paint.setColor(SkColorSetRGB(60, 60, 60));
    canvas->drawRect(SkRect::MakeXYWH(x, y + height - 1, width, 1), paint);

    float btn_y = y + 4;
    float btn_size = 24;

    // 元素选择器按钮（鼠标图标）- 左侧第一个
    float picker_btn_x = x + 4;
    
    SkPaint btn_paint;
    if (picker_active_) {
        btn_paint.setColor(SkColorSetRGB(66, 133, 244));  // 激活时蓝色
    } else {
        btn_paint.setColor(SkColorSetRGB(60, 60, 60));
    }
    canvas->drawRoundRect(SkRect::MakeXYWH(picker_btn_x, btn_y, btn_size, btn_size), 3, 3, btn_paint);
    
    // 绘制鼠标指针图标
    SkPaint icon_paint;
    icon_paint.setColor(picker_active_ ? SK_ColorWHITE : SkColorSetRGB(180, 180, 180));
    icon_paint.setAntiAlias(true);
    icon_paint.setStyle(SkPaint::kStroke_Style);
    icon_paint.setStrokeWidth(1.5f);
    
    float cx = picker_btn_x + btn_size / 2;
    float cy = btn_y + btn_size / 2;
    
    // 简单的鼠标指针形状
    SkPath path;
    path.moveTo(cx - 5, cy - 6);
    path.lineTo(cx - 5, cy + 6);
    path.lineTo(cx - 1, cy + 3);
    path.lineTo(cx + 3, cy + 7);
    path.moveTo(cx - 1, cy + 3);
    path.lineTo(cx + 5, cy - 3);
    canvas->drawPath(path, icon_paint);

    // 停靠位置切换按钮 - 右侧
    float dock_btn_x = x + width - btn_size - 4;
    
    SkPaint dock_btn_paint;
    dock_btn_paint.setColor(SkColorSetRGB(60, 60, 60));
    canvas->drawRoundRect(SkRect::MakeXYWH(dock_btn_x, btn_y, btn_size, btn_size), 3, 3, dock_btn_paint);
    
    // 绘制停靠位置图标
    SkPaint dock_icon_paint;
    dock_icon_paint.setColor(SkColorSetRGB(180, 180, 180));
    dock_icon_paint.setAntiAlias(true);
    dock_icon_paint.setStyle(SkPaint::kStroke_Style);
    dock_icon_paint.setStrokeWidth(1.5f);
    
    float dcx = dock_btn_x + btn_size / 2;
    float dcy = btn_y + btn_size / 2;
    
    // 绘制窗口图标（矩形框）
    SkRect window_rect = SkRect::MakeXYWH(dcx - 7, dcy - 5, 14, 10);
    canvas->drawRect(window_rect, dock_icon_paint);
    
    // 绘制分割线（根据当前停靠位置显示不同方向）
    dock_icon_paint.setStyle(SkPaint::kFill_Style);
    if (is_dock_bottom_) {
        // 当前底部停靠，显示右侧停靠图标（垂直分割线在右边）
        canvas->drawRect(SkRect::MakeXYWH(dcx + 3, dcy - 5, 4, 10), dock_icon_paint);
    } else {
        // 当前右侧停靠，显示底部停靠图标（水平分割线在下边）
        canvas->drawRect(SkRect::MakeXYWH(dcx - 7, dcy + 1, 14, 4), dock_icon_paint);
    }
}

} // namespace mbink
