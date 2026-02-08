/**
 * @file contenteditable_controller.cpp
 * @brief ContentEditable 控制器实现
 */

#include "contenteditable_controller.h"
#include "core/editing/selection_manager.h"
#include "core/editing/contenteditable_handler.h"
#include "core/dom/document.h"
#include "core/dom/element.h"
#include "core/dom/text.h"
#include "core/dom/selection/selection.h"
#include "core/render/objects/render_object.h"
#include "core/render/text/text_renderer.h"
#include "core/window/window.h"
#include <iostream>
#include <functional>
#include <vector>
#include <limits>

namespace lightui {

// ========== 构造函数/析构函数 ==========

ContentEditableController::ContentEditableController(
    SelectionManager* selection_manager,
    ContentEditableHandler* editable_handler)
    : selection_manager_(selection_manager)
    , editable_handler_(editable_handler) {
}

ContentEditableController::~ContentEditableController() = default;

// ========== 静态辅助方法 ==========

std::shared_ptr<Element> ContentEditableController::GetContentEditableRoot(std::shared_ptr<Node> node) {
    if (!node) {
        return nullptr;
    }

    // 向上遍历查找最外层的可编辑元素（设置了 contenteditable="true" 属性的元素）
    auto current = node;
    std::shared_ptr<Element> editable_root = nullptr;
    
    while (current) {
        if (current->GetNodeType() == NodeType::ELEMENT_NODE) {
            auto element = std::dynamic_pointer_cast<Element>(current);
            if (element) {
                // 检查是否显式设置了 contenteditable="true" 属性
                if (element->HasAttribute("contenteditable")) {
                    std::string attr = element->GetAttribute("contenteditable");
                    if (attr == "true" || attr == "") {
                        editable_root = element;
                        // 继续向上查找，以找到最外层的 contentEditable 元素
                    }
                }
            }
        }
        current = current->GetParentNode();
    }

    return editable_root;
}

// ========== 鼠标事件处理 ==========

bool ContentEditableController::HandleMouseDown(
    std::shared_ptr<Element> target,
    float x, float y,
    bool shift_key,
    std::shared_ptr<RenderObject> render_object,
    std::shared_ptr<RenderObject> root_render) {

    if (!target || !target->IsContentEditable()) {
        return false;
    }


    auto doc = target->GetOwnerDocument();
    if (!doc || !selection_manager_ || !render_object) {
        return false;
    }

    auto selection = selection_manager_->GetSelection(doc);
    if (!selection) {
        return false;
    }

    // 记录最后一次 mousedown 的元素
    last_mousedown_element_ = target;

    // 找到 contentEditable 根元素
    auto contenteditable_root = GetContentEditableRoot(target);
    if (!contenteditable_root) {
        contenteditable_root = target;
    }


    // 找到根元素的渲染对象
    float abs_x = 0, abs_y = 0;
    auto contenteditable_render = FindContentEditableRenderObject(root_render, contenteditable_root, abs_x, abs_y);
    
    if (!contenteditable_render) {
        // 回退到 render_object
        contenteditable_render = render_object;
        // 简单估算绝对坐标
        abs_x = x;
        abs_y = y;
    }

    // 计算相对于 contentEditable 根元素的点击坐标
    const auto& style = contenteditable_render->GetComputedStyle();
    float padding_left = style.padding.left.ToPx();
    float padding_top = style.padding.top.ToPx();
    float border_left = style.border_left_width;
    float border_top = style.border_top_width;
    
    float click_x = x - abs_x - padding_left - border_left;
    float click_y = y - abs_y - padding_top - border_top;

    // 查找文本节点
    std::shared_ptr<Node> target_text_node = nullptr;
    int target_offset = 0;
    
    FindTextNodeAtPosition(contenteditable_render, click_x, click_y, target_text_node, target_offset);

    if (target_text_node) {
        if (shift_key) {
            // Shift+点击：扩展选择
            selection->Extend(target_text_node, target_offset);
        } else {
            // 普通点击：设置光标位置
            selection->Collapse(target_text_node, target_offset);
        }
        
        // 开始拖拽选择
        drag_state_.is_active = true;
        drag_state_.editable_root = contenteditable_root;
        drag_state_.start_node = selection->GetAnchorNode();
        drag_state_.start_offset = selection->GetAnchorOffset();
        
    } else {
        // 没找到文本节点，折叠到元素
        selection->Collapse(target, 0);
        drag_state_.is_active = true;
        drag_state_.editable_root = contenteditable_root;
        drag_state_.start_node = target;
        drag_state_.start_offset = 0;
    }

    return true;
}

bool ContentEditableController::HandleMouseMove(
    std::shared_ptr<Document> document,
    float x, float y,
    std::shared_ptr<RenderObject> root_render,
    Window* window) {

    if (!drag_state_.is_active || !drag_state_.editable_root) {
        return false;
    }

    if (!document || !selection_manager_ || !root_render) {
        return false;
    }

    auto selection = selection_manager_->GetSelection(document);
    if (!selection || !drag_state_.start_node) {
        return false;
    }


    // 找到 contentEditable 根元素的渲染对象
    float abs_x = 0, abs_y = 0;
    auto contenteditable_render = FindContentEditableRenderObject(
        root_render, drag_state_.editable_root, abs_x, abs_y);
    
    if (!contenteditable_render) {
        return false;
    }

    // 计算相对坐标
    const auto& style = contenteditable_render->GetComputedStyle();
    float padding_left = style.padding.left.ToPx();
    float padding_top = style.padding.top.ToPx();
    float border_left = style.border_left_width;
    float border_top = style.border_top_width;
    
    float click_x = x - abs_x - padding_left - border_left;
    float click_y = y - abs_y - padding_top - border_top;

    // 查找文本节点
    std::shared_ptr<Node> target_text_node = nullptr;
    int target_offset = 0;
    
    FindTextNodeAtPosition(contenteditable_render, click_x, click_y, target_text_node, target_offset);

    if (target_text_node) {
        // 更新选择
        selection->UpdateFromUserAction(
            drag_state_.start_node, drag_state_.start_offset,
            target_text_node, target_offset
        );
        
        if (window) {
            window->SetNeedsRepaint();
        }
        
    }

    return true;
}

bool ContentEditableController::HandleMouseUp(
    std::shared_ptr<Element> /*target*/,
    float /*x*/, float /*y*/) {

    bool was_dragging = drag_state_.is_active;
    
    // 结束拖拽选择
    drag_state_.Reset();
    
    
    return was_dragging;
}

// ========== 键盘事件处理 ==========

bool ContentEditableController::HandleKeyDown(
    std::shared_ptr<Element> target,
    int key_code,
    bool ctrl_key,
    bool shift_key,
    bool alt_key) {

    if (!editable_handler_) {
        return false;
    }

    return editable_handler_->HandleKeyDown(target, key_code, ctrl_key, shift_key, alt_key);
}

bool ContentEditableController::HandleTextInput(
    std::shared_ptr<Element> target,
    const std::string& text) {

    if (!editable_handler_) {
        return false;
    }

    return editable_handler_->HandleTextInput(target, text);
}

// ========== 私有辅助方法 ==========

std::shared_ptr<RenderObject> ContentEditableController::FindContentEditableRenderObject(
    std::shared_ptr<RenderObject> root_render,
    std::shared_ptr<Element> contenteditable_root,
    float& out_abs_x,
    float& out_abs_y) {

    if (!root_render || !contenteditable_root) {
        return nullptr;
    }

    struct FindResult {
        std::shared_ptr<RenderObject> render_obj;
        float abs_x = 0;
        float abs_y = 0;
    };

    std::function<FindResult(std::shared_ptr<RenderObject>, float, float)> findRenderObj;
    findRenderObj = [&](std::shared_ptr<RenderObject> obj, float offset_x, float offset_y) -> FindResult {
        if (!obj) return {};
        
        const auto& layout = obj->GetLayoutInfo();
        float current_x = offset_x + layout.x;
        float current_y = offset_y + layout.y;
        
        auto node = obj->GetNode();
        if (node && node == contenteditable_root) {
            return {obj, current_x, current_y};
        }
        
        float child_offset_x = current_x - obj->GetScrollX();
        float child_offset_y = current_y - obj->GetScrollY();
        
        for (auto& child : obj->GetChildren()) {
            auto result = findRenderObj(child, child_offset_x, child_offset_y);
            if (result.render_obj) {
                return result;
            }
        }
        
        return {};
    };

    auto result = findRenderObj(root_render, 0.0f, 0.0f);
    if (result.render_obj) {
        out_abs_x = result.abs_x;
        out_abs_y = result.abs_y;
        return result.render_obj;
    }

    return nullptr;
}

bool ContentEditableController::FindTextNodeAtPosition(
    std::shared_ptr<RenderObject> render_obj,
    float click_x, float click_y,
    std::shared_ptr<Node>& out_node,
    int& out_offset) {

    if (!render_obj) {
        return false;
    }

    const auto& root_layout = render_obj->GetLayoutInfo();
    
    // 精确查找
    std::function<bool(RenderObject*, float, float, float, float)> findTextAtPosition;
    findTextAtPosition = [&](RenderObject* obj, float acc_x, float acc_y, float target_x, float target_y) -> bool {
        const auto& layout = obj->GetLayoutInfo();
        float new_x = acc_x + layout.x;
        float new_y = acc_y + layout.y;
        
        auto node = obj->GetNode();
        if (node && node->GetNodeType() == NodeType::TEXT_NODE) {
            auto text_node = std::dynamic_pointer_cast<Text>(node);
            
            bool in_x = target_x >= new_x && target_x < new_x + layout.width;
            bool in_y = target_y >= new_y && target_y < new_y + layout.height;
            
            if (in_x && in_y && text_node) {
                std::string text = text_node->GetTextContent();
                const auto& text_style = obj->GetComputedStyle();
                
                FontDescriptor desc;
                desc.family = !text_style.font_family.empty() ? text_style.font_family : "Arial";
                desc.size = text_style.font_size > 0 ? text_style.font_size : 16.0f;
                desc.weight = (text_style.font_weight == "bold" || text_style.font_weight == "700") 
                              ? FontWeight::BOLD : FontWeight::NORMAL;
                desc.style = (text_style.font_style == "italic") 
                             ? FontStyle::ITALIC : FontStyle::NORMAL;
                SkFont font = FontManager::GetInstance().LoadFont(desc);
                
                float letter_spacing = text_style.letter_spacing.ToPx(0, text_style.font_size);
                float word_spacing = text_style.word_spacing.ToPx(0, text_style.font_size);
                
                float text_x = target_x - new_x;
                int offset = 0;
                float accumulated_width = 0;
                
                for (size_t i = 0; i < text.length(); ) {
                    size_t char_len = 1;
                    unsigned char c = text[i];
                    if ((c & 0x80) == 0) char_len = 1;
                    else if ((c & 0xE0) == 0xC0) char_len = 2;
                    else if ((c & 0xF0) == 0xE0) char_len = 3;
                    else if ((c & 0xF8) == 0xF0) char_len = 4;
                    
                    std::string char_str = text.substr(i, char_len);
                    float char_width = TextRenderer::MeasureMixedTextWidth(char_str, font);
                    if (char_str == " ") char_width += word_spacing;
                    float total_char_width = char_width;
                    if (i + char_len < text.length()) total_char_width += letter_spacing;
                    
                    if (accumulated_width + char_width / 2 > text_x) break;
                    accumulated_width += total_char_width;
                    offset += static_cast<int>(char_len);
                    i += char_len;
                }
                
                out_node = text_node;
                out_offset = offset;
                return true;
            }
        }
        
        for (auto& child : obj->GetChildren()) {
            if (findTextAtPosition(child.get(), new_x, new_y, target_x, target_y)) {
                return true;
            }
        }
        return false;
    };

    if (findTextAtPosition(render_obj.get(), -root_layout.x, -root_layout.y, click_x, click_y)) {
        return true;
    }

    // 如果没找到精确的文本节点，找最近的
    struct TextNodeInfo {
        std::shared_ptr<Text> node;
        float x, y, width, height;
    };
    std::vector<TextNodeInfo> text_nodes;
    
    std::function<void(RenderObject*, float, float)> collectTextNodes;
    collectTextNodes = [&](RenderObject* obj, float acc_x, float acc_y) {
        const auto& layout = obj->GetLayoutInfo();
        float new_x = acc_x + layout.x;
        float new_y = acc_y + layout.y;
        
        auto node = obj->GetNode();
        if (node && node->GetNodeType() == NodeType::TEXT_NODE) {
            auto text_node = std::dynamic_pointer_cast<Text>(node);
            if (text_node && !text_node->GetTextContent().empty()) {
                text_nodes.push_back({text_node, new_x, new_y, layout.width, layout.height});
            }
        }
        
        for (auto& child : obj->GetChildren()) {
            collectTextNodes(child.get(), new_x, new_y);
        }
    };
    collectTextNodes(render_obj.get(), -root_layout.x, -root_layout.y);
    
    if (text_nodes.empty()) {
        return false;
    }

    // 找同一行的节点
    std::vector<TextNodeInfo*> same_line_nodes;
    for (auto& info : text_nodes) {
        if (click_y >= info.y && click_y < info.y + info.height) {
            same_line_nodes.push_back(&info);
        }
    }
    
    if (!same_line_nodes.empty()) {
        TextNodeInfo* closest = nullptr;
        float min_distance = std::numeric_limits<float>::max();
        
        for (auto* info : same_line_nodes) {
            float distance;
            if (click_x < info->x) {
                distance = info->x - click_x;
            } else if (click_x > info->x + info->width) {
                distance = click_x - (info->x + info->width);
            } else {
                distance = 0;
            }
            
            if (distance < min_distance) {
                min_distance = distance;
                closest = info;
            }
        }
        
        if (closest) {
            out_node = closest->node;
            if (click_x >= closest->x + closest->width) {
                out_offset = static_cast<int>(closest->node->GetTextContent().length());
            } else {
                out_offset = 0;
            }
            return true;
        }
    } else if (!text_nodes.empty()) {
        // 没有同一行的，使用最后一个
        auto& last = text_nodes.back();
        out_node = last.node;
        out_offset = static_cast<int>(last.node->GetTextContent().length());
        return true;
    }

    return false;
}

} // namespace lightui
