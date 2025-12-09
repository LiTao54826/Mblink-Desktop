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
#include "core/render/render_object.h"

using namespace lightui;

class IncrementalRenderTest {
public:
    IncrementalRenderTest() : item_count_(0) {}

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
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_EVENT_QUIT) {
                    running = false;
                    break;
                }

                if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                    HandleClick(event.button.x, event.button.y);
                }

                // 处理窗口大小改变
                if (event.type == SDL_EVENT_WINDOW_RESIZED) {
                    int new_width = event.window.data1;
                    int new_height = event.window.data2;
                    std::cout << "[Resize] " << new_width << "x" << new_height << std::endl;
                    window_->OnResize();  // 重新创建 surface
                    window_->InvalidateRenderTree();  // 需要重新布局
                    window_->SetNeedsRepaint();
                }
            }

            // 渲染（限制帧率）
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_render);
            if (elapsed.count() >= 16) {  // ~60 FPS
                // 使用 RenderDocument 而非 RenderDocumentIncremental 来确保布局正确
                if (window_->NeedsRepaint()) {
                    window_->RenderDocument();
                }
                window_->SwapBuffers();  // 显示到屏幕
                last_render = now;
            }

            SDL_Delay(1);
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
        list_container_->SetStyle("min-height", "200px");
        list_container_->SetStyle("background-color", "white");
        body->AppendChild(list_container_);

        // 初始渲染
        std::cout << "UI created, rendering..." << std::endl;
        window_->RenderDocumentIncremental();
        std::cout << "Initial render complete!" << std::endl;
    }

    void HandleClick(float x, float y) {
        // 简单的点击检测 - 检查是否点击了按钮
        // 这里我们使用简单的位置检测

        // 获取按钮的渲染对象位置
        if (add_button_) {
            auto ro = add_button_->GetRenderObject();
            if (ro) {
                auto bounds = ro->GetBoundingRect();
                if (x >= bounds.left() && x <= bounds.right() &&
                    y >= bounds.top() && y <= bounds.bottom()) {
                    OnAddItem();
                    return;
                }
            }
        }

        if (remove_button_) {
            auto ro = remove_button_->GetRenderObject();
            if (ro) {
                auto bounds = ro->GetBoundingRect();
                if (x >= bounds.left() && x <= bounds.right() &&
                    y >= bounds.top() && y <= bounds.bottom()) {
                    OnRemoveItem();
                    return;
                }
            }
        }

        if (clear_button_) {
            auto ro = clear_button_->GetRenderObject();
            if (ro) {
                auto bounds = ro->GetBoundingRect();
                if (x >= bounds.left() && x <= bounds.right() &&
                    y >= bounds.top() && y <= bounds.bottom()) {
                    OnClearAll();
                    return;
                }
            }
        }
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

    std::unique_ptr<Window> window_;
    std::shared_ptr<Document> doc_;
    std::shared_ptr<Element> add_button_;
    std::shared_ptr<Element> remove_button_;
    std::shared_ptr<Element> clear_button_;
    std::shared_ptr<Element> counter_display_;
    std::shared_ptr<Element> list_container_;
    int item_count_;
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

