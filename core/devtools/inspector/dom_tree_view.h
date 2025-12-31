/**
 * @file dom_tree_view.h
 * @brief DOM 树视图组件
 */

#pragma once

#include <memory>
#include <functional>
#include <unordered_map>
#include <vector>
#include "core/dom/document.h"
#include "core/dom/node.h"
#include "core/dom/element.h"

class SkCanvas;

namespace lightui {

class Event;

/**
 * @brief DOM 树视图
 */
class DOMTreeView {
public:
    explicit DOMTreeView(Document* document);
    ~DOMTreeView();

    // 树操作
    void SetRootNode(std::shared_ptr<Node> root);
    void Refresh();

    // 节点展开/折叠
    void ExpandNode(std::shared_ptr<Node> node);
    void CollapseNode(std::shared_ptr<Node> node);
    void ExpandAll();
    void CollapseAll();
    bool IsExpanded(std::shared_ptr<Node> node) const;

    // 选择
    void SelectNode(std::shared_ptr<Node> node);
    std::shared_ptr<Node> GetSelectedNode() const { return selected_node_; }

    // 滚动到可见
    void ScrollToNode(std::shared_ptr<Node> node);

    // 搜索高亮
    void SetSearchResults(const std::vector<std::shared_ptr<Node>>& results);
    void ClearSearchResults();

    // 渲染
    void Render(SkCanvas* canvas, float x, float y, float width, float height);

    // 事件
    bool HandleMouseEvent(int mouse_x, int mouse_y, int button, bool pressed);
    bool HandleKeyboardEvent(int key, bool pressed);
    bool HandleMouseWheel(float delta_y);
    
    // 滚动
    void SetScrollOffset(float offset);
    float GetScrollOffset() const { return scroll_offset_; }
    float GetContentHeight() const { return content_height_; }
    float GetViewHeight() const { return view_height_; }

    // 回调
    using SelectionCallback = std::function<void(std::shared_ptr<Node>)>;
    void SetOnSelectionChanged(SelectionCallback callback) {
        on_selection_changed_ = callback;
    }

    // 右键菜单
    bool IsContextMenuVisible() const { return context_menu_visible_; }
    void HideContextMenu() { context_menu_visible_ = false; }

private:
    struct TreeNodeState {
        bool expanded = true;  // 默认展开
        float y_position = 0;
        float height = 0;
    };

    // 右键菜单项
    enum class ContextMenuItem {
        CopyOuterHTML,
        CopyInnerHTML,
        CopySelector,
        ExpandAll,
        CollapseAll
    };

    Document* document_;
    std::shared_ptr<Node> root_node_;
    std::shared_ptr<Node> selected_node_;
    std::shared_ptr<Node> hovered_node_;
    std::shared_ptr<Node> context_menu_node_;  // 右键菜单对应的节点

    std::unordered_map<Node*, TreeNodeState> node_states_;
    std::vector<std::shared_ptr<Node>> search_results_;

    float scroll_offset_ = 0;
    float content_height_ = 0;
    float view_x_ = 0;
    float view_y_ = 0;
    float view_width_ = 0;
    float view_height_ = 0;

    // 右键菜单状态
    bool context_menu_visible_ = false;
    float context_menu_x_ = 0;
    float context_menu_y_ = 0;

    SelectionCallback on_selection_changed_;

    // 渲染辅助
    void RenderNode(SkCanvas* canvas, std::shared_ptr<Node> node,
                    float x, float& y, int depth);
    std::string FormatNodeLabel(std::shared_ptr<Node> node);
    bool IsSearchResult(std::shared_ptr<Node> node) const;

    // 计算内容高度
    float CalculateContentHeight(std::shared_ptr<Node> node, int depth);

    // 命中测试
    std::shared_ptr<Node> HitTest(float x, float y);

    // 右键菜单
    void ShowContextMenu(float x, float y, std::shared_ptr<Node> node);
    void RenderContextMenu(SkCanvas* canvas);
    bool HandleContextMenuClick(float x, float y);
    void ExecuteContextMenuAction(ContextMenuItem item);
    
    // HTML 序列化
    std::string GetOuterHTML(std::shared_ptr<Node> node);
    std::string GetInnerHTML(std::shared_ptr<Node> node);
    std::string GetCSSSelector(std::shared_ptr<Node> node);
};

} // namespace lightui
