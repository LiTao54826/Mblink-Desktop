/**
 * @file devtools_manager.h
 * @brief DevTools 管理器 - 开发者工具的核心管理类
 * 
 * 功能：
 * - DevTools 生命周期管理
 * - 面板控制（打开/关闭/切换）
 * - 停靠位置和尺寸管理
 * - 元素选择和拾取模式
 * - 状态持久化
 */

#pragma once

#include <memory>
#include <functional>
#include "core/dom/document.h"
#include "core/dom/element.h"
#include "core/dom/observers/dom_observer.h"

// 前向声明
class SkCanvas;

namespace mbink {

// 前向声明
class Window;
class DevToolsPanel;
class ElementHighlighter;
class ElementPicker;
class Event;
struct LayoutBox;

/**
 * @brief DevTools 停靠位置
 */
enum class DockPosition {
    Bottom,  // 底部停靠
    Right    // 右侧停靠
};

/**
 * @brief DevTools 管理器
 * 
 * 实现 DOMObserver 接口以监听 DOM 变化并更新视图
 */
class DevToolsManager : public DOMObserver {
public:
    /**
     * @brief 获取单例实例
     */
    static DevToolsManager& GetInstance();

    /**
     * @brief 初始化 DevTools
     * @param document 文档对象
     * @param window 窗口对象
     */
    void Initialize(Document* document, Window* window);

    /**
     * @brief 关闭 DevTools
     */
    void Shutdown();

    /**
     * @brief 切换 DevTools 面板显示状态
     */
    void Toggle();

    /**
     * @brief 打开 DevTools 面板
     */
    void Open();

    /**
     * @brief 关闭 DevTools 面板
     */
    void Close();

    /**
     * @brief 检查 DevTools 是否打开
     */
    bool IsOpen() const { return is_open_; }

    /**
     * @brief 设置停靠位置
     */
    void SetDockPosition(DockPosition position);

    /**
     * @brief 获取停靠位置
     */
    DockPosition GetDockPosition() const { return dock_position_; }

    /**
     * @brief 切换停靠位置（底部/右侧）
     */
    void ToggleDockPosition();

    /**
     * @brief 设置面板尺寸（百分比，0.0-1.0）
     */
    void SetPanelSize(float size);

    /**
     * @brief 获取面板尺寸
     */
    float GetPanelSize() const { return panel_size_; }

    /**
     * @brief 选择元素
     */
    void SelectElement(std::shared_ptr<Element> element);

    /**
     * @brief 获取选中的元素
     */
    std::shared_ptr<Element> GetSelectedElement() const { return selected_element_; }

    /**
     * @brief 启动元素拾取模式
     */
    void StartElementPicker();

    /**
     * @brief 停止元素拾取模式
     */
    void StopElementPicker();

    /**
     * @brief 检查拾取模式是否激活
     */
    bool IsPickerActive() const { return picker_active_; }

    /**
     * @brief 渲染 DevTools
     * @param canvas Skia 画布
     * @param window_width 窗口宽度
     * @param window_height 窗口高度
     */
    void Render(SkCanvas* canvas, float window_width, float window_height);

    /**
     * @brief 渲染高亮覆盖层（在主应用内容之上）
     * @param canvas Skia 画布
     */
    void RenderHighlight(SkCanvas* canvas);

    /**
     * @brief 处理事件
     * @param event 事件对象
     * @return 是否消费了事件
     */
    bool HandleEvent(const Event& event);

    /**
     * @brief 处理鼠标事件
     * @param x 鼠标 X 坐标（相对于面板）
     * @param y 鼠标 Y 坐标（相对于面板）
     * @param button 鼠标按钮（0=左键，1=右键）
     * @param pressed 是否按下
     * @return 是否消费了事件
     */
    bool HandleMouseEvent(int x, int y, int button, bool pressed);
    
    /**
     * @brief 处理鼠标移动事件
     * @param x 鼠标 X 坐标（相对于面板）
     * @param y 鼠标 Y 坐标（相对于面板）
     * @return 是否需要重绘
     */
    bool HandleMouseMove(int x, int y);

    /**
     * @brief 获取元素选择器（用于event_loop设置悬停元素）
     */
    ElementPicker* GetElementPicker() { return picker_.get(); }

    /**
     * @brief 设置 Box Model 悬停高亮
     * @param element 要高亮的元素
     * @param area 要高亮的区域 (0=None, 1=Margin, 2=Border, 3=Padding, 4=Content)
     */
    void SetBoxModelHover(std::shared_ptr<Element> element, int area);

    /**
     * @brief 清除 Box Model 悬停高亮
     */
    void ClearBoxModelHover();

    /**
     * @brief 处理鼠标滚轮事件
     */
    bool HandleMouseWheel(int x, int y, float delta_x, float delta_y);

    /**
     * @brief 检查是否正在拖动分隔线
     */
    bool IsDraggingSplitter() const;

    /**
     * @brief 检查鼠标是否在分隔线上
     * @param x 鼠标 X 坐标（相对于面板）
     * @param y 鼠标 Y 坐标（相对于面板）
     * @return 是否在分隔线上
     */
    bool IsMouseOnSplitter(int x, int y) const;

    /**
     * @brief 处理键盘快捷键
     * @param key 按键
     * @param ctrl 是否按下 Ctrl
     * @param shift 是否按下 Shift
     * @param alt 是否按下 Alt
     * @return 是否消费了事件
     */
    bool HandleKeyboardShortcut(int key, bool ctrl, bool shift, bool alt);

    /**
     * @brief 保存状态
     */
    void SaveState();

    /**
     * @brief 加载状态
     */
    void LoadState();

    /**
     * @brief 获取主应用区域（排除 DevTools 面板后的区域）
     * @param window_width 窗口宽度
     * @param window_height 窗口高度
     * @param out_x 输出 X 坐标
     * @param out_y 输出 Y 坐标
     * @param out_width 输出宽度
     * @param out_height 输出高度
     */
    void GetMainAppBounds(float window_width, float window_height,
                          float& out_x, float& out_y,
                          float& out_width, float& out_height) const;

    /**
     * @brief 获取 DevTools 面板区域
     */
    void GetPanelBounds(float window_width, float window_height,
                        float& out_x, float& out_y,
                        float& out_width, float& out_height) const;

    // ========== DOMObserver 接口实现 ==========
    
    /**
     * @brief 节点被添加时调用
     */
    void OnNodeAdded(Node* node, Node* parent) override;
    
    /**
     * @brief 节点被移除时调用
     */
    void OnNodeRemoved(Node* node, Node* parent) override;
    
    /**
     * @brief 元素属性被修改时调用
     */
    void OnAttributeChanged(Element* element, 
                           const std::string& name,
                           const std::string& old_value,
                           const std::string& new_value) override;
    
    /**
     * @brief 元素样式被修改时调用
     */
    void OnStyleChanged(Element* element,
                       const std::string& property,
                       const std::string& old_value,
                       const std::string& new_value) override;
    
    /**
     * @brief 文本内容被修改时调用
     */
    void OnTextChanged(Node* node,
                      const std::string& old_text,
                      const std::string& new_text) override;

private:
    DevToolsManager();
    ~DevToolsManager();

    // 禁止拷贝
    DevToolsManager(const DevToolsManager&) = delete;
    DevToolsManager& operator=(const DevToolsManager&) = delete;

    Document* document_ = nullptr;
    Window* window_ = nullptr;

    bool is_open_ = false;
    DockPosition dock_position_ = DockPosition::Bottom;
    float panel_size_ = 0.3f;  // 30% of window

    std::shared_ptr<Element> selected_element_;
    bool picker_active_ = false;
    bool dragging_panel_border_ = false;  // 是否正在拖动面板边界

    std::unique_ptr<DevToolsPanel> panel_;
    std::unique_ptr<ElementHighlighter> highlighter_;
    std::unique_ptr<ElementPicker> picker_;

public:
    /**
     * @brief 检查是否正在拖动面板边界
     */
    bool IsDraggingPanelBorder() const { return dragging_panel_border_; }

    /**
     * @brief 检查鼠标是否在面板边界上
     * @param x 鼠标 X 坐标（窗口坐标）
     * @param y 鼠标 Y 坐标（窗口坐标）
     * @param window_width 窗口宽度
     * @param window_height 窗口高度
     * @return 是否在面板边界上
     */
    bool IsMouseOnPanelBorder(float x, float y, float window_width, float window_height) const;

    /**
     * @brief 处理面板边界拖动
     * @param x 鼠标 X 坐标（窗口坐标）
     * @param y 鼠标 Y 坐标（窗口坐标）
     * @param window_width 窗口宽度
     * @param window_height 窗口高度
     * @param pressed 是否按下
     * @return 是否处理了事件
     */
    bool HandlePanelBorderDrag(float x, float y, float window_width, float window_height, bool pressed);

    /**
     * @brief 更新面板边界拖动
     * @param x 鼠标 X 坐标（窗口坐标）
     * @param y 鼠标 Y 坐标（窗口坐标）
     * @param window_width 窗口宽度
     * @param window_height 窗口高度
     * @return 是否需要重绘
     */
    bool UpdatePanelBorderDrag(float x, float y, float window_width, float window_height);
};

} // namespace mbink
