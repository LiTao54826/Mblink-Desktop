#pragma once

#include <memory>
#include <vector>

namespace lightui {

class Element;
class Node;
class Range;
class Selection;

struct ContentEditableCaretRect {
    bool valid = false;
    float x = 0.0f;
    float y = 0.0f;
    float width = 1.0f;
    float height = 0.0f;
};

struct ContentEditableSelectionRect {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
};

struct ContentEditableResolvedPosition {
    bool valid = false;
    std::shared_ptr<Node> node;
    int offset = 0;
};

std::shared_ptr<Element> GetContentEditableEditingHost(const std::shared_ptr<Node>& node);
std::shared_ptr<Element> GetContainingContentEditableHost(const std::shared_ptr<Node>& node);

bool IsNodeInsideEditingHost(const std::shared_ptr<Node>& node,
                             const Element* editing_host);

bool IsNodeInsideContentEditable(const std::shared_ptr<Node>& node,
                                 const Element* contenteditable_root);

ContentEditableResolvedPosition ResolveContentEditableCaretPosition(
    const std::shared_ptr<Element>& contenteditable_root,
    const std::shared_ptr<Node>& node,
    int offset,
    bool prefer_after);

ContentEditableCaretRect ComputeContentEditableCaretRect(
    const std::shared_ptr<Element>& contenteditable_root,
    const std::shared_ptr<Node>& node,
    int offset);

std::vector<ContentEditableSelectionRect> ComputeContentEditableSelectionRects(
    const std::shared_ptr<Element>& contenteditable_root,
    const std::shared_ptr<Selection>& selection);

std::vector<ContentEditableSelectionRect> ConvertRangeClientRects(
    const std::shared_ptr<Element>& contenteditable_root,
    const std::shared_ptr<Range>& range);

} // namespace lightui

