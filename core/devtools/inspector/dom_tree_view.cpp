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

#include <iostream>
#include <algorithm>

namespace lightui {

namespace {
    const float ROW_HEIGHT = 20.0f;
    const float INDENT_WIDTH = 16.0f;
    const float EXPAND_ICON_SIZE = 12.0f;
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
        std::cout << "[DOMTreeView] SelectNode: " << FormatNodeLabel(node) << std::endl;
        if (on_selection_changed_) {
            std::cout << "[DOMTreeView] Calling on_selection_changed_ callback" << std::endl;
            on_selection_changed_(node);
        } else {
            std::cout << "[DOMTreeView] WARNING: on_selection_changed_ is not set!" << std::endl;
        }
        // 滚动到可见
        ScrollToNode(node);
    }
}

void DOMTreeView::ScrollToNode(std::shared_ptr<Node> node) {
    // TODO: 实现滚动到节点
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
    if (!pressed || button != 0) return false;

    // mouse_x, mouse_y 是相对于 DOM 树视图的坐标（已由调用者转换）
    // 检查是否在视图区域内（使用相对坐标）
    if (mouse_x < 0 || mouse_x > view_width_ ||
        mouse_y < 0 || mouse_y > view_height_) {
        return false;
    }

    // 命中测试 - 转换为绝对坐标
    float abs_x = view_x_ + mouse_x;
    float abs_y = view_y_ + mouse_y;
    
    auto hit_node = HitTest(abs_x, abs_y);
    if (hit_node) {
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

} // namespace lightui
