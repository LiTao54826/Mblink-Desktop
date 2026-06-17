/**
 * @file wheel_event_dispatcher.cpp
 * @brief 滚轮事件分发器实现
 *
 * 从 event_loop.cpp 提取的滚轮事件处理逻辑。
 */

#include "wheel_event_dispatcher.h"

#include "core/devtools/devtools_bridge.h"
#include "core/dom/document.h"
#include "core/dom/element.h"
#include "core/dom/elements/html_textarea_element.h"
#include "core/dom/elements/terminal/html_terminal_element.h"
#include "core/dom/elements/logview/html_logview_element.h"
#include "core/event/input/hit_test_controller.h"
#include "core/render/objects/select_dropdown.h"
#include "core/render/objects/render_object.h"
#include "core/render/pipeline/render_pipeline.h"
#include "core/render/text/font_manager.h"
#include "core/window/window.h"

#include "include/core/SkFont.h"
#include "include/core/SkFontMetrics.h"

#include <algorithm>
#include <vector>

namespace mbink {

WheelEventDispatcher::WheelEventDispatcher() = default;

WheelEventDispatcher::~WheelEventDispatcher() = default;

namespace {

constexpr int kTopLayerZIndexThreshold = 100;
constexpr float kViewportCoverTolerance = 1.0f;

bool CoversViewport(const std::shared_ptr<RenderObject>& render_obj) {
    if (!render_obj) {
        return false;
    }

    if (!render_obj->GetViewportBounds().valid) {
        render_obj->UpdateViewportBounds();
    }

    const auto& bounds = render_obj->GetViewportBounds();
    const float viewport_width = RenderObject::GetViewportWidth();
    const float viewport_height = RenderObject::GetViewportHeight();
    if (!bounds.valid || viewport_width <= 0.0f || viewport_height <= 0.0f) {
        return false;
    }

    return bounds.x <= kViewportCoverTolerance &&
           bounds.y <= kViewportCoverTolerance &&
           bounds.x + bounds.width >= viewport_width - kViewportCoverTolerance &&
           bounds.y + bounds.height >= viewport_height - kViewportCoverTolerance;
}

bool IsFixedTopLayerScrollBoundary(const std::shared_ptr<RenderObject>& render_obj) {
    if (!render_obj) {
        return false;
    }

    const auto& style = render_obj->GetComputedStyle();
    return style.position == "fixed" &&
           style.z_index >= kTopLayerZIndexThreshold &&
           style.pointer_events != "none" &&
           CoversViewport(render_obj);
}

bool IsPositionedTopLayer(const std::shared_ptr<RenderObject>& render_obj) {
    if (!render_obj) {
        return false;
    }

    const auto& style = render_obj->GetComputedStyle();
    return (style.position == "fixed" || style.position == "absolute") &&
           style.z_index >= kTopLayerZIndexThreshold &&
           style.pointer_events != "none";
}

bool TreeHasFixedTopLayerScrollBoundary(const std::shared_ptr<RenderObject>& render_obj) {
    if (!render_obj) {
        return false;
    }

    if (IsFixedTopLayerScrollBoundary(render_obj)) {
        return true;
    }

    for (const auto& child : render_obj->GetChildren()) {
        if (TreeHasFixedTopLayerScrollBoundary(child)) {
            return true;
        }
    }

    return false;
}

}  // namespace

bool WheelEventDispatcher::HandleWheelEvent(const SDL_Event& event,
                                             std::shared_ptr<Window> window,
                                             std::shared_ptr<Document> document) {
    if (!window || !document) {
        return false;
    }

    if (event.type != SDL_EVENT_MOUSE_WHEEL) {
        return false;
    }

    float mouse_x = event.wheel.mouse_x;
    float mouse_y = event.wheel.mouse_y;
    float wheel_x = event.wheel.x;
    float wheel_y = event.wheel.y;

    // 将物理像素坐标转换为逻辑像素坐标
    float dpi_scale = window->GetDisplayScale();
    float logical_x = mouse_x / dpi_scale;
    float logical_y = mouse_y / dpi_scale;

    // ===== 优先处理 DevTools 面板的滚轮事件 =====
    if (DevToolsIsOpen()) {
        // 获取窗口尺寸
        int win_width, win_height;
        SDL_GetWindowSize(window->GetSDLWindow(), &win_width, &win_height);
        float width = static_cast<float>(win_width) / dpi_scale;
        float height = static_cast<float>(win_height) / dpi_scale;
        
        // 获取 DevTools 面板区域
        float panel_x, panel_y, panel_width, panel_height;
        const auto panel_bounds = DevToolsGetPanelBounds(width, height);
        panel_x = panel_bounds.x;
        panel_y = panel_bounds.y;
        panel_width = panel_bounds.width;
        panel_height = panel_bounds.height;
        
        // 检查鼠标是否在 DevTools 面板区域内
        bool in_panel = (logical_x >= panel_x && logical_x < panel_x + panel_width &&
                         logical_y >= panel_y && logical_y < panel_y + panel_height);
        
        if (in_panel) {
            // 将滚轮事件传递给 DevTools
            int rel_x = static_cast<int>(logical_x - panel_x);
            int rel_y = static_cast<int>(logical_y - panel_y);
            if (DevToolsHandleMouseWheel(rel_x, rel_y, wheel_x, wheel_y)) {
                window->SetNeedsRepaintFor(RepaintReason::DevTools);
                return true;  // 事件被 DevTools 消费
            }
        }
    }

    // ===== 优先处理 Select 下拉菜单滚轮 =====
    auto& dropdown_manager = SelectDropdownManager::Instance();
    if (dropdown_manager.IsDropdownOpen() && dropdown_manager.HitTest(logical_x, logical_y)) {
        if (dropdown_manager.HandleWheel(wheel_y)) {
            window->SetNeedsRepaintFor(RepaintReason::WheelScroll);
            if (auto pipeline = window->GetRenderPipeline()) {
                pipeline->ForceRasterize();
            }
            return true;
        }
        return true;
    }

    // 检查是否按住 Shift 键
    SDL_Keymod mod_state = SDL_GetModState();
    bool shift_pressed = (mod_state & SDL_KMOD_SHIFT) != 0;

    // 使用缓存的渲染树进行 Hit Testing
    window->EnsureRenderTree();
    auto root_render = window->GetCachedRenderTree();
    if (!root_render) {
        return false;
    }

    HitTestController hit_controller;
    HitTestRequest request;
    auto result_ex = hit_controller.HitTest(root_render, logical_x, logical_y, request);
    
    HitTestResult hit_result;
    if (result_ex.IsValid()) {
        hit_result.element = result_ex.element;
        hit_result.render_object = result_ex.render_object;
        hit_result.local_x = result_ex.local_x;
        hit_result.local_y = result_ex.local_y;
    }

    // 检查是否命中了特殊元素
    if (hit_result.IsValid() && hit_result.element) {
        std::string tag_name = hit_result.element->GetTagName();
        
        // 处理 terminal 元素的滚轮事件
        if (tag_name == "terminal") {
            if (HandleTerminalWheel(window, hit_result.element, wheel_x, wheel_y,
                                    shift_pressed)) {
                return true;
            }
        }
        
        // 处理 logview 元素的滚轮事件
        if (tag_name == "logview") {
            if (HandleLogViewWheel(window, hit_result.element, wheel_x, wheel_y,
                                   shift_pressed)) {
                return true;
            }
        }
        
        // 处理 textarea 元素的滚轮事件
        if (tag_name == "textarea") {
            if (HandleTextAreaWheel(window, hit_result.element, hit_result.render_object,
                                    wheel_x, wheel_y, shift_pressed)) {
                return true;
            }
            // 如果 textarea 不能滚动，让事件穿透到父元素
        }
    }

    // 从命中的元素向上遍历，找到第一个可滚动的元素
    auto render_obj = hit_result.render_object;
    bool top_layer_boundary_known = false;
    bool has_fixed_top_layer_boundary = false;
    
    while (render_obj) {
        if (HandleScrollableElementWheel(window, render_obj, logical_x, logical_y,
                                         wheel_x, wheel_y, shift_pressed)) {
            return true;
        }
        if (IsFixedTopLayerScrollBoundary(render_obj)) {
            return true;
        }
        if (IsPositionedTopLayer(render_obj)) {
            if (!top_layer_boundary_known) {
                has_fixed_top_layer_boundary = TreeHasFixedTopLayerScrollBoundary(root_render);
                top_layer_boundary_known = true;
            }
            if (has_fixed_top_layer_boundary) {
                return true;
            }
        }
        render_obj = render_obj->GetParent();
    }

    return false;
}

bool WheelEventDispatcher::HandleTerminalWheel(std::shared_ptr<Window> window,
                                                std::shared_ptr<Element> element,
                                                float wheel_x, float wheel_y,
                                                bool shift_pressed) {
    if (!element) {
        return false;
    }

    auto terminal_element = std::dynamic_pointer_cast<HTMLTerminalElement>(element);
    if (!terminal_element) {
        return false;
    }

    bool horizontal = (wheel_x != 0.0f) || shift_pressed;
    float delta = horizontal && wheel_x != 0.0f ? -wheel_x * 40.0f : -wheel_y * 40.0f;
    terminal_element->HandleWheel(delta, horizontal);

    // 标记窗口需要重绘
    window->SetNeedsRepaintFor(RepaintReason::WheelScroll);
    if (auto pipeline = window->GetRenderPipeline()) {
        pipeline->ForceRasterize();
    }

    return true;
}

bool WheelEventDispatcher::HandleLogViewWheel(std::shared_ptr<Window> window,
                                               std::shared_ptr<Element> element,
                                               float wheel_x, float wheel_y,
                                               bool shift_pressed) {
    if (!element) {
        return false;
    }

    auto logview_element = std::dynamic_pointer_cast<HTMLLogViewElement>(element);
    if (!logview_element) {
        return false;
    }

    bool horizontal = (wheel_x != 0.0f) || shift_pressed;
    float delta = horizontal && wheel_x != 0.0f ? -wheel_x * 40.0f : -wheel_y * 40.0f;
    logview_element->OnWheel(delta, horizontal);

    // 标记窗口需要重绘
    window->SetNeedsRepaintFor(RepaintReason::WheelScroll);
    if (auto pipeline = window->GetRenderPipeline()) {
        pipeline->ForceRasterize();
    }

    return true;
}

bool WheelEventDispatcher::HandleTextAreaWheel(std::shared_ptr<Window> window,
                                                std::shared_ptr<Element> element,
                                                std::shared_ptr<RenderObject> render_object,
                                                float wheel_x, float wheel_y,
                                                bool shift_pressed) {
    if (!render_object || !element) {
        return false;
    }

    auto textarea_element = std::dynamic_pointer_cast<HTMLTextAreaElement>(element);
    if (!textarea_element) {
        return false;
    }

    const auto& style = render_object->GetComputedStyle();
    float padding_top = style.padding.top.ToPx();
    float padding_bottom = style.padding.bottom.ToPx();
    float padding_left = style.padding.left.ToPx();
    float padding_right = style.padding.right.ToPx();
    const auto& layout = render_object->GetLayoutInfo();
    float visible_height = layout.height - padding_top - padding_bottom;
    float visible_width = layout.width - padding_left - padding_right;

    // 计算行高（与渲染保持一致）
    float font_size = style.font_size > 0 ? style.font_size : 14.0f;
    FontDescriptor desc;
    desc.family = style.font_family.empty() ? "sans-serif" : style.font_family;
    desc.size = font_size;
    desc.weight = FontWeight::NORMAL;
    desc.style = FontStyle::NORMAL;
    SkFont font = FontManager::GetInstance().LoadFont(desc);
    SkFontMetrics font_metrics;
    font.getMetrics(&font_metrics);
    float line_height = -font_metrics.fAscent + font_metrics.fDescent;
    if (font_metrics.fLeading > 0) {
        line_height += font_metrics.fLeading;
    } else {
        line_height += font_size * 0.2f;
    }

    // 检查 textarea 是否真的需要滚动（内容是否超出可见区域）
    float content_height = textarea_element->GetContentHeight(line_height);
    float max_scroll = std::max(0.0f, content_height - visible_height);
    float current_scroll = textarea_element->GetScrollTop();
    
    // 判断滚动方向和是否可以滚动
    bool scrolling_down = wheel_y < 0;
    bool scrolling_up = wheel_y > 0;
    bool can_scroll_down = current_scroll < max_scroll - 0.1f;
    bool can_scroll_up = current_scroll > 0.1f;
    
    // 如果 textarea 可以在当前方向滚动，则处理滚动
    if ((scrolling_down && can_scroll_down) || (scrolling_up && can_scroll_up)) {
        // 处理滚轮事件
        if (shift_pressed) {
            // Shift+滚轮：横向滚动
            textarea_element->HandleMouseWheelHorizontal(-wheel_y, visible_width, font);
        } else {
            // 普通滚轮：垂直滚动
            textarea_element->HandleMouseWheel(-wheel_y, line_height, visible_height);
        }

        // 标记窗口需要重绘
        window->SetNeedsRepaintFor(RepaintReason::WheelScroll);
        return true;
    }

    // textarea 不能滚动
    (void)wheel_x;  // 未使用
    return false;
}

bool WheelEventDispatcher::HandleScrollableElementWheel(
    std::shared_ptr<Window> window,
    std::shared_ptr<RenderObject> render_obj,
    float logical_x, float logical_y,
    float wheel_x, float wheel_y,
    bool shift_pressed) {
    
    if (!render_obj) {
        return false;
    }

    const auto& style = render_obj->GetComputedStyle();

    // 获取独立的 overflow-x 和 overflow-y 值
    std::string overflow_x = !style.overflow_x.empty() ? style.overflow_x : style.overflow;
    std::string overflow_y = !style.overflow_y.empty() ? style.overflow_y : style.overflow;

    bool allow_h_scroll = (overflow_x == "scroll" || overflow_x == "auto");
    bool allow_v_scroll = (overflow_y == "scroll" || overflow_y == "auto");

    // 检查是否可滚动
    if (!allow_h_scroll && !allow_v_scroll) {
        return false;
    }

    // 检查内容是否真的需要滚动（内容是否超出可见区域）
    float max_scroll_x = render_obj->GetMaxScrollX();
    float max_scroll_y = render_obj->GetMaxScrollY();
    bool can_scroll_h = allow_h_scroll && max_scroll_x > 0;
    bool can_scroll_v = allow_v_scroll && max_scroll_y > 0;

    // 计算滚动量（负值向下滚动，正值向上滚动，所以要取反）
    // 每行滚动 40 像素（类似浏览器的默认行为）
    float scroll_delta_x = -wheel_x * 40.0f;
    float scroll_delta_y = -wheel_y * 40.0f;

    // 检查滚动方向是否与元素实际可滚动的方向匹配
    bool wants_v_scroll = (wheel_y != 0);
    bool wants_h_scroll = (wheel_x != 0) || shift_pressed;

    // 如果滚动方向与元素实际可滚动的方向不匹配，继续向上查找
    if ((wants_v_scroll && !can_scroll_v && !wants_h_scroll) ||
        (wants_h_scroll && !can_scroll_h && !wants_v_scroll)) {
        return false;
    }

    // 如果元素完全不能滚动，穿透到父级
    if (!can_scroll_h && !can_scroll_v) {
        return false;
    }

    // 计算元素的绝对位置，用于检测鼠标是否在滚动条上
    float elem_abs_x = 0, elem_abs_y = 0;
    std::vector<std::shared_ptr<RenderObject>> ancestors;
    auto current = render_obj;
    while (current) {
        ancestors.push_back(current);
        current = current->GetParent();
    }
    for (auto it = ancestors.rbegin(); it != ancestors.rend(); ++it) {
        const auto& l = (*it)->GetLayoutInfo();
        elem_abs_x += l.x;
        elem_abs_y += l.y;
    }

    float element_local_x = logical_x - elem_abs_x;
    float element_local_y = logical_y - elem_abs_y;
    auto scrollbar_area = render_obj->HitTestScrollbar(element_local_x, element_local_y);

    // 如果鼠标在水平滚动条上，或者按住 Shift 键，将垂直滚动转换为水平滚动
    bool use_horizontal_scroll = (scrollbar_area == RenderObject::ScrollbarHitArea::HorizontalTrack) ||
                                 (shift_pressed && wheel_y != 0 && wheel_x == 0);

    if (use_horizontal_scroll) {
        scroll_delta_x = -wheel_y * 40.0f;
        scroll_delta_y = 0;
    }

    // 应用滚动
    auto render_pipeline = window->GetRenderPipeline();
    bool scrolled = false;
    
    if (render_pipeline) {
        scrolled = render_pipeline->HandleScroll(render_obj.get(), scroll_delta_x, scroll_delta_y);
    } else {
        render_obj->ScrollBy(scroll_delta_x, scroll_delta_y);
        scrolled = true;
    }
    
    if (!scrolled) {
        return false;
    }

    // 标记窗口需要重绘
    bool is_body_scroll = render_obj->IsBodyElement();
    
    if (is_body_scroll) {
        window->SetForceFullRepaint(true);
    } else {
        SkRect scroll_bounds = render_obj->GetViewportBoundingRect();
        if (!scroll_bounds.isEmpty()) {
            window->AddDirtyRect(scroll_bounds);
        }
        window->SetForceFullRepaint(true);
    }
    window->SetNeedsRepaintFor(RepaintReason::WheelScroll);

    return true;
}

} // namespace mbink
