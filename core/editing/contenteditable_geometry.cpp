#include "contenteditable_geometry.h"

#include "core/dom/document.h"
#include "core/dom/element.h"
#include "core/dom/selection/range.h"
#include "core/dom/selection/selection.h"
#include "core/dom/text.h"
#include "core/utils/utf8_utils.h"

#include <algorithm>

namespace mbink {

bool IsNodeInsideEditingHost(const std::shared_ptr<Node>& node,
                             const Element* editing_host);

namespace {

int GetNodeTextLength(const std::shared_ptr<Node>& node) {
    auto text = std::dynamic_pointer_cast<Text>(node);
    return text ? static_cast<int>(utf8::CharCount(text->GetData())) : 0;
}

std::shared_ptr<Text> FindFirstEditableTextDescendant(const std::shared_ptr<Node>& node,
                                                      const Element* editing_host) {
    if (!node || !IsNodeInsideEditingHost(node, editing_host)) return nullptr;
    if (node->GetNodeType() == NodeType::TEXT_NODE) return std::dynamic_pointer_cast<Text>(node);
    for (const auto& child : node->GetChildNodes()) {
        if (auto text = FindFirstEditableTextDescendant(child, editing_host)) return text;
    }
    return nullptr;
}

std::shared_ptr<Text> FindLastEditableTextDescendant(const std::shared_ptr<Node>& node,
                                                     const Element* editing_host) {
    if (!node || !IsNodeInsideEditingHost(node, editing_host)) return nullptr;
    if (node->GetNodeType() == NodeType::TEXT_NODE) return std::dynamic_pointer_cast<Text>(node);
    const auto& children = node->GetChildNodes();
    for (auto it = children.rbegin(); it != children.rend(); ++it) {
        if (auto text = FindLastEditableTextDescendant(*it, editing_host)) return text;
    }
    return nullptr;
}

std::shared_ptr<Node> GetDeepestLastEditableNode(const std::shared_ptr<Node>& node,
                                                 const Element* editing_host) {
    auto current = node;
    while (current && current->GetNodeType() == NodeType::ELEMENT_NODE) {
        std::shared_ptr<Node> last_child = nullptr;
        const auto& children = current->GetChildNodes();
        for (auto it = children.rbegin(); it != children.rend(); ++it) {
            if (IsNodeInsideEditingHost(*it, editing_host)) {
                last_child = *it;
                break;
            }
        }
        if (!last_child) break;
        current = last_child;
    }
    return current;
}

std::shared_ptr<Node> GetNextNodeInHost(const std::shared_ptr<Node>& node,
                                        const Element* editing_host) {
    if (!node || !editing_host) return nullptr;
    if (node->GetNodeType() == NodeType::ELEMENT_NODE) {
        for (const auto& child : node->GetChildNodes()) {
            if (IsNodeInsideEditingHost(child, editing_host)) return child;
        }
    }
    auto current = node;
    while (current) {
        if (current.get() == editing_host) return nullptr;
        auto sibling = current->GetNextSibling();
        while (sibling && !IsNodeInsideEditingHost(sibling, editing_host)) {
            sibling = sibling->GetNextSibling();
        }
        if (sibling) return sibling;
        current = current->GetParentNode();
        if (current.get() == editing_host) return nullptr;
    }
    return nullptr;
}

std::shared_ptr<Node> GetPreviousNodeInHost(const std::shared_ptr<Node>& node,
                                            const Element* editing_host) {
    if (!node || !editing_host) return nullptr;
    auto sibling = node->GetPreviousSibling();
    while (sibling && !IsNodeInsideEditingHost(sibling, editing_host)) {
        sibling = sibling->GetPreviousSibling();
    }
    if (sibling) return GetDeepestLastEditableNode(sibling, editing_host);
    auto parent = node->GetParentNode();
    return (parent && parent.get() != editing_host && IsNodeInsideEditingHost(parent, editing_host))
        ? parent
        : nullptr;
}

ContentEditableResolvedPosition MakeResolved(const std::shared_ptr<Node>& node, int offset) {
    ContentEditableResolvedPosition result;
    result.valid = (node != nullptr);
    result.node = node;
    result.offset = std::max(0, offset);
    return result;
}

} // namespace

std::shared_ptr<Element> GetContentEditableEditingHost(const std::shared_ptr<Node>& node) {
    auto current = node;
    while (current) {
        if (current->GetNodeType() == NodeType::ELEMENT_NODE) {
            auto element = std::dynamic_pointer_cast<Element>(current);
            if (element && element->HasAttribute("contenteditable")) {
                const std::string attr = element->GetContentEditable();
                if (attr == "true") return element;
                if (attr == "false") return nullptr;
            }
        }
        current = current->GetParentNode();
    }
    return nullptr;
}

std::shared_ptr<Element> GetContainingContentEditableHost(const std::shared_ptr<Node>& node) {
    auto current = node;
    while (current) {
        if (current->GetNodeType() == NodeType::ELEMENT_NODE) {
            auto element = std::dynamic_pointer_cast<Element>(current);
            if (element && element->HasAttribute("contenteditable") &&
                element->GetContentEditable() == "true") {
                return element;
            }
        }
        current = current->GetParentNode();
    }
    return nullptr;
}

bool IsNodeInsideEditingHost(const std::shared_ptr<Node>& node,
                             const Element* editing_host) {
    if (!node || !editing_host) {
        return false;
    }
    return GetContentEditableEditingHost(node).get() == editing_host;
}

bool IsNodeInsideContentEditable(const std::shared_ptr<Node>& node,
                                 const Element* contenteditable_root) {
    return IsNodeInsideEditingHost(node, contenteditable_root);
}


ContentEditableResolvedPosition ResolveContentEditableCaretPosition(
    const std::shared_ptr<Element>& contenteditable_root,
    const std::shared_ptr<Node>& node,
    int offset,
    bool prefer_after) {
    if (!contenteditable_root || !node) {
        return {};
    }

    const Element* editing_host = contenteditable_root.get();
    std::shared_ptr<Node> normalized_node = node;
    if (!IsNodeInsideEditingHost(normalized_node, editing_host)) {
        normalized_node = GetContainingContentEditableHost(normalized_node);
        if (!normalized_node || normalized_node.get() != editing_host) {
            normalized_node = contenteditable_root;
        }
    }

    if (normalized_node->GetNodeType() == NodeType::TEXT_NODE) {
        return MakeResolved(normalized_node, std::min(offset, GetNodeTextLength(normalized_node)));
    }

    const auto& children = normalized_node->GetChildNodes();
    const int child_count = static_cast<int>(children.size());
    const int clamped_offset = std::max(0, std::min(offset, child_count));

    if (clamped_offset < child_count) {
        auto candidate = FindFirstEditableTextDescendant(children[clamped_offset], editing_host);
        if (candidate) {
            return MakeResolved(candidate, 0);
        }
    }

    if (clamped_offset > 0) {
        auto candidate = FindLastEditableTextDescendant(children[clamped_offset - 1], editing_host);
        if (candidate) {
            return MakeResolved(candidate, GetNodeTextLength(candidate));
        }
    }

    auto search = prefer_after
        ? GetNextNodeInHost(normalized_node, editing_host)
        : GetPreviousNodeInHost(normalized_node, editing_host);

    while (search) {
        if (auto text = prefer_after
                ? FindFirstEditableTextDescendant(search, editing_host)
                : FindLastEditableTextDescendant(search, editing_host)) {
            return MakeResolved(text, prefer_after ? 0 : GetNodeTextLength(text));
        }
        search = prefer_after
            ? GetNextNodeInHost(search, editing_host)
            : GetPreviousNodeInHost(search, editing_host);
    }

    if (prefer_after) {
        if (auto first = FindFirstEditableTextDescendant(contenteditable_root, editing_host)) {
            return MakeResolved(first, 0);
        }
    } else {
        if (auto last = FindLastEditableTextDescendant(contenteditable_root, editing_host)) {
            return MakeResolved(last, GetNodeTextLength(last));
        }
    }

    return {};
}

ContentEditableCaretRect ComputeContentEditableCaretRect(
    const std::shared_ptr<Element>& contenteditable_root,
    const std::shared_ptr<Node>& node,
    int offset) {
    ContentEditableCaretRect result;
    if (!contenteditable_root || !node || !IsNodeInsideContentEditable(node, contenteditable_root.get())) {
        return result;
    }

    auto document = std::dynamic_pointer_cast<Document>(contenteditable_root->GetOwnerDocument());
    if (!document) {
        return result;
    }

    auto range = std::make_shared<Range>(document);
    range->SetStart(node, offset);
    range->SetEnd(node, offset);
    auto rect = range->GetBoundingClientRect();
    if (rect.height <= 0.0f) {
        return result;
    }

    result.valid = true;
    result.x = rect.x;
    result.y = rect.y;
    result.width = std::max(1.0f, rect.width);
    result.height = rect.height;
    return result;
}

std::vector<ContentEditableSelectionRect> ConvertRangeClientRects(
    const std::shared_ptr<Element>& contenteditable_root,
    const std::shared_ptr<Range>& range) {
    std::vector<ContentEditableSelectionRect> rects;
    if (!contenteditable_root || !range) {
        return rects;
    }

    for (const auto& rect : range->GetClientRects()) {
        if (rect.width <= 0.0f || rect.height <= 0.0f) {
            continue;
        }
        rects.push_back({rect.x, rect.y, rect.width, rect.height});
    }
    return rects;
}

std::vector<ContentEditableSelectionRect> ComputeContentEditableSelectionRects(
    const std::shared_ptr<Element>& contenteditable_root,
    const std::shared_ptr<Selection>& selection) {
    if (!contenteditable_root || !selection || selection->IsCollapsed()) {
        return {};
    }

    auto anchor_node = selection->GetComputedAnchorNode();
    auto focus_node = selection->GetComputedFocusNode();
    if (!IsNodeInsideContentEditable(anchor_node, contenteditable_root.get()) ||
        !IsNodeInsideContentEditable(focus_node, contenteditable_root.get())) {
        return {};
    }

    auto range = selection->GetRangeAt(0);
    return ConvertRangeClientRects(contenteditable_root, range);
}

} // namespace mbink

