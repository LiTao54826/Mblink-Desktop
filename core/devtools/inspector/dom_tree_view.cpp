/**
 * @file dom_tree_view.cpp
 * @brief DOM 树视图组件实现
 */

#include "dom_tree_view.h"
#include "core/dom/text.h"
#include "core/render/text/font_manager.h"

#include "include/core/SkCanvas.h"
#include "include/core/SkPaint.h"
#include "include/core/SkRect.h"
#include "include/core/SkFont.h"
#include "include/core/SkFontMgr.h"
#include "include/core/SkPath.h"
#include "include/core/SkRRect.h"

#include <SDL3/SDL.h>
#include <iostream>
#include <algorithm>
#include <set>
#include <sstream>

namespace lightui {

namespace {
    const float ROW_HEIGHT = 20.0f;
    const float INDENT_WIDTH = 16.0f;
    const float EXPAND_ICON_SIZE = 12.0f;
    const float CONTEXT_MENU_WIDTH = 160.0f;
    const float CONTEXT_MENU_ITEM_HEIGHT = 24.0f;
}

DOMTreeView::DOMTreeView(Document* document)
    : document_(document) {
}

DOMTreeView::~DOMTreeView() = default;

void DOMTreeView::SetRootNode(std::shared_ptr<Node> root) {
    root_node_ = root;
    Refresh();
}

void DOMTreeView::Refresh() {
    // 重新计算内容高度
    if (root_node_) {
        content_height_ = CalculateContentHeight(root_node_, 0);
    }
}

void DOMTreeView::ExpandNode(std::shared_ptr<Node> node) {
    if (node) {
        node_states_[node.get()].expanded = true;
        Refresh();
    }
}

void DOMTreeView::CollapseNode(std::shared_ptr<Node> node) {
    if (node) {
        node_states_[node.get()].expanded = false;
        Refresh();
    }
}

void DOMTreeView::ExpandAll() {
    // 递归展开所有节点
    std::function<void(std::shared_ptr<Node>)> expand_recursive;
    expand_recursive = [this, &expand_recursive](std::shared_ptr<Node> node) {
        if (!node) return;
        node_states_[node.get()].expanded = true;
        for (const auto& child : node->GetChildNodes()) {
            expand_recursive(child);
        }
    };
    expand_recursive(root_node_);
    Refresh();
}

void DOMTreeView::CollapseAll() {
    for (auto& [node, state] : node_states_) {
        state.expanded = false;
    }
    Refresh();
}

bool DOMTreeView::IsExpanded(std::shared_ptr<Node> node) const {
    auto it = node_states_.find(node.get());
    if (it != node_states_.end()) {
        return it->second.expanded;
    }
    return true;  // 默认展开
}

void DOMTreeView::SelectNode(std::shared_ptr<Node> node) {
    if (selected_node_ != node) {
        selected_node_ = node;
        if (on_selection_changed_) {
            on_selection_changed_(node);
        } else {
        }
        // 滚动到可见
        ScrollToNode(node);
    }
}

void DOMTreeView::ScrollToNode(std::shared_ptr<Node> node) {
    if (!node || !root_node_) return;

    // 1. 确保所有祖先节点都已展开
    std::vector<std::shared_ptr<Node>> ancestors;
    auto parent = node->GetParentNode();
    while (parent) {
        ancestors.push_back(parent);
        parent = parent->GetParentNode();
    }
    // 从根到叶展开
    for (auto it = ancestors.rbegin(); it != ancestors.rend(); ++it) {
        node_states_[it->get()].expanded = true;
    }

    // 2. 重新计算内容高度（因为可能展开了新节点）
    content_height_ = CalculateContentHeight(root_node_, 0);

    // 3. 计算目标节点的 Y 位置
    float target_y = 0;
    bool found = false;

    std::function<void(std::shared_ptr<Node>, float&)> find_node_y;
    find_node_y = [this, &node, &found, &find_node_y](std::shared_ptr<Node> current, float& y) {
        if (!current || found) return;

        // 跳过纯空白的文本节点（与 RenderNode 逻辑一致）
        if (current->GetNodeType() == NodeType::TEXT_NODE) {
            auto text = std::dynamic_pointer_cast<Text>(current);
            if (text) {
                std::string content = text->GetData();
                bool is_whitespace_only = true;
                for (char c : content) {
                    if (c != ' ' && c != '\t' && c != '\n' && c != '\r') {
                        is_whitespace_only = false;
                        break;
                    }
                }
                if (is_whitespace_only) {
                    return;
                }
            }
        }

        if (current == node) {
            found = true;
            return;
        }

        y += ROW_HEIGHT;

        if (IsExpanded(current)) {
            for (const auto& child : current->GetChildNodes()) {
                find_node_y(child, y);
                if (found) return;
            }
        }
    };

    find_node_y(root_node_, target_y);

    if (!found) return;

    // 4. 调整滚动偏移使节点可见
    // 如果 view_height_ 还没有设置（首次渲染前），使用默认值
    float effective_view_height = view_height_ > 0 ? view_height_ : 300.0f;
    
    // 如果节点在可见区域上方，滚动到节点位置
    // 如果节点在可见区域下方，滚动使节点出现在底部
    float visible_top = scroll_offset_;
    float visible_bottom = scroll_offset_ + effective_view_height;

    if (target_y < visible_top) {
        // 节点在可见区域上方，滚动到节点位置（留一点边距）
        scroll_offset_ = std::max(0.0f, target_y - ROW_HEIGHT);
    } else if (target_y + ROW_HEIGHT > visible_bottom) {
        // 节点在可见区域下方，滚动使节点出现在底部
        float max_scroll = std::max(0.0f, content_height_ - effective_view_height);
        scroll_offset_ = std::min(max_scroll, target_y - effective_view_height + ROW_HEIGHT * 2);
    }
    // 如果节点已在可见区域内，不需要滚动
}

void DOMTreeView::SetSearchResults(const std::vector<std::shared_ptr<Node>>& results) {
    search_results_ = results;
}

void DOMTreeView::ClearSearchResults() {
    search_results_.clear();
}

void DOMTreeView::Render(SkCanvas* canvas, float x, float y, float width, float height) {
    view_x_ = x;
    view_y_ = y;
    view_width_ = width;
    view_height_ = height;

    // 裁剪区域
    canvas->save();
    canvas->clipRect(SkRect::MakeXYWH(x, y, width, height));

    // 背景
    SkPaint bg_paint;
    bg_paint.setColor(SkColorSetRGB(36, 36, 36));
    canvas->drawRect(SkRect::MakeXYWH(x, y, width, height), bg_paint);

    // 渲染节点前，重置所有节点的位置信息（保留展开状态）
    for (auto& [node_ptr, state] : node_states_) {
        state.y_position = -99999;  // 标记为无效位置
        state.height = 0;
    }

    // 渲染节点
    if (root_node_) {
        float current_y = y - scroll_offset_;
        RenderNode(canvas, root_node_, x + 4, current_y, 0);
    }

    // 渲染右键菜单（在裁剪区域内）
    RenderContextMenu(canvas);

    canvas->restore();
}

void DOMTreeView::RenderNode(SkCanvas* canvas, std::shared_ptr<Node> node,
                              float x, float& y, int depth) {
    if (!node) return;

    // 跳过纯空白的文本节点
    if (node->GetNodeType() == NodeType::TEXT_NODE) {
        auto text = std::dynamic_pointer_cast<Text>(node);
        if (text) {
            std::string content = text->GetData();
            // 检查是否只包含空白字符
            bool is_whitespace_only = true;
            for (char c : content) {
                if (c != ' ' && c != '\t' && c != '\n' && c != '\r') {
                    is_whitespace_only = false;
                    break;
                }
            }
            if (is_whitespace_only) {
                return;  // 跳过纯空白文本节点
            }
        }
    }

    // 始终保存节点位置（用于命中测试），无论是否可见
    // y 是当前节点的渲染 Y 坐标（已考虑滚动偏移）
    node_states_[node.get()].y_position = y;
    node_states_[node.get()].height = ROW_HEIGHT;

    // 检查节点是否在可见区域内
    bool is_above_view = (y + ROW_HEIGHT < view_y_);
    bool is_below_view = (y > view_y_ + view_height_);

    // 如果节点不在可见区域内，跳过渲染但继续遍历子节点以更新位置
    if (is_above_view || is_below_view) {
        y += ROW_HEIGHT;
        if (IsExpanded(node)) {
            for (const auto& child : node->GetChildNodes()) {
                RenderNode(canvas, child, x, y, depth + 1);
            }
        }
        return;
    }

    float node_x = x + depth * INDENT_WIDTH;

    // 选中背景
    if (node == selected_node_) {
        SkPaint sel_paint;
        sel_paint.setColor(SkColorSetRGB(51, 102, 153));
        canvas->drawRect(SkRect::MakeXYWH(view_x_, y, view_width_, ROW_HEIGHT), sel_paint);
    }

    // 搜索结果高亮
    if (IsSearchResult(node)) {
        SkPaint highlight_paint;
        highlight_paint.setColor(SkColorSetARGB(64, 255, 255, 0));
        canvas->drawRect(SkRect::MakeXYWH(view_x_, y, view_width_, ROW_HEIGHT), highlight_paint);
    }

    // 展开/折叠图标
    bool has_children = !node->GetChildNodes().empty();
    if (has_children) {
        SkPaint icon_paint;
        icon_paint.setColor(SkColorSetRGB(150, 150, 150));
        icon_paint.setAntiAlias(true);

        float icon_x = node_x;
        float icon_y = y + (ROW_HEIGHT - EXPAND_ICON_SIZE) / 2;

        if (IsExpanded(node)) {
            // 向下箭头 ▼
            SkPath path;
            path.moveTo(icon_x, icon_y + 3);
            path.lineTo(icon_x + EXPAND_ICON_SIZE, icon_y + 3);
            path.lineTo(icon_x + EXPAND_ICON_SIZE / 2, icon_y + EXPAND_ICON_SIZE - 3);
            path.close();
            canvas->drawPath(path, icon_paint);
        } else {
            // 向右箭头 ▶
            SkPath path;
            path.moveTo(icon_x + 3, icon_y);
            path.lineTo(icon_x + EXPAND_ICON_SIZE - 3, icon_y + EXPAND_ICON_SIZE / 2);
            path.lineTo(icon_x + 3, icon_y + EXPAND_ICON_SIZE);
            path.close();
            canvas->drawPath(path, icon_paint);
        }
    }

    // 节点标签
    float text_x = node_x + (has_children ? EXPAND_ICON_SIZE + 4 : 0);
    std::string label = FormatNodeLabel(node);

    // 使用 FontManager 获取字体（使用支持中文的字体）
    FontDescriptor font_desc;
    font_desc.family = "Microsoft YaHei";  // 微软雅黑同时支持中英文
    font_desc.size = 12.0f;
    font_desc.weight = FontWeight::NORMAL;
    font_desc.style = FontStyle::NORMAL;
    SkFont font = FontManager::GetInstance().LoadFont(font_desc);

    SkPaint text_paint;
    text_paint.setAntiAlias(true);

    // 根据节点类型设置颜色
    if (node->GetNodeType() == NodeType::ELEMENT_NODE) {
        // 元素节点：标签名用紫色
        text_paint.setColor(SkColorSetRGB(136, 106, 168));
    } else if (node->GetNodeType() == NodeType::TEXT_NODE) {
        // 文本节点：灰色
        text_paint.setColor(SkColorSetRGB(150, 150, 150));
    } else {
        text_paint.setColor(SkColorSetRGB(200, 200, 200));
    }

    canvas->drawString(label.c_str(), text_x, y + ROW_HEIGHT - 5, font, text_paint);

    y += ROW_HEIGHT;

    // 递归渲染子节点
    if (has_children && IsExpanded(node)) {
        for (const auto& child : node->GetChildNodes()) {
            RenderNode(canvas, child, x, y, depth + 1);
        }
    }
}

std::string DOMTreeView::FormatNodeLabel(std::shared_ptr<Node> node) {
    if (!node) return "";

    if (node->GetNodeType() == NodeType::ELEMENT_NODE) {
        auto element = std::dynamic_pointer_cast<Element>(node);
        if (!element) return "";

        std::string label = "<" + element->GetTagName();

        // 添加 id
        std::string id = element->GetAttribute("id");
        if (!id.empty()) {
            label += " id=\"" + id + "\"";
        }

        // 添加 class
        std::string cls = element->GetAttribute("class");
        if (!cls.empty()) {
            label += " class=\"" + cls + "\"";
        }

        label += ">";
        return label;
    } else if (node->GetNodeType() == NodeType::TEXT_NODE) {
        auto text = std::dynamic_pointer_cast<Text>(node);
        if (text) {
            std::string content = text->GetData();
            // 截断长文本
            if (content.length() > 50) {
                content = content.substr(0, 47) + "...";
            }
            // 替换换行符
            for (char& c : content) {
                if (c == '\n' || c == '\r') c = ' ';
            }
            return "\"" + content + "\"";
        }
    }

    // 返回节点类型名称
    switch (node->GetNodeType()) {
        case NodeType::ELEMENT_NODE: return "#element";
        case NodeType::TEXT_NODE: return "#text";
        case NodeType::DOCUMENT_NODE: return "#document";
        default: return "#node";
    }
}

bool DOMTreeView::IsSearchResult(std::shared_ptr<Node> node) const {
    for (const auto& result : search_results_) {
        if (result == node) return true;
    }
    return false;
}

float DOMTreeView::CalculateContentHeight(std::shared_ptr<Node> node, int depth) {
    if (!node) return 0;

    float height = ROW_HEIGHT;

    if (IsExpanded(node)) {
        for (const auto& child : node->GetChildNodes()) {
            height += CalculateContentHeight(child, depth + 1);
        }
    }

    return height;
}

bool DOMTreeView::HandleMouseEvent(int mouse_x, int mouse_y, int button, bool pressed) {
    // mouse_x, mouse_y 是相对于 DOM 树视图的坐标（已由调用者转换）
    // 检查是否在视图区域内（使用相对坐标）
    if (mouse_x < 0 || mouse_x > view_width_ ||
        mouse_y < 0 || mouse_y > view_height_) {
        // 点击视图外部时隐藏右键菜单
        if (pressed && context_menu_visible_) {
            context_menu_visible_ = false;
            return true;
        }
        return false;
    }

    // 如果右键菜单可见，优先处理菜单点击
    if (context_menu_visible_ && pressed && button == 0) {
        if (HandleContextMenuClick(static_cast<float>(mouse_x), static_cast<float>(mouse_y))) {
            return true;
        }
        // 点击菜单外部，隐藏菜单
        context_menu_visible_ = false;
        return true;
    }

    if (!pressed) return false;

    // 右键点击 - 显示上下文菜单
    if (button == 1) {
        float abs_x = view_x_ + mouse_x;
        float abs_y = view_y_ + mouse_y;
        auto hit_node = HitTest(abs_x, abs_y);
        if (hit_node) {
            SelectNode(hit_node);
            ShowContextMenu(static_cast<float>(mouse_x), static_cast<float>(mouse_y), hit_node);
            return true;
        }
        return false;
    }

    // 左键点击
    if (button != 0) return false;

    // 命中测试 - 转换为绝对坐标
    float abs_x = view_x_ + mouse_x;
    float abs_y = view_y_ + mouse_y;
    
    auto hit_node = HitTest(abs_x, abs_y);
    if (hit_node) {
        // 隐藏右键菜单
        context_menu_visible_ = false;
        
        // 检查是否点击了展开图标
        float node_x = 4;  // 相对于视图的 x 坐标

        // 切换展开状态或选择节点
        if (!hit_node->GetChildNodes().empty() && mouse_x < node_x + EXPAND_ICON_SIZE + 20) {
            if (IsExpanded(hit_node)) {
                CollapseNode(hit_node);
            } else {
                ExpandNode(hit_node);
            }
        } else {
            SelectNode(hit_node);
        }
        return true;
    }

    return false;
}

bool DOMTreeView::HandleKeyboardEvent(int key, bool pressed) {
    // TODO: 实现键盘导航
    return false;
}

bool DOMTreeView::HandleMouseWheel(float delta_y) {
    // 滚动速度
    const float scroll_speed = 30.0f;
    float new_offset = scroll_offset_ - delta_y * scroll_speed;
    
    // 限制滚动范围
    float max_scroll = std::max(0.0f, content_height_ - view_height_);
    new_offset = std::max(0.0f, std::min(new_offset, max_scroll));
    
    if (new_offset != scroll_offset_) {
        scroll_offset_ = new_offset;
        return true;
    }
    return false;
}

void DOMTreeView::SetScrollOffset(float offset) {
    float max_scroll = std::max(0.0f, content_height_ - view_height_);
    scroll_offset_ = std::max(0.0f, std::min(offset, max_scroll));
}

std::shared_ptr<Node> DOMTreeView::HitTest(float x, float y) {
    for (const auto& [node_ptr, state] : node_states_) {
        // 跳过无效位置的节点（位置未在本次渲染中更新）
        if (state.height <= 0) continue;
        
        if (y >= state.y_position && y < state.y_position + state.height) {
            // 找到对应的 shared_ptr，需要遍历 DOM 树
            std::function<std::shared_ptr<Node>(std::shared_ptr<Node>)> find_node;
            find_node = [&find_node, node_ptr](std::shared_ptr<Node> node) -> std::shared_ptr<Node> {
                if (!node) return nullptr;
                if (node.get() == node_ptr) return node;
                for (const auto& child : node->GetChildNodes()) {
                    auto found = find_node(child);
                    if (found) return found;
                }
                return nullptr;
            };
            return find_node(root_node_);
        }
    }
    return nullptr;
}

void DOMTreeView::ShowContextMenu(float x, float y, std::shared_ptr<Node> node) {
    context_menu_visible_ = true;
    context_menu_x_ = x;
    context_menu_y_ = y;
    context_menu_node_ = node;
    
    // 确保菜单不超出视图边界
    float menu_height = CONTEXT_MENU_ITEM_HEIGHT * 5;  // 5 个菜单项
    if (context_menu_y_ + menu_height > view_height_) {
        context_menu_y_ = view_height_ - menu_height - 4;
    }
    if (context_menu_x_ + CONTEXT_MENU_WIDTH > view_width_) {
        context_menu_x_ = view_width_ - CONTEXT_MENU_WIDTH - 4;
    }
}

void DOMTreeView::RenderContextMenu(SkCanvas* canvas) {
    if (!context_menu_visible_) return;

    const float menu_height = CONTEXT_MENU_ITEM_HEIGHT * 5;
    const float menu_x = view_x_ + context_menu_x_;
    const float menu_y = view_y_ + context_menu_y_;

    // 菜单背景
    SkPaint bg_paint;
    bg_paint.setColor(SkColorSetRGB(50, 50, 50));
    SkRect menu_rect = SkRect::MakeXYWH(menu_x, menu_y, CONTEXT_MENU_WIDTH, menu_height);
    canvas->drawRRect(SkRRect::MakeRectXY(menu_rect, 4, 4), bg_paint);

    // 边框
    SkPaint border_paint;
    border_paint.setColor(SkColorSetRGB(80, 80, 80));
    border_paint.setStyle(SkPaint::kStroke_Style);
    border_paint.setStrokeWidth(1);
    canvas->drawRRect(SkRRect::MakeRectXY(menu_rect, 4, 4), border_paint);

    // 菜单项
    FontDescriptor font_desc;
    font_desc.family = "Microsoft YaHei";
    font_desc.size = 12.0f;
    font_desc.weight = FontWeight::NORMAL;
    font_desc.style = FontStyle::NORMAL;
    SkFont font = FontManager::GetInstance().LoadFont(font_desc);

    SkPaint text_paint;
    text_paint.setColor(SkColorSetRGB(220, 220, 220));
    text_paint.setAntiAlias(true);

    const char* menu_items[] = {
        "Copy outerHTML",
        "Copy innerHTML",
        "Copy selector",
        "Expand all",
        "Collapse all"
    };

    for (int i = 0; i < 5; i++) {
        float item_y = menu_y + i * CONTEXT_MENU_ITEM_HEIGHT;
        canvas->drawString(menu_items[i], menu_x + 12, item_y + 17, font, text_paint);
    }
}

bool DOMTreeView::HandleContextMenuClick(float x, float y) {
    if (!context_menu_visible_) return false;

    // 检查点击是否在菜单区域内
    float menu_height = CONTEXT_MENU_ITEM_HEIGHT * 5;
    if (x < context_menu_x_ || x > context_menu_x_ + CONTEXT_MENU_WIDTH ||
        y < context_menu_y_ || y > context_menu_y_ + menu_height) {
        return false;
    }

    // 计算点击的菜单项索引
    int item_index = static_cast<int>((y - context_menu_y_) / CONTEXT_MENU_ITEM_HEIGHT);
    if (item_index < 0 || item_index >= 5) return false;

    context_menu_visible_ = false;

    switch (item_index) {
        case 0: ExecuteContextMenuAction(ContextMenuItem::CopyOuterHTML); break;
        case 1: ExecuteContextMenuAction(ContextMenuItem::CopyInnerHTML); break;
        case 2: ExecuteContextMenuAction(ContextMenuItem::CopySelector); break;
        case 3: ExecuteContextMenuAction(ContextMenuItem::ExpandAll); break;
        case 4: ExecuteContextMenuAction(ContextMenuItem::CollapseAll); break;
    }

    return true;
}

void DOMTreeView::ExecuteContextMenuAction(ContextMenuItem item) {
    switch (item) {
        case ContextMenuItem::CopyOuterHTML: {
            if (context_menu_node_) {
                std::string html = GetOuterHTML(context_menu_node_);
                SDL_SetClipboardText(html.c_str());
            }
            break;
        }
        case ContextMenuItem::CopyInnerHTML: {
            if (context_menu_node_) {
                std::string html = GetInnerHTML(context_menu_node_);
                SDL_SetClipboardText(html.c_str());
            }
            break;
        }
        case ContextMenuItem::CopySelector: {
            if (context_menu_node_) {
                std::string selector = GetCSSSelector(context_menu_node_);
                SDL_SetClipboardText(selector.c_str());
            }
            break;
        }
        case ContextMenuItem::ExpandAll: {
            ExpandAll();
            break;
        }
        case ContextMenuItem::CollapseAll: {
            CollapseAll();
            break;
        }
    }
}

std::string DOMTreeView::GetOuterHTML(std::shared_ptr<Node> node) {
    if (!node) return "";

    std::ostringstream oss;

    if (node->GetNodeType() == NodeType::ELEMENT_NODE) {
        auto element = std::dynamic_pointer_cast<Element>(node);
        if (!element) return "";

        std::string tag = element->GetTagName();
        // 转换为小写
        std::transform(tag.begin(), tag.end(), tag.begin(), ::tolower);

        oss << "<" << tag;

        // 添加属性
        const auto& attrs = element->GetAllAttributes();
        for (const auto& [name, value] : attrs) {
            oss << " " << name << "=\"" << value << "\"";
        }

        // 自闭合标签
        static const std::set<std::string> void_elements = {
            "area", "base", "br", "col", "embed", "hr", "img", "input",
            "link", "meta", "param", "source", "track", "wbr"
        };

        if (void_elements.count(tag)) {
            oss << " />";
        } else {
            oss << ">";
            oss << GetInnerHTML(node);
            oss << "</" << tag << ">";
        }
    } else if (node->GetNodeType() == NodeType::TEXT_NODE) {
        auto text = std::dynamic_pointer_cast<Text>(node);
        if (text) {
            oss << text->GetData();
        }
    }

    return oss.str();
}

std::string DOMTreeView::GetInnerHTML(std::shared_ptr<Node> node) {
    if (!node) return "";

    std::ostringstream oss;
    for (const auto& child : node->GetChildNodes()) {
        oss << GetOuterHTML(child);
    }
    return oss.str();
}

std::string DOMTreeView::GetCSSSelector(std::shared_ptr<Node> node) {
    if (!node || node->GetNodeType() != NodeType::ELEMENT_NODE) return "";

    auto element = std::dynamic_pointer_cast<Element>(node);
    if (!element) return "";

    std::string tag = element->GetTagName();
    std::transform(tag.begin(), tag.end(), tag.begin(), ::tolower);

    // 如果有 id，直接返回 #id
    std::string id = element->GetAttribute("id");
    if (!id.empty()) {
        return "#" + id;
    }

    // 构建选择器路径
    std::string selector = tag;

    // 添加 class
    std::string cls = element->GetAttribute("class");
    if (!cls.empty()) {
        // 只取第一个 class
        size_t space_pos = cls.find(' ');
        if (space_pos != std::string::npos) {
            cls = cls.substr(0, space_pos);
        }
        selector += "." + cls;
    }

    // 如果需要更精确，可以添加 :nth-child
    auto parent = node->GetParentNode();
    if (parent) {
        int index = 1;
        for (const auto& sibling : parent->GetChildNodes()) {
            if (sibling == node) break;
            if (sibling->GetNodeType() == NodeType::ELEMENT_NODE) {
                auto sib_elem = std::dynamic_pointer_cast<Element>(sibling);
                if (sib_elem && sib_elem->GetTagName() == element->GetTagName()) {
                    index++;
                }
            }
        }
        // 检查是否有同名兄弟节点
        bool has_same_tag_sibling = false;
        for (const auto& sibling : parent->GetChildNodes()) {
            if (sibling != node && sibling->GetNodeType() == NodeType::ELEMENT_NODE) {
                auto sib_elem = std::dynamic_pointer_cast<Element>(sibling);
                if (sib_elem && sib_elem->GetTagName() == element->GetTagName()) {
                    has_same_tag_sibling = true;
                    break;
                }
            }
        }
        if (has_same_tag_sibling) {
            selector += ":nth-of-type(" + std::to_string(index) + ")";
        }
    }

    return selector;
}

} // namespace lightui
