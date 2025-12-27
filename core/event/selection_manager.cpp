/**
 * @file selection_manager.cpp
 * @brief 选择管理器实现
 */

#include "selection_manager.h"
#include "core/dom/selection.h"
#include "core/dom/document.h"
#include "core/dom/element.h"
#include "core/dom/text.h"
#include "core/dom/range.h"
#include <algorithm>

// Skia 头文件
#include "include/core/SkCanvas.h"
#include "include/core/SkPaint.h"
#include "include/core/SkRect.h"

namespace lightui {

SelectionManager::SelectionManager() = default;

SelectionManager::~SelectionManager() = default;

// ========== Selection 管理 ==========

std::shared_ptr<Selection> SelectionManager::GetSelection(std::shared_ptr<Document> document) {
    if (!document) {
        return nullptr;
    }

    auto* doc_ptr = document.get();
    auto it = selections_.find(doc_ptr);
    if (it != selections_.end()) {
        return it->second;
    }

    // 创建新的 Selection 对象
    auto selection = std::make_shared<Selection>(document);
    selections_[doc_ptr] = selection;
    
    // 设置到 Document 中，以便渲染时可以访问
    document->SetSelection(selection);
    
    return selection;
}

void SelectionManager::ClearSelection(std::shared_ptr<Document> document) {
    if (!document) {
        return;
    }

    auto selection = GetSelection(document);
    if (selection) {
        selection->RemoveAllRanges();
    }
}

// ========== 鼠标选择处理 ==========

void SelectionManager::HandleMouseDown(std::shared_ptr<Element> target, int x, int y) {
    if (!target) {
        return;
    }

    auto doc = target->GetOwnerDocument();
    if (!doc) {
        return;
    }

    // 计算光标位置
    CaretPosition pos = HitTestToCaretPosition(target, x, y);
    if (!pos.IsValid()) {
        return;
    }

    // 开始选择
    is_selecting_ = true;
    selection_start_ = pos;

    // 设置光标位置（折叠选择）
    auto selection = GetSelection(doc);
    if (selection) {
        selection->Collapse(pos.node, pos.offset);
    }

    // 重置光标闪烁
    ResetCaretBlink();
}

void SelectionManager::HandleMouseMove(std::shared_ptr<Element> target, int x, int y, bool is_dragging) {
    if (!is_selecting_ || !is_dragging || !target) {
        return;
    }

    auto doc = target->GetOwnerDocument();
    if (!doc) {
        return;
    }

    // 计算当前光标位置
    CaretPosition pos = HitTestToCaretPosition(target, x, y);
    if (!pos.IsValid()) {
        return;
    }

    // 扩展选择
    auto selection = GetSelection(doc);
    if (selection && selection_start_.IsValid()) {
        selection->UpdateFromUserAction(
            selection_start_.node, selection_start_.offset,
            pos.node, pos.offset);
    }
}

void SelectionManager::HandleMouseUp(std::shared_ptr<Element> target, int x, int y) {
    is_selecting_ = false;
}

// ========== 键盘选择处理 ==========

void SelectionManager::HandleShiftArrow(std::shared_ptr<Document> document, const std::string& direction) {
    if (!document) {
        return;
    }

    auto selection = GetSelection(document);
    if (!selection) {
        return;
    }

    // 获取当前焦点位置
    auto focus_node = selection->GetFocusNode();
    int focus_offset = selection->GetFocusOffset();

    if (!focus_node) {
        return;
    }

    // 计算新的焦点位置
    CaretPosition new_pos;
    new_pos.node = focus_node;
    new_pos.offset = focus_offset;

    if (direction == "left") {
        if (focus_offset > 0) {
            new_pos.offset = focus_offset - 1;
        }
        // TODO: 跨节点移动
    } else if (direction == "right") {
        // 获取节点长度
        int max_offset = 0;
        if (focus_node->GetNodeType() == NodeType::TEXT_NODE) {
            auto text_node = std::dynamic_pointer_cast<Text>(focus_node);
            if (text_node) {
                max_offset = static_cast<int>(text_node->GetData().length());
            }
        }
        if (focus_offset < max_offset) {
            new_pos.offset = focus_offset + 1;
        }
        // TODO: 跨节点移动
    }
    // TODO: 处理 up/down

    // 扩展选择
    selection->Extend(new_pos.node, new_pos.offset);
}

void SelectionManager::HandleArrowKey(std::shared_ptr<Document> document, const std::string& direction) {
    if (!document) {
        return;
    }

    auto selection = GetSelection(document);
    if (!selection) {
        return;
    }

    // 如果有选择，先折叠
    if (!selection->IsCollapsed()) {
        if (direction == "left" || direction == "up") {
            selection->CollapseToStart();
        } else {
            selection->CollapseToEnd();
        }
        ResetCaretBlink();
        return;
    }

    // 移动光标
    bool forward = (direction == "right" || direction == "down");
    CaretPosition new_pos = MoveCaretByCharacter(document, forward);

    if (new_pos.IsValid()) {
        selection->Collapse(new_pos.node, new_pos.offset);
        ResetCaretBlink();
    }
}

// ========== 光标位置计算 ==========

CaretPosition SelectionManager::HitTestToCaretPosition(std::shared_ptr<Element> element, int x, int y) {
    CaretPosition result;

    if (!element) {
        return result;
    }

    // 简化实现：查找包含坐标的文本节点
    result = FindTextNodeAtPosition(element, x, y);

    return result;
}

CaretPosition SelectionManager::GetCaretPosition(std::shared_ptr<Document> document) {
    CaretPosition result;

    if (!document) {
        return result;
    }

    auto selection = GetSelection(document);
    if (!selection || !selection->IsCollapsed()) {
        return result;
    }

    result.node = selection->GetFocusNode();
    result.offset = selection->GetFocusOffset();

    // TODO: 计算屏幕坐标
    // 这需要访问布局信息

    return result;
}

// ========== 光标渲染 ==========

void SelectionManager::RenderCaret(SkCanvas* canvas, std::shared_ptr<Document> document) {
    if (!canvas || !document || !caret_visible_) {
        return;
    }

    auto selection = GetSelection(document);
    if (!selection || !selection->IsCollapsed()) {
        return;
    }

    CaretPosition pos = GetCaretPosition(document);
    if (!pos.IsValid()) {
        return;
    }

    // 绘制光标
    SkPaint paint;
    paint.setColor(SK_ColorBLACK);
    paint.setStrokeWidth(1.0f);
    paint.setStyle(SkPaint::kStroke_Style);

    float caret_height = pos.height > 0 ? pos.height : 16.0f;
    canvas->drawLine(pos.x, pos.y, pos.x, pos.y + caret_height, paint);
}

void SelectionManager::RenderSelectionHighlight(SkCanvas* canvas, std::shared_ptr<Document> document) {
    if (!canvas || !document) {
        return;
    }

    auto selection = GetSelection(document);
    if (!selection || selection->IsCollapsed()) {
        return;
    }

    // TODO: 实现选择高亮渲染
    // 这需要计算选择区域的矩形并绘制半透明背景
}

// ========== 光标闪烁控制 ==========

void SelectionManager::UpdateCaretBlink(float delta_time) {
    caret_blink_timer_ += delta_time;
    if (caret_blink_timer_ >= CARET_BLINK_INTERVAL) {
        caret_blink_timer_ = 0.0f;
        caret_visible_ = !caret_visible_;
    }
}

void SelectionManager::ResetCaretBlink() {
    caret_visible_ = true;
    caret_blink_timer_ = 0.0f;
}

// ========== 选择状态查询 ==========

std::string SelectionManager::GetSelectedText(std::shared_ptr<Document> document) {
    if (!document) {
        return "";
    }

    auto selection = GetSelection(document);
    if (!selection) {
        return "";
    }

    return selection->ToString();
}

// ========== 私有辅助方法 ==========

CaretPosition SelectionManager::FindTextNodeAtPosition(std::shared_ptr<Element> element, int x, int y) {
    CaretPosition result;

    if (!element) {
        return result;
    }

    // 遍历子节点查找文本节点
    for (const auto& child : element->GetChildNodes()) {
        if (child->GetNodeType() == NodeType::TEXT_NODE) {
            // 简化实现：返回第一个文本节点
            // TODO: 实现真正的 hit testing
            result.node = child;
            result.offset = CalculateTextOffset(child, x);
            result.x = static_cast<float>(x);
            result.y = static_cast<float>(y);
            result.height = 16.0f; // 默认高度
            return result;
        } else if (child->GetNodeType() == NodeType::ELEMENT_NODE) {
            auto child_element = std::dynamic_pointer_cast<Element>(child);
            if (child_element) {
                result = FindTextNodeAtPosition(child_element, x, y);
                if (result.IsValid()) {
                    return result;
                }
            }
        }
    }

    return result;
}

int SelectionManager::CalculateTextOffset(std::shared_ptr<Node> text_node, int x) {
    if (!text_node || text_node->GetNodeType() != NodeType::TEXT_NODE) {
        return 0;
    }

    auto text = std::dynamic_pointer_cast<Text>(text_node);
    if (!text) {
        return 0;
    }

    // 简化实现：假设每个字符宽度为 8 像素
    // TODO: 使用实际的字体度量
    const int char_width = 8;
    std::string content = text->GetData();
    int offset = x / char_width;
    return std::min(offset, static_cast<int>(content.length()));
}

CaretPosition SelectionManager::MoveCaretByCharacter(std::shared_ptr<Document> document, bool forward) {
    CaretPosition result;

    auto selection = GetSelection(document);
    if (!selection) {
        return result;
    }

    auto node = selection->GetFocusNode();
    int offset = selection->GetFocusOffset();

    if (!node) {
        return result;
    }

    if (node->GetNodeType() == NodeType::TEXT_NODE) {
        auto text_node = std::dynamic_pointer_cast<Text>(node);
        if (text_node) {
            std::string content = text_node->GetData();
            int max_offset = static_cast<int>(content.length());

            if (forward) {
                if (offset < max_offset) {
                    result.node = node;
                    result.offset = offset + 1;
                }
                // TODO: 跨节点移动
            } else {
                if (offset > 0) {
                    result.node = node;
                    result.offset = offset - 1;
                }
                // TODO: 跨节点移动
            }
        }
    }

    return result;
}

} // namespace lightui
