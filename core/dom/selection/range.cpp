/**
 * @file range.cpp
 * @brief DOM Range API 实现
 */

#include "range.h"
#include "core/dom/document.h"
#include "core/dom/element.h"
#include "core/dom/node.h"
#include "core/dom/text.h"
#include "core/render/objects/render_object.h"
#include "core/render/text/font_manager.h"
#include "include/core/SkFont.h"
#include "include/core/SkTextBlob.h"
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace lightui {

Range::Range(std::shared_ptr<Document> owner_document)
    : owner_document_(owner_document)
    , start_offset_(0)
    , end_offset_(0) {
    // 默认情况下，Range 的边界设置为文档本身
    if (owner_document) {
        start_container_ = owner_document;
        end_container_ = owner_document;
    }
}

Range::~Range() = default;

// ========== 边界设置方法 ==========

void Range::SetStart(std::shared_ptr<Node> node, int offset) {
    if (!node) {
        throw std::invalid_argument("Node cannot be null");
    }

    int max_offset = GetNodeLength(node);
    if (offset < 0 || offset > max_offset) {
        throw std::out_of_range("Offset out of range");
    }

    start_container_ = node;
    start_offset_ = offset;

    // 如果起始位置在结束位置之后，将结束位置设置为起始位置
    auto end_node = end_container_.lock();
    if (!end_node) {
        end_container_ = node;
        end_offset_ = offset;
    }
}

void Range::SetEnd(std::shared_ptr<Node> node, int offset) {
    if (!node) {
        throw std::invalid_argument("Node cannot be null");
    }

    int max_offset = GetNodeLength(node);
    if (offset < 0 || offset > max_offset) {
        throw std::out_of_range("Offset out of range");
    }

    end_container_ = node;
    end_offset_ = offset;

    // 如果结束位置在起始位置之前，将起始位置设置为结束位置
    auto start_node = start_container_.lock();
    if (!start_node) {
        start_container_ = node;
        start_offset_ = offset;
    }
}

void Range::SetStartBefore(std::shared_ptr<Node> node) {
    if (!node) {
        throw std::invalid_argument("Node cannot be null");
    }

    auto parent = node->GetParentNode();
    if (!parent) {
        throw std::invalid_argument("Node has no parent");
    }

    int index = GetNodeIndex(node);
    SetStart(parent, index);
}

void Range::SetStartAfter(std::shared_ptr<Node> node) {
    if (!node) {
        throw std::invalid_argument("Node cannot be null");
    }

    auto parent = node->GetParentNode();
    if (!parent) {
        throw std::invalid_argument("Node has no parent");
    }

    int index = GetNodeIndex(node);
    SetStart(parent, index + 1);
}

void Range::SetEndBefore(std::shared_ptr<Node> node) {
    if (!node) {
        throw std::invalid_argument("Node cannot be null");
    }

    auto parent = node->GetParentNode();
    if (!parent) {
        throw std::invalid_argument("Node has no parent");
    }

    int index = GetNodeIndex(node);
    SetEnd(parent, index);
}

void Range::SetEndAfter(std::shared_ptr<Node> node) {
    if (!node) {
        throw std::invalid_argument("Node cannot be null");
    }

    auto parent = node->GetParentNode();
    if (!parent) {
        throw std::invalid_argument("Node has no parent");
    }

    int index = GetNodeIndex(node);
    SetEnd(parent, index + 1);
}

// ========== 选择方法 ==========

void Range::SelectNode(std::shared_ptr<Node> node) {
    if (!node) {
        throw std::invalid_argument("Node cannot be null");
    }

    auto parent = node->GetParentNode();
    if (!parent) {
        throw std::invalid_argument("Node has no parent");
    }

    int index = GetNodeIndex(node);
    start_container_ = parent;
    start_offset_ = index;
    end_container_ = parent;
    end_offset_ = index + 1;
}

void Range::SelectNodeContents(std::shared_ptr<Node> node) {
    if (!node) {
        throw std::invalid_argument("Node cannot be null");
    }

    start_container_ = node;
    start_offset_ = 0;
    end_container_ = node;
    end_offset_ = GetNodeLength(node);
}

void Range::Collapse(bool to_start) {
    if (to_start) {
        end_container_ = start_container_;
        end_offset_ = start_offset_;
    } else {
        start_container_ = end_container_;
        start_offset_ = end_offset_;
    }
}

// ========== 属性访问 ==========

bool Range::IsCollapsed() const {
    auto start_node = start_container_.lock();
    auto end_node = end_container_.lock();

    if (!start_node || !end_node) {
        return true;
    }

    return start_node == end_node && start_offset_ == end_offset_;
}

std::shared_ptr<Node> Range::GetCommonAncestorContainer() const {
    auto start_node = start_container_.lock();
    auto end_node = end_container_.lock();

    if (!start_node || !end_node) {
        return nullptr;
    }

    if (start_node == end_node) {
        return start_node;
    }

    return FindCommonAncestor(start_node, end_node);
}

// ========== 克隆和转换 ==========

std::shared_ptr<Range> Range::CloneRange() const {
    auto doc = owner_document_.lock();
    auto cloned = std::make_shared<Range>(doc);

    cloned->start_container_ = start_container_;
    cloned->start_offset_ = start_offset_;
    cloned->end_container_ = end_container_;
    cloned->end_offset_ = end_offset_;

    return cloned;
}

std::string Range::ToString() const {
    auto start_node = start_container_.lock();
    auto end_node = end_container_.lock();

    if (!start_node || !end_node) {
        return "";
    }

    // 如果起始和结束在同一个文本节点
    if (start_node == end_node && start_node->GetNodeType() == NodeType::TEXT_NODE) {
        auto text_node = std::dynamic_pointer_cast<Text>(start_node);
        if (text_node) {
            std::string content = text_node->GetData();
            int start = std::max(0, start_offset_);
            int end = std::min(static_cast<int>(content.length()), end_offset_);
            if (start < end) {
                return content.substr(start, end - start);
            }
        }
        return "";
    }

    // 复杂情况：跨越多个节点
    std::string result;
    bool in_range = false;
    auto common_ancestor = GetCommonAncestorContainer();
    if (common_ancestor) {
        CollectText(common_ancestor, result, in_range);
    }

    return result;
}

// ========== 几何信息 ==========

/**
 * @brief 计算文本节点在指定偏移范围内的矩形
 * @param text_node 文本节点
 * @param start_offset 起始偏移
 * @param end_offset 结束偏移
 * @return 矩形区域
 */
Range::DOMRect Range::ComputeTextRect(
    std::shared_ptr<Text> text_node,
    int start_offset,
    int end_offset) const {
    
    DOMRect rect;
    if (!text_node) {
        return rect;
    }
    
    // 获取父元素
    auto parent = text_node->GetParentNode();
    if (!parent) {
        return rect;
    }
    
    auto element = std::dynamic_pointer_cast<Element>(parent);
    if (!element) {
        return rect;
    }
    
    // 获取元素的边界矩形
    auto elem_rect = element->GetBoundingClientRect();
    
    // 获取字体信息
    float font_size = 14.0f;
    float line_height = font_size * 1.4f;
    std::string font_family = "monospace";
    float padding_left = 0.0f;
    float padding_top = 0.0f;
    
    auto render_obj = element->GetRenderObject();
    if (render_obj) {
        const auto& computed = render_obj->GetComputedStyle();
        font_size = computed.font_size;
        line_height = font_size * 1.4f;
        if (!computed.font_family.empty()) {
            font_family = computed.font_family;
        }
        padding_left = computed.padding_left.ToPx();
        padding_top = computed.padding_top.ToPx();
    }
    
    // 使用 FontManager 测量文本
    FontDescriptor desc;
    desc.family = font_family;
    desc.size = font_size;
    desc.weight = FontWeight::NORMAL;
    desc.style = FontStyle::NORMAL;
    
    SkFont font = FontManager::GetInstance().LoadFont(desc);
    
    // 获取文本内容
    std::string full_text = text_node->GetData();
    int text_len = static_cast<int>(full_text.length());
    
    // 限制偏移范围
    start_offset = std::max(0, std::min(start_offset, text_len));
    end_offset = std::max(start_offset, std::min(end_offset, text_len));
    
    // 计算起始偏移的 x 位置
    float start_x = elem_rect.x + padding_left;
    if (start_offset > 0) {
        std::string prefix = full_text.substr(0, start_offset);
        float prefix_width = font.measureText(
            prefix.c_str(), prefix.size(), SkTextEncoding::kUTF8, nullptr);
        start_x = elem_rect.x + padding_left + prefix_width;
    }
    
    // 计算范围的宽度
    float width = 0;
    int char_count = end_offset - start_offset;
    if (char_count > 0) {
        std::string range_text = full_text.substr(start_offset, char_count);
        width = font.measureText(
            range_text.c_str(), range_text.size(), SkTextEncoding::kUTF8, nullptr);
    }
    
    rect.x = start_x;
    rect.left = start_x;
    rect.y = elem_rect.y + padding_top;
    rect.top = elem_rect.y + padding_top;
    rect.width = width;
    rect.height = line_height;
    rect.right = start_x + width;
    rect.bottom = elem_rect.y + padding_top + line_height;
    
    return rect;
}

/**
 * @brief 合并两个矩形
 */
void Range::UnionRect(DOMRect& result, const DOMRect& other) const {
    if (other.width <= 0 && other.height <= 0) {
        return;
    }
    
    if (result.width <= 0 && result.height <= 0) {
        result = other;
        return;
    }
    
    float min_left = std::min(result.left, other.left);
    float min_top = std::min(result.top, other.top);
    float max_right = std::max(result.right, other.right);
    float max_bottom = std::max(result.bottom, other.bottom);
    
    result.x = min_left;
    result.left = min_left;
    result.y = min_top;
    result.top = min_top;
    result.right = max_right;
    result.bottom = max_bottom;
    result.width = max_right - min_left;
    result.height = max_bottom - min_top;
}

/**
 * @brief 深度优先遍历收集 Range 内所有节点的矩形
 */
void Range::CollectRects(
    std::shared_ptr<Node> node,
    std::vector<DOMRect>& rects,
    bool& in_range,
    bool& done) const {
    
    if (!node || done) {
        return;
    }
    
    auto start_node = start_container_.lock();
    auto end_node = end_container_.lock();
    
    // 检查是否是起始节点
    if (node == start_node) {
        in_range = true;
        
        if (node->GetNodeType() == NodeType::TEXT_NODE) {
            auto text_node = std::dynamic_pointer_cast<Text>(node);
            if (text_node) {
                int text_len = static_cast<int>(text_node->GetData().length());
                int start_off = start_offset_;
                int end_off = (start_node == end_node) ? end_offset_ : text_len;
                
                auto rect = ComputeTextRect(text_node, start_off, end_off);
                if (rect.width > 0 || rect.height > 0) {
                    rects.push_back(rect);
                }
                
                if (start_node == end_node) {
                    done = true;
                    return;
                }
            }
        }
    }
    // 检查是否是结束节点
    else if (node == end_node) {
        // 如果结束节点是元素节点，需要处理其子节点
        if (node->GetNodeType() == NodeType::ELEMENT_NODE) {
            // 结束在元素节点上，offset 表示子节点索引
            // 需要包含 offset 之前的所有子节点
            const auto& children = node->GetChildNodes();
            for (int i = 0; i < end_offset_ && i < static_cast<int>(children.size()); ++i) {
                auto child = children[i];
                if (child->GetNodeType() == NodeType::TEXT_NODE) {
                    auto text_node = std::dynamic_pointer_cast<Text>(child);
                    if (text_node) {
                        int text_len = static_cast<int>(text_node->GetData().length());
                        auto rect = ComputeTextRect(text_node, 0, text_len);
                        if (rect.width > 0 || rect.height > 0) {
                            rects.push_back(rect);
                        }
                    }
                }
            }
        } else if (node->GetNodeType() == NodeType::TEXT_NODE) {
            auto text_node = std::dynamic_pointer_cast<Text>(node);
            if (text_node) {
                auto rect = ComputeTextRect(text_node, 0, end_offset_);
                if (rect.width > 0 || rect.height > 0) {
                    rects.push_back(rect);
                }
            }
        }
        done = true;
        return;
    }
    // 在范围内的文本节点
    else if (in_range && node->GetNodeType() == NodeType::TEXT_NODE) {
        auto text_node = std::dynamic_pointer_cast<Text>(node);
        if (text_node) {
            int text_len = static_cast<int>(text_node->GetData().length());
            auto rect = ComputeTextRect(text_node, 0, text_len);
            if (rect.width > 0 || rect.height > 0) {
                rects.push_back(rect);
            }
        }
    }
    
    // 递归处理子节点
    for (const auto& child : node->GetChildNodes()) {
        if (done) {
            break;
        }
        CollectRects(child, rects, in_range, done);
    }
}

Range::DOMRect Range::GetBoundingClientRect() const {
    DOMRect rect;
    
    auto start_node = start_container_.lock();
    auto end_node = end_container_.lock();
    
    if (!start_node || !end_node) {
        return rect;
    }
    
    // 强制同步布局
    auto doc = owner_document_.lock();
    if (doc) {
        doc->ForceLayout();
    }
    
    // 获取所有矩形
    auto rects = GetClientRects();
    
    // 合并所有矩形
    for (const auto& r : rects) {
        UnionRect(rect, r);
    }
    
    // 如果没有找到任何矩形，尝试返回折叠位置的矩形
    if (rects.empty() && start_node->GetNodeType() == NodeType::TEXT_NODE) {
        auto text_node = std::dynamic_pointer_cast<Text>(start_node);
        if (text_node) {
            // 对于折叠的 Range，返回光标位置
            rect = ComputeTextRect(text_node, start_offset_, start_offset_);
        }
    }
    
    return rect;
}

std::vector<Range::DOMRect> Range::GetClientRects() const {
    std::vector<DOMRect> rects;
    
    auto start_node = start_container_.lock();
    auto end_node = end_container_.lock();
    
    if (!start_node || !end_node) {
        return rects;
    }
    
    // 强制同步布局
    auto doc = owner_document_.lock();
    if (doc) {
        doc->ForceLayout();
    }
    
    // 获取公共祖先
    auto common_ancestor = GetCommonAncestorContainer();
    if (!common_ancestor) {
        return rects;
    }
    
    // 收集所有矩形
    bool in_range = false;
    bool done = false;
    CollectRects(common_ancestor, rects, in_range, done);
    
    // 如果没有找到任何矩形，尝试返回折叠位置的矩形
    if (rects.empty() && start_node->GetNodeType() == NodeType::TEXT_NODE) {
        auto text_node = std::dynamic_pointer_cast<Text>(start_node);
        if (text_node) {
            // 对于折叠的 Range，返回光标位置
            auto rect = ComputeTextRect(text_node, start_offset_, start_offset_);
            if (rect.height > 0) {
                rects.push_back(rect);
            }
        }
    }
    
    return rects;
}

// ========== 私有辅助方法 ==========

int Range::GetNodeIndex(std::shared_ptr<Node> node) const {
    if (!node) {
        return -1;
    }

    auto parent = node->GetParentNode();
    if (!parent) {
        return -1;
    }

    const auto& children = parent->GetChildNodes();
    for (size_t i = 0; i < children.size(); ++i) {
        if (children[i] == node) {
            return static_cast<int>(i);
        }
    }

    return -1;
}

std::shared_ptr<Node> Range::FindCommonAncestor(
    std::shared_ptr<Node> node1,
    std::shared_ptr<Node> node2) const {

    if (!node1 || !node2) {
        return nullptr;
    }

    // 收集 node1 的所有祖先
    std::vector<std::shared_ptr<Node>> ancestors1;
    auto current = node1;
    while (current) {
        ancestors1.push_back(current);
        current = current->GetParentNode();
    }

    // 从 node2 向上查找，找到第一个在 ancestors1 中的节点
    current = node2;
    while (current) {
        for (const auto& ancestor : ancestors1) {
            if (current == ancestor) {
                return current;
            }
        }
        current = current->GetParentNode();
    }

    return nullptr;
}

int Range::GetNodeLength(std::shared_ptr<Node> node) const {
    if (!node) {
        return 0;
    }

    if (node->GetNodeType() == NodeType::TEXT_NODE) {
        auto text_node = std::dynamic_pointer_cast<Text>(node);
        if (text_node) {
            return static_cast<int>(text_node->GetData().length());
        }
        return 0;
    }

    // 对于元素节点，返回子节点数量
    return static_cast<int>(node->GetChildNodes().size());
}

void Range::CollectText(std::shared_ptr<Node> node, std::string& result, bool& in_range) const {
    if (!node) {
        return;
    }

    auto start_node = start_container_.lock();
    auto end_node = end_container_.lock();

    // 检查是否进入 Range
    if (node == start_node) {
        in_range = true;
        if (node->GetNodeType() == NodeType::TEXT_NODE) {
            auto text_node = std::dynamic_pointer_cast<Text>(node);
            if (text_node) {
                std::string content = text_node->GetData();
                if (start_node == end_node) {
                    // 起始和结束在同一节点
                    int start = std::max(0, start_offset_);
                    int end = std::min(static_cast<int>(content.length()), end_offset_);
                    if (start < end) {
                        result += content.substr(start, end - start);
                    }
                    in_range = false;
                } else {
                    // 只取起始偏移之后的部分
                    int start = std::max(0, start_offset_);
                    if (start < static_cast<int>(content.length())) {
                        result += content.substr(start);
                    }
                }
            }
        }
    } else if (node == end_node) {
        if (node->GetNodeType() == NodeType::TEXT_NODE) {
            auto text_node = std::dynamic_pointer_cast<Text>(node);
            if (text_node) {
                std::string content = text_node->GetData();
                int end = std::min(static_cast<int>(content.length()), end_offset_);
                if (end > 0) {
                    result += content.substr(0, end);
                }
            }
        }
        in_range = false;
    } else if (in_range && node->GetNodeType() == NodeType::TEXT_NODE) {
        auto text_node = std::dynamic_pointer_cast<Text>(node);
        if (text_node) {
            result += text_node->GetData();
        }
    }

    // 递归处理子节点
    bool first_child = true;
    for (const auto& child : node->GetChildNodes()) {
        if (!in_range && child == end_node) {
            break;
        }
        
        // 在块级元素之间添加换行符（除了第一个子元素）
        if (in_range && !first_child && child->GetNodeType() == NodeType::ELEMENT_NODE) {
            auto elem = std::dynamic_pointer_cast<Element>(child);
            if (elem) {
                std::string tag = elem->GetTagName();
                // 转换为小写
                for (auto& c : tag) {
                    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                }
                // 块级元素：div, p, br 等
                if (tag == "div" || tag == "p" || tag == "br" || tag == "li" || 
                    tag == "h1" || tag == "h2" || tag == "h3" || tag == "h4" || 
                    tag == "h5" || tag == "h6" || tag == "pre") {
                    result += "\n";
                }
            }
        }
        
        CollectText(child, result, in_range);
        first_child = false;
        
        if (!in_range) {
            break;
        }
    }
}

} // namespace lightui
