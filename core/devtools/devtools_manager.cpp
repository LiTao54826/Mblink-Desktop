/**
 * @file devtools_manager.cpp
 * @brief DevTools 管理器实现
 */

#include "devtools_manager.h"
#include "devtools_panel.h"
#include "devtools_state.h"
#include "inspector/element_highlighter.h"
#include "inspector/element_picker.h"
#include "core/dom/event.h"
#include <fstream>
#include <sstream>

namespace lightui {

DevToolsManager& DevToolsManager::GetInstance() {
    static DevToolsManager instance;
    return instance;
}

DevToolsManager::DevToolsManager() = default;

DevToolsManager::~DevToolsManager() {
    Shutdown();
}

void DevToolsManager::Initialize(Document* document, Window* window) {
    document_ = document;
    window_ = window;

    // 创建子组件
    panel_ = std::make_unique<DevToolsPanel>(document_);
    highlighter_ = std::make_unique<ElementHighlighter>();
    picker_ = std::make_unique<ElementPicker>(document_);

    // 设置 Box Model 悬停回调
    if (panel_) {
        panel_->SetOnBoxModelHover([this](std::shared_ptr<Element> element, int area) {
            SetBoxModelHover(element, area);
        });
        
        // 设置元素选择器切换回调
        panel_->SetOnPickerToggled([this](bool active) {
            if (active) {
                StartElementPicker();
            } else {
                StopElementPicker();
            }
        });
        
        // 设置停靠位置切换回调
        panel_->SetOnDockPositionToggled([this]() {
            ToggleDockPosition();
        });
        
        // 设置初始停靠位置显示
        panel_->SetDockPosition(dock_position_ == DockPosition::Bottom);
    }

    // 注册为 DOM 观察者
    if (document_) {
        document_->GetObserverManager().AddObserver(this);
    }

    // 加载保存的状态
    LoadState();
}

void DevToolsManager::Shutdown() {
    // 保存状态
    if (document_) {
        SaveState();
        // 取消注册 DOM 观察者
        document_->GetObserverManager().RemoveObserver(this);
    }

    picker_.reset();
    highlighter_.reset();
    panel_.reset();

    document_ = nullptr;
    window_ = nullptr;
    selected_element_.reset();
    is_open_ = false;
    picker_active_ = false;
}

void DevToolsManager::Toggle() {
    if (is_open_) {
        Close();
    } else {
        Open();
    }
}

void DevToolsManager::Open() {
    if (!is_open_) {
        is_open_ = true;
        // 刷新面板内容
        if (panel_) {
            panel_->Refresh();
        }
    }
}

void DevToolsManager::Close() {
    if (is_open_) {
        is_open_ = false;
        // 停止拾取模式
        StopElementPicker();
    }
}

void DevToolsManager::SetDockPosition(DockPosition position) {
    dock_position_ = position;
    // 更新面板显示
    if (panel_) {
        panel_->SetDockPosition(dock_position_ == DockPosition::Bottom);
    }
}

void DevToolsManager::ToggleDockPosition() {
    if (dock_position_ == DockPosition::Bottom) {
        SetDockPosition(DockPosition::Right);
    } else {
        SetDockPosition(DockPosition::Bottom);
    }
}

void DevToolsManager::SetPanelSize(float size) {
    // 限制在合理范围内
    panel_size_ = std::max(0.1f, std::min(0.8f, size));
}

void DevToolsManager::SelectElement(std::shared_ptr<Element> element) {
    selected_element_ = element;

    // 更新高亮
    if (highlighter_) {
        highlighter_->SetHighlightedElement(element);
    }

    // 更新面板
    if (panel_) {
        panel_->SetSelectedElement(element);
    }
}

void DevToolsManager::StartElementPicker() {
    picker_active_ = true;
    if (picker_) {
        picker_->Start();
    }
    // 同步面板状态
    if (panel_) {
        panel_->SetPickerActive(true);
    }
}

void DevToolsManager::StopElementPicker() {
    picker_active_ = false;
    if (picker_) {
        picker_->Stop();
    }
    // 同步面板状态
    if (panel_) {
        panel_->SetPickerActive(false);
    }
}

void DevToolsManager::Render(SkCanvas* canvas, float window_width, float window_height) {
    if (!is_open_ || !panel_) {
        return;
    }

    float panel_x, panel_y, panel_width, panel_height;
    GetPanelBounds(window_width, window_height, panel_x, panel_y, panel_width, panel_height);

    panel_->Render(canvas, panel_x, panel_y, panel_width, panel_height);
}

void DevToolsManager::RenderHighlight(SkCanvas* canvas) {
    if (highlighter_) {
        highlighter_->Render(canvas);
    }

    // 拾取模式下显示悬停高亮
    if (picker_active_ && picker_) {
        picker_->RenderHoverHighlight(canvas);
    }
}

bool DevToolsManager::HandleEvent(const Event& event) {
    if (!is_open_) {
        return false;
    }

    // 拾取模式优先处理
    if (picker_active_ && picker_) {
        if (picker_->HandleEvent(event)) {
            // 如果拾取了元素，选中它
            auto picked = picker_->GetPickedElement();
            if (picked) {
                SelectElement(picked);
                StopElementPicker();
            }
            return true;
        }
    }

    // 面板处理事件
    if (panel_) {
        return panel_->HandleEvent(event);
    }

    return false;
}

bool DevToolsManager::HandleMouseEvent(int x, int y, int button, bool pressed) {
    if (!is_open_ || !panel_) {
        return false;
    }

    // 将事件传递给面板
    return panel_->HandleMouseEvent(x, y, button, pressed);
}

bool DevToolsManager::HandleMouseMove(int x, int y) {
    if (!is_open_ || !panel_) {
        return false;
    }

    // 将鼠标移动事件传递给面板
    return panel_->HandleMouseMove(x, y);
}

void DevToolsManager::SetBoxModelHover(std::shared_ptr<Element> element, int area) {
    if (!highlighter_) return;
    
    if (area == 0 || !element) {
        // area == 0 表示 None，清除高亮
        highlighter_->ClearBoxModelHover();
    } else {
        // 转换 area 到 HighlightAreaType
        HighlightAreaType highlight_area = static_cast<HighlightAreaType>(area);
        highlighter_->SetBoxModelHover(element, highlight_area);
    }
}

void DevToolsManager::ClearBoxModelHover() {
    if (panel_) {
        panel_->ClearBoxModelHover();
    }
    if (highlighter_) {
        highlighter_->ClearBoxModelHover();
    }
}

bool DevToolsManager::HandleMouseWheel(int x, int y, float delta_x, float delta_y) {
    if (!is_open_ || !panel_) {
        return false;
    }
    
    return panel_->HandleMouseWheel(x, y, delta_x, delta_y);
}

bool DevToolsManager::IsDraggingSplitter() const {
    if (!is_open_ || !panel_) {
        return false;
    }
    return panel_->IsDraggingSplitter();
}

bool DevToolsManager::IsMouseOnSplitter(int x, int y) const {
    if (!is_open_ || !panel_) {
        return false;
    }
    return panel_->IsMouseOnSplitter(x, y);
}

bool DevToolsManager::HandleKeyboardShortcut(int key, bool ctrl, bool shift, bool alt) {
    // F12 切换 DevTools
    if (key == 123 && !ctrl && !shift && !alt) {  // F12 = 123
        Toggle();
        return true;
    }

    // Ctrl+Shift+I 切换 DevTools
    if (key == 'I' && ctrl && shift && !alt) {
        Toggle();
        return true;
    }

    // Escape 退出拾取模式
    if (key == 27 && picker_active_) {  // Escape = 27
        StopElementPicker();
        return true;
    }

    return false;
}

void DevToolsManager::SaveState() {
    DevToolsState state;
    state.is_open = is_open_;
    state.dock_position = dock_position_;
    state.panel_size = panel_size_;
    
    // TODO: 保存展开的节点 ID 和选中的元素路径
    
    std::string json = state.ToJSON();
    
    // 保存到文件
    std::string config_path = ".devtools_state.json";
    std::ofstream file(config_path);
    if (file.is_open()) {
        file << json;
        file.close();
    }
}

void DevToolsManager::LoadState() {
    std::string config_path = ".devtools_state.json";
    std::ifstream file(config_path);
    if (!file.is_open()) {
        return;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string json = buffer.str();
    file.close();
    
    DevToolsState state = DevToolsState::FromJSON(json);
    
    // 恢复状态（但不自动打开）
    dock_position_ = state.dock_position;
    panel_size_ = state.panel_size;
    
    // 注意：不恢复 is_open_ 状态，让用户手动打开
}

void DevToolsManager::GetMainAppBounds(float window_width, float window_height,
                                        float& out_x, float& out_y,
                                        float& out_width, float& out_height) const {
    out_x = 0;
    out_y = 0;

    if (!is_open_) {
        out_width = window_width;
        out_height = window_height;
        return;
    }

    if (dock_position_ == DockPosition::Bottom) {
        out_width = window_width;
        out_height = window_height * (1.0f - panel_size_);
    } else {  // Right
        out_width = window_width * (1.0f - panel_size_);
        out_height = window_height;
    }
}

void DevToolsManager::GetPanelBounds(float window_width, float window_height,
                                      float& out_x, float& out_y,
                                      float& out_width, float& out_height) const {
    if (dock_position_ == DockPosition::Bottom) {
        out_x = 0;
        out_y = window_height * (1.0f - panel_size_);
        out_width = window_width;
        out_height = window_height * panel_size_;
    } else {  // Right
        out_x = window_width * (1.0f - panel_size_);
        out_y = 0;
        out_width = window_width * panel_size_;
        out_height = window_height;
    }
}

bool DevToolsManager::IsMouseOnPanelBorder(float x, float y, float window_width, float window_height) const {
    if (!is_open_) {
        return false;
    }

    const float border_width = 12.0f;  // 边界检测宽度（加宽以便更容易选中）
    
    if (dock_position_ == DockPosition::Bottom) {
        // 底部停靠时，检测上边界
        float border_y = window_height * (1.0f - panel_size_);
        return y >= border_y - border_width / 2 && y <= border_y + border_width / 2;
    } else {
        // 右侧停靠时，检测左边界
        float border_x = window_width * (1.0f - panel_size_);
        return x >= border_x - border_width / 2 && x <= border_x + border_width / 2;
    }
}

bool DevToolsManager::HandlePanelBorderDrag(float x, float y, float window_width, float window_height, bool pressed) {
    if (!is_open_) {
        return false;
    }

    if (pressed) {
        // 鼠标按下，开始拖动
        if (IsMouseOnPanelBorder(x, y, window_width, window_height)) {
            dragging_panel_border_ = true;
            return true;
        }
    } else {
        // 鼠标释放，结束拖动
        if (dragging_panel_border_) {
            dragging_panel_border_ = false;
            return true;
        }
    }
    return false;
}

bool DevToolsManager::UpdatePanelBorderDrag(float x, float y, float window_width, float window_height) {
    if (!dragging_panel_border_) {
        return false;
    }

    float new_size;
    if (dock_position_ == DockPosition::Bottom) {
        // 底部停靠时，根据 Y 坐标计算新尺寸
        new_size = 1.0f - (y / window_height);
    } else {
        // 右侧停靠时，根据 X 坐标计算新尺寸
        new_size = 1.0f - (x / window_width);
    }

    // 限制在合理范围内 (10% - 80%)
    new_size = std::max(0.1f, std::min(0.8f, new_size));

    if (new_size != panel_size_) {
        panel_size_ = new_size;
        return true;
    }
    return false;
}

// ========== DOMObserver 接口实现 ==========

void DevToolsManager::OnNodeAdded(Node* node, Node* parent) {
    if (!is_open_ || !panel_) {
        return;
    }
    
    // 刷新 DOM 树视图
    panel_->RefreshDOMTree();
}

void DevToolsManager::OnNodeRemoved(Node* node, Node* parent) {
    if (!is_open_ || !panel_) {
        return;
    }
    
    // 如果移除的是当前选中的元素，清除选择
    if (selected_element_ && selected_element_.get() == node) {
        selected_element_.reset();
        if (highlighter_) {
            highlighter_->ClearHighlight();
        }
    }
    
    // 刷新 DOM 树视图
    panel_->RefreshDOMTree();
}

void DevToolsManager::OnAttributeChanged(Element* element, 
                                         const std::string& name,
                                         const std::string& old_value,
                                         const std::string& new_value) {
    if (!is_open_ || !panel_) {
        return;
    }
    
    // 如果是当前选中的元素，更新属性面板
    if (selected_element_ && selected_element_.get() == element) {
        panel_->RefreshAttributes();
    }
    
    // 刷新 DOM 树视图（属性可能影响显示）
    panel_->RefreshDOMTree();
}

void DevToolsManager::OnStyleChanged(Element* element,
                                     const std::string& property,
                                     const std::string& old_value,
                                     const std::string& new_value) {
    if (!is_open_ || !panel_) {
        return;
    }
    
    // 如果是当前选中的元素，更新样式面板
    if (selected_element_ && selected_element_.get() == element) {
        panel_->RefreshStyles();
    }
}

void DevToolsManager::OnTextChanged(Node* node,
                                    const std::string& old_text,
                                    const std::string& new_text) {
    if (!is_open_ || !panel_) {
        return;
    }
    
    // 刷新 DOM 树视图
    panel_->RefreshDOMTree();
}

} // namespace lightui
