/**
 * @file incremental_render_test.cpp
 * @brief 增量渲染可视化测试
 *
 * 这个测试程序可以直观地验证增量渲染是否正常工作：
 * - 点击 "Add" 按钮添加元素
 * - 点击 "Remove" 按钮删除元素
 * - 观察 UI 是否正确更新
 */

#include <iostream>
#include <string>
#include <memory>
#include <chrono>

#include "core/window/window.h"
#include "core/dom/document.h"
#include "core/dom/element.h"
#include "core/dom/text.h"
#include "core/event/event_loop.h"
#include "core/render/render_object.h"  // For paint stats
#include "core/render/render_object.h"

using namespace lightui;

class IncrementalRenderTest {
public:
    IncrementalRenderTest() : item_count_(0), box_x_(50), box_y_(50), popup_visible_(false) {}

    bool Initialize() {
        std::cout << "=== Incremental Render Visual Test ===" << std::endl;

        // 创建窗口
        WindowConfig config;
        config.title = "Incremental Render Test";
        config.width = 600;
        config.height = 500;
        config.hidden = false;

        window_ = std::make_unique<Window>(config);
        if (!window_) {
            std::cerr << "Failed to create window" << std::endl;
            return false;
        }

        // 创建文档
        doc_ = std::make_shared<Document>();
        doc_->Initialize();
        window_->SetDocument(doc_);

        // 创建 UI
        CreateUI();

        std::cout << "Initialization complete!" << std::endl;
        std::cout << "Click 'Add Item' to add elements" << std::endl;
        std::cout << "Click 'Remove Item' to remove elements" << std::endl;
        std::cout << "Watch the UI update!" << std::endl;

        return true;
    }

    void Run() {
        // 使用简单的事件循环
        bool running = true;
        auto last_render = std::chrono::steady_clock::now();

        while (running && !window_->ShouldClose()) {
            // 处理事件
            SDL_Event event;
            // 1. 阻塞等待事件（带超时，用于保持渲染帧率）
            // 如果有动画或需要持续渲染，这里应该设为较小的值（如16ms）
            // 如果完全静止，可以设长一点或者 INFINITE（但要注意刷新）
            // 这里设为 16ms 是为了兼顾可能的动画更新和低 CPU
            if (SDL_WaitEventTimeout(&event, 16)) {
                // 有事件发生，批量读取所有积压事件
                std::vector<SDL_Event> batch_events;
                batch_events.push_back(event);
                while (SDL_PollEvent(&event)) {
                    batch_events.push_back(event);
                }

                // 2. 第一遍：优先处理所有非鼠标移动事件（点击、键盘、窗口消息等）
                // 并找出最后一个鼠标移动事件
                SDL_Event* last_mouse_motion = nullptr;

                for (auto& e : batch_events) {
                    if (e.type == SDL_EVENT_MOUSE_MOTION) {
                        last_mouse_motion = &e;
                    } else {
                        // 立即处理非移动事件
                        if (e.type == SDL_EVENT_QUIT) {
                            running = false;
                        }
                        else if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                            float dpi_scale = window_->GetDisplayScale();
                            float logical_x = e.button.x / dpi_scale;
                            float logical_y = e.button.y / dpi_scale;
                            std::cout << "[Click] " << logical_x << "," << logical_y << std::endl;
                            HandleClick(logical_x, logical_y);
                        }
                        else if (e.type == SDL_EVENT_WINDOW_RESIZED) {
                             int new_width = e.window.data1;
                             int new_height = e.window.data2;
                             std::cout << "[Resize] " << new_width << "x" << new_height << std::endl;
                             window_->OnResize();
                             window_->InvalidateRenderTree();
                             window_->SetNeedsRepaint();
                        }
                        else if (e.type == SDL_EVENT_MOUSE_WHEEL) {
                            float scroll_x = e.wheel.x * 40.0f;
                            float scroll_y = e.wheel.y * 40.0f;
                            auto body = doc_->GetBody();
                            if (body) {
                                auto render_obj = body->GetRenderObject();
                                if (render_obj && render_obj->IsScrollable()) {
                                    float old_scroll_y = render_obj->GetScrollY();
                                    render_obj->ScrollBy(-scroll_x, -scroll_y);
                                    if (render_obj->GetScrollY() != old_scroll_y) {
                                        window_->SetNeedsRepaint();
                                    }
                                }
                            }
                        }
                        else if (e.type == SDL_EVENT_KEY_DOWN) {
                            bool moved = false;
                            switch (e.key.scancode) {
                                case SDL_SCANCODE_LEFT: case SDL_SCANCODE_A: box_x_ -= 20; moved = true; break;
                                case SDL_SCANCODE_RIGHT: case SDL_SCANCODE_D: box_x_ += 20; moved = true; break;
                                case SDL_SCANCODE_UP: case SDL_SCANCODE_W: box_y_ -= 20; moved = true; break;
                                case SDL_SCANCODE_DOWN: case SDL_SCANCODE_S: box_y_ += 20; moved = true; break;
                                case SDL_SCANCODE_P: TogglePopup(); break;
                                default: break;
                            }
                            if (moved && moving_box_) {
                                moving_box_->SetStyle("left", std::to_string(box_x_) + "px");
                                moving_box_->SetStyle("top", std::to_string(box_y_) + "px");
                                window_->SetNeedsRepaint();
                            }
                        }
                    }
                }

                // 3. 第二遍：只处理最后一次鼠标移动事件（事件合并）
                if (last_mouse_motion) {
                    static float last_hover_x = -1, last_hover_y = -1;
                    float dpi_scale = window_->GetDisplayScale();
                    float logical_x = last_mouse_motion->motion.x / dpi_scale;
                    float logical_y = last_mouse_motion->motion.y / dpi_scale;
                    
                    if (logical_x != last_hover_x || logical_y != last_hover_y) {
                        last_hover_x = logical_x;
                        last_hover_y = logical_y;
                        HandleHover(logical_x, logical_y);
                    }
                }
            }

            // 渲染（限制帧率）
            // 在事件驱动模型中，即使没有事件（超时返回），也需要检查是否需要渲染（例如动画）
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_render);
            
            // 只有当有需要重绘的内容或者距离上次渲染超过一定时间时才渲染
            // 但在这个测试中，为了简单起见，我们保持 ~60FPS 的检查频率
            // 由于 SDL_WaitEventTimeout(16) 的存在，这本身就是受限的
            if (elapsed.count() >= 16) {  // ~60 FPS
                if (window_->NeedsRepaint()) {
                    lightui::RenderObject::ResetPaintStats();
                    
                    auto render_start = std::chrono::high_resolution_clock::now();
                    window_->Render();
                    auto render_end = std::chrono::high_resolution_clock::now();
                    auto render_us = std::chrono::duration_cast<std::chrono::microseconds>(render_end - render_start).count();
                    // 仅在渲染耗时较长时输出，避免刷屏
                    if (render_us > 1000) {
                        std::cout << "[Perf] Render: " << render_us << " μs" << std::endl;
                    }
                    
                    lightui::RenderObject::PrintPaintStats();
                }
                window_->SwapBuffers();
                last_render = now;
            }
        }
    }

private:
    void CreateUI() {
        auto body = doc_->GetBody();
        if (!body) {
            std::cout << "ERROR: Body is null!" << std::endl;
            return;
        }
        std::cout << "Body found, creating UI..." << std::endl;

        // 设置 body 样式
        body->SetStyle("padding", "20px");
        body->SetStyle("font-family", "Arial, sans-serif");
        body->SetStyle("background-color", "#f0f0f0");

        // 标题
        auto title = doc_->CreateElement("h1");
        title->SetTextContent("Incremental Render Test");
        title->SetStyle("color", "#333");
        title->SetStyle("margin-bottom", "20px");
        body->AppendChild(title);

        // 按钮容器 - 使用 flexbox
        auto button_container = doc_->CreateElement("div");
        button_container->SetAttribute("id", "button-container");
        button_container->SetStyle("display", "flex");
        button_container->SetStyle("gap", "10px");
        button_container->SetStyle("margin-bottom", "20px");
        body->AppendChild(button_container);

        // Add 按钮
        add_button_ = doc_->CreateElement("button");
        add_button_->SetAttribute("id", "add-btn");
        add_button_->SetTextContent("Add Item");
        add_button_->SetStyle("padding", "10px 20px");
        add_button_->SetStyle("background-color", "#4CAF50");
        add_button_->SetStyle("color", "white");
        
        // 调试：检查 inline style 是否正确设置
        std::cout << "[DEBUG] Add button inline styles after SetStyle:" << std::endl;
        std::cout << "  style attribute: '" << add_button_->GetAttribute("style") << "'" << std::endl;
        
        button_container->AppendChild(add_button_);

        // Remove 按钮
        remove_button_ = doc_->CreateElement("button");
        remove_button_->SetAttribute("id", "remove-btn");
        remove_button_->SetTextContent("Remove Item");
        remove_button_->SetStyle("padding", "10px 20px");
        remove_button_->SetStyle("background-color", "#f44336");
        remove_button_->SetStyle("color", "white");
        button_container->AppendChild(remove_button_);

        // Clear 按钮
        clear_button_ = doc_->CreateElement("button");
        clear_button_->SetAttribute("id", "clear-btn");
        clear_button_->SetTextContent("Clear All");
        clear_button_->SetStyle("padding", "10px 20px");
        clear_button_->SetStyle("background-color", "#666");
        clear_button_->SetStyle("color", "white");
        button_container->AppendChild(clear_button_);

        // 计数器显示
        counter_display_ = doc_->CreateElement("div");
        counter_display_->SetStyle("margin-bottom", "20px");
        counter_display_->SetStyle("font-size", "18px");
        counter_display_->SetStyle("color", "#666");
        UpdateCounterDisplay();
        body->AppendChild(counter_display_);

        // 项目列表容器
        list_container_ = doc_->CreateElement("div");
        list_container_->SetAttribute("id", "list-container");
        list_container_->SetStyle("border", "1px solid #ccc");
        list_container_->SetStyle("padding", "10px");
        list_container_->SetStyle("min-height", "100px");
        list_container_->SetStyle("background-color", "white");
        body->AppendChild(list_container_);

        // ========== 移动方块演示 (验证双区域脏标记) ==========
        auto move_hint = doc_->CreateElement("div");
        move_hint->SetStyle("margin-top", "20px");
        move_hint->SetStyle("margin-bottom", "10px");
        move_hint->SetStyle("color", "#666");
        move_hint->SetTextContent("Use WASD or Arrow keys to move the box. Press P to toggle popup.");
        body->AppendChild(move_hint);

        // 移动方块容器 (相对定位，作为 absolute 定位的参考)
        auto box_container = doc_->CreateElement("div");
        box_container->SetAttribute("id", "box-container");
        box_container->SetStyle("position", "relative");
        box_container->SetStyle("width", "400px");
        box_container->SetStyle("height", "150px");
        box_container->SetStyle("border", "2px dashed #999");
        box_container->SetStyle("background-color", "#fafafa");
        body->AppendChild(box_container);

        // 可移动的方块 (absolute 定位)
        moving_box_ = doc_->CreateElement("div");
        moving_box_->SetAttribute("id", "moving-box");
        moving_box_->SetStyle("position", "absolute");
        moving_box_->SetStyle("left", std::to_string(box_x_) + "px");
        moving_box_->SetStyle("top", std::to_string(box_y_) + "px");
        moving_box_->SetStyle("width", "60px");
        moving_box_->SetStyle("height", "60px");
        moving_box_->SetStyle("background-color", "#FF5722");
        moving_box_->SetStyle("border-radius", "8px");
        box_container->AppendChild(moving_box_);

        // ========== 弹出层演示 (验证布局隔离) ==========
        popup_layer_ = doc_->CreateElement("div");
        popup_layer_->SetAttribute("id", "popup");
        popup_layer_->SetStyle("position", "fixed");
        popup_layer_->SetStyle("left", "150px");
        popup_layer_->SetStyle("top", "150px");
        popup_layer_->SetStyle("width", "200px");
        popup_layer_->SetStyle("padding", "20px");
        popup_layer_->SetStyle("background-color", "#FFF3E0");
        popup_layer_->SetStyle("border", "2px solid #FF9800");
        popup_layer_->SetStyle("border-radius", "8px");
        popup_layer_->SetStyle("display", "none");  // 初始隐藏
        popup_layer_->SetTextContent("This is a popup! Press P to close.");
        body->AppendChild(popup_layer_);

        // 初始渲染
        std::cout << "UI created, rendering..." << std::endl;
        window_->Render();
        std::cout << "Initial render complete!" << std::endl;
        
        // 调试：检查按钮的布局
        if (add_button_) {
            auto ro = add_button_->GetRenderObject();
            if (ro) {
                auto bounds = ro->GetBoundingRect();
                auto& style = ro->GetComputedStyle();
                std::cout << "[DEBUG] Add button after render:" << std::endl;
                std::cout << "  Bounds: [" << bounds.left() << "," << bounds.top() 
                          << "," << bounds.right() << "," << bounds.bottom() << "]" << std::endl;
                std::cout << "  Size: " << bounds.width() << "x" << bounds.height() << std::endl;
                std::cout << "  Padding: " << style.padding.top.ToPx(0, style.font_size) << "px " 
                          << style.padding.right.ToPx(0, style.font_size) << "px" << std::endl;
                std::cout << "  Display: " << static_cast<int>(style.display) << std::endl;
            } else {
                std::cout << "[DEBUG] Add button has NO RenderObject!" << std::endl;
            }
        }
    }

    void HandleClick(float x, float y) {
        // 简单的点击检测 - 检查是否点击了按钮
        // 这里我们使用简单的位置检测
        std::cout << "[HandleClick] x=" << x << ", y=" << y << std::endl;

        // 获取按钮的渲染对象位置
        if (add_button_) {
            auto ro = add_button_->GetRenderObject();
            std::cout << "[HandleClick] add_button RenderObject: "  << (ro ? "valid" : "NULL") << std::endl;
            if (ro) {
                auto bounds = ro->GetBoundingRect();
                std::cout << "[HandleClick] add_button bounds: [" 
                          << bounds.left() << "," << bounds.top() << "," 
                          << bounds.right() << "," << bounds.bottom() << "]" << std::endl;
                if (x >= bounds.left() && x <= bounds.right() &&
                    y >= bounds.top() && y <= bounds.bottom()) {
                    std::cout << "[HandleClick] Add button clicked!" << std::endl;
                    OnAddItem();
                    return;
                }
            }
        }

        if (remove_button_) {
            auto ro = remove_button_->GetRenderObject();
            std::cout << "[HandleClick] remove_button RenderObject: " << (ro ? "valid" : "NULL") << std::endl;
            if (ro) {
                auto bounds = ro->GetBoundingRect();
                if (x >= bounds.left() && x <= bounds.right() &&
                    y >= bounds.top() && y <= bounds.bottom()) {
                    std::cout << "[HandleClick] Remove button clicked!" << std::endl;
                    OnRemoveItem();
                    return;
                }
            }
        }

        if (clear_button_) {
            auto ro = clear_button_->GetRenderObject();
            std::cout << "[HandleClick] clear_button RenderObject: " << (ro ? "valid" : "NULL") << std::endl;
            if (ro) {
                auto bounds = ro->GetBoundingRect();
                if (x >= bounds.left() && x <= bounds.right() &&
                    y >= bounds.top() && y <= bounds.bottom()) {
                    std::cout << "[HandleClick] Clear button clicked!" << std::endl;
                    OnClearAll();
                    return;
                }
            }
        }
        
        std::cout << "[HandleClick] No button clicked at this position" << std::endl;
    }

    // 处理鼠标悬停 - 更新按钮 hover 状态和视觉效果
    // 优化：只在状态变化时才更新样式和触发重绘
    void HandleHover(float x, float y) {
        // 按钮颜色配置: {正常颜色, hover颜色}
        struct ButtonColors {
            std::string normal;
            std::string hover;
        };
        
        // 检查每个按钮是否被 hover，并更新视觉效果
        auto checkHover = [&](std::shared_ptr<Element> button, const std::string& name, 
                              const ButtonColors& colors) {
            if (!button) return;
            auto ro = button->GetRenderObject();
            if (!ro) return;
            
            auto bounds = ro->GetBoundingRect();
            bool is_hovering = (x >= bounds.left() && x <= bounds.right() &&
                               y >= bounds.top() && y <= bounds.bottom());
            
            // 只有状态变化时才更新 (避免无效重绘)
            if (is_hovering != button->HasPseudoClass("hover")) {
                button->SetPseudoClass("hover", is_hovering);
                // 更新视觉效果 - hover 时使用更亮的颜色
                button->SetStyle("background-color", is_hovering ? colors.hover : colors.normal);
                window_->SetNeedsRepaint();
            }
        };
        
        // Add 按钮: 绿色系
        checkHover(add_button_, "add_button", {"#4CAF50", "#66BB6A"});
        // Remove 按钮: 红色系  
        checkHover(remove_button_, "remove_button", {"#f44336", "#EF5350"});
        // Clear 按钮: 灰色系
        checkHover(clear_button_, "clear_button", {"#757575", "#9E9E9E"});
    }

    void OnAddItem() {
        if (!list_container_) return;

        auto start = std::chrono::high_resolution_clock::now();

        item_count_++;

        // 创建新项目
        auto item = doc_->CreateElement("div");
        item->SetAttribute("id", "item-" + std::to_string(item_count_));
        item->SetStyle("padding", "10px");
        item->SetStyle("margin", "5px 0");
        item->SetStyle("background-color", "#e3f2fd");
        item->SetStyle("border-radius", "4px");
        item->SetStyle("border-left", "4px solid #2196F3");
        item->SetTextContent("Item #" + std::to_string(item_count_) + " - Added at " +
                            std::to_string(std::chrono::system_clock::now().time_since_epoch().count() % 10000));

        list_container_->AppendChild(item);
        UpdateCounterDisplay();

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

        std::cout << "[Add] Item #" << item_count_ << " added in " << duration << " us" << std::endl;
    }

    void OnRemoveItem() {
        if (!list_container_) return;

        auto children = list_container_->GetChildNodes();
        if (children.empty()) {
            std::cout << "[Remove] No items to remove" << std::endl;
            return;
        }

        auto start = std::chrono::high_resolution_clock::now();

        // 删除最后一个项目
        auto last_child = children.back();
        list_container_->RemoveChild(last_child);
        UpdateCounterDisplay();

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

        std::cout << "[Remove] Item removed in " << duration << " us" << std::endl;
    }

    void OnClearAll() {
        if (!list_container_) return;

        auto start = std::chrono::high_resolution_clock::now();

        // 使用批量更新
        doc_->BeginBatch();

        auto children = list_container_->GetChildNodes();
        for (const auto& child : children) {
            list_container_->RemoveChild(child);
        }
        item_count_ = 0;
        UpdateCounterDisplay();

        doc_->EndBatch();

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

        std::cout << "[Clear] All items cleared in " << duration << " us" << std::endl;
    }

    void UpdateCounterDisplay() {
        if (counter_display_) {
            auto children = list_container_ ? list_container_->GetChildNodes().size() : 0;
            counter_display_->SetTextContent("Items in list: " + std::to_string(children));
        }
    }

    void TogglePopup() {
        if (!popup_layer_) return;
        
        popup_visible_ = !popup_visible_;
        popup_layer_->SetStyle("display", popup_visible_ ? "block" : "none");
        
        std::cout << "[Popup] " << (popup_visible_ ? "Shown" : "Hidden") 
                  << " (fixed positioning - should NOT trigger parent layout)" << std::endl;
        
        window_->SetNeedsRepaint();
    }

    std::unique_ptr<Window> window_;
    std::shared_ptr<Document> doc_;
    std::shared_ptr<Element> add_button_;
    std::shared_ptr<Element> remove_button_;
    std::shared_ptr<Element> clear_button_;
    std::shared_ptr<Element> counter_display_;
    std::shared_ptr<Element> list_container_;
    std::shared_ptr<Element> moving_box_;
    std::shared_ptr<Element> popup_layer_;
    int item_count_;
    float box_x_;
    float box_y_;
    bool popup_visible_;
};

int main() {
    IncrementalRenderTest test;

    if (!test.Initialize()) {
        std::cerr << "Initialization failed!" << std::endl;
        return 1;
    }

    test.Run();

    std::cout << "Test completed." << std::endl;
    return 0;
}

