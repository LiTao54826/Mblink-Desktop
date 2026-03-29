/**
 * @file contenteditable_controller.cpp
 * @brief ContentEditable 控制器实现
 */

#include "contenteditable_controller.h"
#include "core/render/text/font_manager.h"
#include "core/editing/selection_manager.h"
#include "core/editing/contenteditable_handler.h"
#include "core/editing/contenteditable_geometry.h"
#include "core/dom/document.h"
#include "core/dom/element.h"
#include "core/dom/text.h"
#include "core/dom/selection/selection.h"
#include "core/render/objects/render_object.h"
#include "core/render/text/text_renderer.h"
#include "core/window/window.h"
#include "core/utils/utf8_utils.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <functional>
#include <vector>
#include <limits>

namespace mbink {

namespace {

struct TextFragmentInfo {
    std::shared_ptr<Text> node;
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    float center_y = 0.0f;
    float letter_spacing = 0.0f;
    float word_spacing = 0.0f;
    std::string text;
    SkFont font;
    int char_count = 0;
};

SkFont BuildFontFromStyle(const ComputedStyle& text_style) {
    FontDescriptor desc;
    desc.family = !text_style.font_family.empty() ? text_style.font_family : "Arial";
    desc.size = text_style.font_size > 0 ? text_style.font_size : 16.0f;
    desc.weight = ParseCSSFontWeight(text_style.font_weight);
    desc.style = (text_style.font_style == "italic")
                     ? FontStyle::ITALIC
                     : FontStyle::NORMAL;
    return FontManager::GetInstance().LoadFont(desc);
}

float MeasureTextWidthWithEmoji(const std::string& text, const SkFont& font) {
    TextRenderer renderer(nullptr);
    return renderer.MeasureTextWidthWithEmoji(text, font);
}

float MeasureFragmentCharAdvance(const TextFragmentInfo& fragment, int char_index) {
    if (char_index < 0 || char_index >= fragment.char_count) {
        return 0.0f;
    }

    std::string char_str = utf8::SubstrByChar(fragment.text, char_index, char_index + 1);
    float char_width = MeasureTextWidthWithEmoji(char_str, fragment.font);
    if (char_str == " ") {
        char_width += fragment.word_spacing;
    }
    if (char_index + 1 < fragment.char_count) {
        char_width += fragment.letter_spacing;
    }
    return char_width;
}

float MeasureFragmentPrefixWidth(const TextFragmentInfo& fragment, int char_offset) {
    const int clamped_offset = std::max(0, std::min(char_offset, fragment.char_count));
    if (clamped_offset <= 0) {
        return 0.0f;
    }

    std::string prefix = utf8::SubstrByChar(fragment.text, 0, clamped_offset);
    float prefix_width = MeasureTextWidthWithEmoji(prefix, fragment.font);

    for (int i = 0; i + 1 < clamped_offset; ++i) {
        prefix_width += fragment.letter_spacing;
        std::string char_str = utf8::SubstrByChar(fragment.text, i, i + 1);
        if (char_str == " ") {
            prefix_width += fragment.word_spacing;
        }
    }

    return prefix_width;
}

int HitTestFragmentOffset(const TextFragmentInfo& fragment, float local_x) {
    if (fragment.char_count <= 0 || local_x <= 0.0f) {
        return 0;
    }

    float best_distance = std::numeric_limits<float>::max();
    int best_offset = 0;

    for (int offset = 0; offset <= fragment.char_count; ++offset) {
        const float boundary_x = MeasureFragmentPrefixWidth(fragment, offset);
        const float distance = std::abs(local_x - boundary_x);
        if (distance < best_distance) {
            best_distance = distance;
            best_offset = offset;
        }
    }

    return best_offset;
}

float DistanceToFragmentBoxSquared(const TextFragmentInfo& fragment, float x, float y) {
    const float dx = (x < fragment.x) ? (fragment.x - x)
        : (x > fragment.x + fragment.width ? x - (fragment.x + fragment.width) : 0.0f);
    const float dy = (y < fragment.y) ? (fragment.y - y)
        : (y > fragment.y + fragment.height ? y - (fragment.y + fragment.height) : 0.0f);
    return dx * dx + dy * dy;
}

} // namespace

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
    return GetContentEditableEditingHost(node);
}

ContentEditableResolvedPosition ContentEditableController::ResolveFallbackCaretPosition(
    const std::shared_ptr<Element>& contenteditable_root,
    const std::shared_ptr<Node>& fallback_node,
    float click_x,
    float host_width) {
    if (!contenteditable_root) {
        return {};
    }

    std::shared_ptr<Node> base_node = fallback_node ? fallback_node : contenteditable_root;
    const bool prefer_after = click_x >= std::max(0.0f, host_width * 0.5f);
    return ResolveContentEditableCaretPosition(contenteditable_root, base_node, 0, prefer_after);
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

    const auto& style = contenteditable_render->GetComputedStyle();
    const auto& layout = contenteditable_render->GetLayoutInfo();
    const float padding_left = style.padding.left.ToPx();
    const float padding_top = style.padding.top.ToPx();
    const float padding_right = style.padding.right.ToPx();
    const float padding_bottom = style.padding.bottom.ToPx();
    const float border_left = style.border_left_width;
    const float border_top = style.border_top_width;
    const float border_right = style.border_right_width;
    const float border_bottom = style.border_bottom_width;
    const float content_width = std::max(0.0f,
        layout.width - padding_left - padding_right - border_left - border_right);
    const float content_height = std::max(0.0f,
        layout.height - padding_top - padding_bottom - border_top - border_bottom);

    float click_x = x - abs_x - padding_left - border_left;
    float click_y = y - abs_y - padding_top - border_top;
    click_x = std::clamp(click_x, 0.0f, content_width);
    click_y = std::clamp(click_y, 0.0f, content_height);

    const float hit_test_x = click_x + padding_left + border_left;
    const float hit_test_y = click_y + padding_top + border_top;

    // 查找文本节点
    std::shared_ptr<Node> target_text_node = nullptr;
    int target_offset = 0;

    FindTextNodeAtPosition(contenteditable_render, hit_test_x, hit_test_y, target_text_node, target_offset);

    ContentEditableResolvedPosition resolved_position;
    if (target_text_node) {
        resolved_position.valid = true;
        resolved_position.node = target_text_node;
        resolved_position.offset = target_offset;
    } else {
        resolved_position = ResolveFallbackCaretPosition(
            contenteditable_root, target, click_x, content_width);
    }

    if (resolved_position.valid && resolved_position.node) {
        if (shift_key) {
            selection->Extend(resolved_position.node, resolved_position.offset);
        } else {
            selection->Collapse(resolved_position.node, resolved_position.offset);
        }

        drag_state_.is_active = true;
        drag_state_.editable_root = contenteditable_root;
        drag_state_.start_node = selection->GetAnchorNode();
        drag_state_.start_offset = selection->GetAnchorOffset();
    } else {
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

    if (!IsNodeInsideEditingHost(drag_state_.start_node, drag_state_.editable_root.get())) {
        drag_state_.Reset();
        return false;
    }

    float abs_x = 0.0f;
    float abs_y = 0.0f;
    auto contenteditable_render = FindContentEditableRenderObject(
        root_render, drag_state_.editable_root, abs_x, abs_y);
    if (!contenteditable_render) {
        return false;
    }

    const auto& style = contenteditable_render->GetComputedStyle();
    const auto& layout = contenteditable_render->GetLayoutInfo();
    const float padding_left = style.padding.left.ToPx();
    const float padding_top = style.padding.top.ToPx();
    const float padding_right = style.padding.right.ToPx();
    const float padding_bottom = style.padding.bottom.ToPx();
    const float border_left = style.border_left_width;
    const float border_top = style.border_top_width;
    const float border_right = style.border_right_width;
    const float border_bottom = style.border_bottom_width;
    const float content_width = std::max(0.0f,
        layout.width - padding_left - padding_right - border_left - border_right);
    const float content_height = std::max(0.0f,
        layout.height - padding_top - padding_bottom - border_top - border_bottom);

    float click_x = x - abs_x - padding_left - border_left;
    float click_y = y - abs_y - padding_top - border_top;
    click_x = std::clamp(click_x, 0.0f, content_width);
    click_y = std::clamp(click_y, 0.0f, content_height);

    const float hit_test_x = click_x + padding_left + border_left;
    const float hit_test_y = click_y + padding_top + border_top;

    std::shared_ptr<Node> target_text_node = nullptr;
    int target_offset = 0;
    FindTextNodeAtPosition(contenteditable_render, hit_test_x, hit_test_y, target_text_node, target_offset);

    ContentEditableResolvedPosition resolved_position;
    if (target_text_node) {
        resolved_position.valid = true;
        resolved_position.node = target_text_node;
        resolved_position.offset = target_offset;
    } else {
        resolved_position = ResolveFallbackCaretPosition(
            drag_state_.editable_root, drag_state_.start_node, click_x, content_width);
    }

    if (!resolved_position.valid || !resolved_position.node) {
        return false;
    }

    if (!IsNodeInsideEditingHost(resolved_position.node, drag_state_.editable_root.get())) {
        resolved_position = ResolveFallbackCaretPosition(
            drag_state_.editable_root, drag_state_.editable_root, click_x, content_width);
        if (!resolved_position.valid || !resolved_position.node) {
            return false;
        }
    }

    selection->UpdateFromUserAction(
        drag_state_.start_node, drag_state_.start_offset,
        resolved_position.node, resolved_position.offset
    );

    if (window) {
        window->SetNeedsRepaint();
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
    std::shared_ptr<Document> document,
    const std::string& text) {

    if (!target || !target->IsContentEditable() || !editable_handler_ || !document) {
        return false;
    }

    if (editable_handler_->HasActiveComposition(document)) {
        return editable_handler_->CommitComposition(document, text);
    }

    return editable_handler_->HandleTextInput(target, text);
}

bool ContentEditableController::HandleTextEditing(
    std::shared_ptr<Element> target,
    std::shared_ptr<Document> document,
    const std::string& text) {

    if (!target || !target->IsContentEditable() || !editable_handler_ || !document) {
        return false;
    }

    auto selection = document->GetSelection();
    int start = selection ? selection->GetFocusOffset() : 0;
    int end = start;
    if (editable_handler_->HasActiveComposition(document)) {
        const auto composition = editable_handler_->GetCompositionState(document);
        start = composition.start;
        end = composition.end;
    }

    if (text.empty()) {
        return editable_handler_->CancelComposition(document);
    }

    if (editable_handler_->HasActiveComposition(document)) {
        return editable_handler_->UpdateComposition(document, text, start, end);
    }

    return editable_handler_->StartComposition(document, text, start, end);
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

    std::vector<TextFragmentInfo> fragments;
    std::function<void(RenderObject*, float, float)> collectFragments;
    collectFragments = [&](RenderObject* obj, float acc_x, float acc_y) {
        if (!obj) {
            return;
        }

        const auto& layout = obj->GetLayoutInfo();
        const float new_x = acc_x + layout.x;
        const float new_y = acc_y + layout.y;
        const float child_acc_x = new_x - obj->GetScrollX();
        const float child_acc_y = new_y - obj->GetScrollY();

        auto node = obj->GetNode();
        if (node && node->GetNodeType() == NodeType::TEXT_NODE) {
            auto text_node = std::dynamic_pointer_cast<Text>(node);
            if (text_node) {
                std::string text = text_node->GetTextContent();
                const int char_count = static_cast<int>(utf8::CharCount(text));
                if (char_count > 0 && layout.width >= 0.0f && layout.height > 0.0f) {
                    const auto& text_style = obj->GetComputedStyle();
                    TextFragmentInfo fragment;
                    fragment.node = text_node;
                    fragment.x = new_x;
                    fragment.y = new_y;
                    fragment.width = layout.width;
                    fragment.height = layout.height;
                    fragment.center_y = new_y + layout.height * 0.5f;
                    fragment.letter_spacing = text_style.letter_spacing.ToPx(0, text_style.font_size);
                    fragment.word_spacing = text_style.word_spacing.ToPx(0, text_style.font_size);
                    fragment.text = std::move(text);
                    fragment.font = BuildFontFromStyle(text_style);
                    fragment.char_count = char_count;
                    fragments.push_back(std::move(fragment));
                }
            }
        }

        for (auto& child : obj->GetChildren()) {
            collectFragments(child.get(), child_acc_x, child_acc_y);
        }
    };

    collectFragments(render_obj.get(), -root_layout.x, -root_layout.y);
    if (fragments.empty()) {
        return false;
    }

    std::vector<TextFragmentInfo*> same_line_fragments;
    TextFragmentInfo* nearest_line_fragment = nullptr;
    float nearest_line_distance = std::numeric_limits<float>::max();

    for (auto& fragment : fragments) {
        const bool in_line_band = click_y >= fragment.y && click_y < fragment.y + fragment.height;
        const float line_distance = in_line_band
            ? 0.0f
            : std::abs(click_y - fragment.center_y);

        if (line_distance < nearest_line_distance) {
            nearest_line_distance = line_distance;
            nearest_line_fragment = &fragment;
        }

        if (in_line_band) {
            same_line_fragments.push_back(&fragment);
        }
    }

    std::vector<TextFragmentInfo*> candidate_fragments;
    if (!same_line_fragments.empty()) {
        candidate_fragments = same_line_fragments;
    } else if (nearest_line_fragment) {
        for (auto& fragment : fragments) {
            const float tolerance = std::max(fragment.height * 0.5f, 2.0f);
            if (std::abs(fragment.center_y - nearest_line_fragment->center_y) <= tolerance) {
                candidate_fragments.push_back(&fragment);
            }
        }
    }

    if (candidate_fragments.empty()) {
        candidate_fragments.push_back(&fragments.front());
    }

    std::sort(candidate_fragments.begin(), candidate_fragments.end(), [](const TextFragmentInfo* a, const TextFragmentInfo* b) {
        if (std::abs(a->center_y - b->center_y) > 1.0f) {
            return a->center_y < b->center_y;
        }
        if (std::abs(a->x - b->x) > 0.5f) {
            return a->x < b->x;
        }
        return a->width < b->width;
    });

    TextFragmentInfo* best_fragment = nullptr;
    float best_distance = std::numeric_limits<float>::max();
    for (auto* fragment : candidate_fragments) {
        const float distance = DistanceToFragmentBoxSquared(*fragment, click_x, click_y);
        if (distance < best_distance) {
            best_distance = distance;
            best_fragment = fragment;
        }
    }

    if (!best_fragment) {
        return false;
    }

    out_node = best_fragment->node;

    if (click_x <= best_fragment->x) {
        out_offset = 0;
        return true;
    }

    if (click_x >= best_fragment->x + best_fragment->width) {
        out_offset = best_fragment->char_count;
        return true;
    }

    const float local_x = click_x - best_fragment->x;
    const int resolved_offset = HitTestFragmentOffset(*best_fragment, local_x);

    out_offset = std::max(0, std::min(resolved_offset, best_fragment->char_count));
    return true;
}

} // namespace mbink
