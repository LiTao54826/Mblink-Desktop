#include "editor_input_session.h"

#include "clipboard_manager.h"
#include "contenteditable_controller.h"
#include "contenteditable_geometry.h"
#include "contenteditable_handler.h"
#include "input_edit_command.h"
#include "textarea_editing_controller.h"
#include "core/dom/document.h"
#include "core/dom/element.h"
#include "core/dom/elements/html_input_element.h"
#include "core/dom/elements/html_textarea_element.h"
#include "core/dom/selection/selection.h"
#include "core/render/input/input_paint_model.h"
#include "core/render/input/text_edit_metrics.h"
#include "core/render/objects/render_object.h"
#include "core/render/text/font_manager.h"
#include "core/event/input/keyboard_utils.h"
#include "core/utils/utf8_utils.h"
#include "core/window/window.h"
#include "include/core/SkFontMetrics.h"
#include <algorithm>
#include <cmath>

namespace mbink {

namespace {

float ResolveLineHeight(const ComputedStyle& style) {
    return std::max(1.0f, style.line_height * style.font_size);
}

SkFont BuildFont(const RenderObject* render_object) {
    FontDescriptor desc;
    if (render_object) {
        const auto& style = render_object->GetComputedStyle();
        desc.family = style.font_family.empty() ? "Arial" : style.font_family;
        desc.size = style.font_size > 0.0f ? style.font_size : 16.0f;
    }
    return FontManager::GetInstance().LoadFont(desc);
}

}

EditorInputSession::EditorInputSession() = default;
EditorInputSession::~EditorInputSession() = default;

bool EditorInputSession::IsEditorTarget(const std::shared_ptr<Element>& element) const {
    if (!element) return false;
    const std::string tag = element->GetTagName();
    return tag == "input" || tag == "textarea" || tag == "terminal" || element->IsContentEditable();
}

void EditorInputSession::PrepareTextInput(Window* window) {
    if (!window || !window->GetSDLWindow()) return;
    SDL_Window* sdl_window = window->GetSDLWindow();
    if (prepared_text_input_windows_.find(sdl_window) != prepared_text_input_windows_.end()) return;
    if (!SDL_TextInputActive(sdl_window)) {
        SDL_StartTextInput(sdl_window);
    }
    prepared_text_input_windows_.insert(sdl_window);
}

void EditorInputSession::SyncTextInputState(Window* window, const std::shared_ptr<Element>& element) {
    if (!window || !window->GetSDLWindow()) return;
    if (IsEditorTarget(element)) {
        PrepareTextInput(window);
        UpdateTextInputArea(window, element);
    }
}

void EditorInputSession::UpdateTextInputArea(Window* window, const std::shared_ptr<Element>& element) {
    if (!window || !element || !IsEditorTarget(element)) return;
    auto render_object = element->GetRenderObject();
    SDL_Window* sdl_window = window->GetSDLWindow();
    if (!sdl_window || !render_object) return;
    const auto& bounds = render_object->GetViewportBounds().valid ? render_object->GetViewportBounds() : (render_object->UpdateViewportBounds(), render_object->GetViewportBounds());
    if (!bounds.valid) return;
    const auto& style = render_object->GetComputedStyle();
    const auto& layout = render_object->GetLayoutInfo();
    SkFont font = BuildFont(render_object.get());
    SkFontMetrics metrics; font.getMetrics(&metrics);
    float line_height = ResolveLineHeight(style);
    float content_x = bounds.x + style.border_left_width + style.padding.left.ToPx(layout.width, style.font_size);
    float content_y = bounds.y + style.border_top_width + style.padding.top.ToPx(layout.height, style.font_size);
    float content_w = std::max(1.0f, bounds.width - style.border_left_width - style.border_right_width - style.padding.left.ToPx(layout.width, style.font_size) - style.padding.right.ToPx(layout.width, style.font_size));
    float area_x = content_x, area_y = content_y, area_w = content_w, area_h = std::max(1.0f, std::max(style.font_size, -metrics.fAscent + metrics.fDescent)), caret_x = content_x;
    if (auto input = std::dynamic_pointer_cast<HTMLInputElement>(element)) {
        auto paint_model = InputPaintModel::FromInputElement(input.get());
        int anchor = paint_model.HasComposition() ? paint_model.composition_start : paint_model.VisibleCaretPosition();
        caret_x += text_edit_metrics::MeasurePrefixWidth(paint_model.visual_text, anchor, font, paint_model.is_password && !paint_model.is_placeholder);
    } else if (auto textarea = std::dynamic_pointer_cast<HTMLTextAreaElement>(element)) {
        std::string value = textarea->GetValue();
        int anchor = textarea->GetSelectionEnd();
        if (auto edit_state = textarea->GetEditState(); edit_state && edit_state->HasActiveComposition()) anchor = edit_state->composition_state.start;
        std::string before = value.substr(0, utf8::CharPosToBytePos(value, anchor));
        int line = 0; for (char ch : before) if (ch == '\n') ++line;
        size_t last_newline = before.rfind('\n');
        std::string current_line = last_newline != std::string::npos ? before.substr(last_newline + 1) : before;
        caret_x = content_x + text_edit_metrics::MeasureTextWidth(current_line, font, false) - textarea->GetScrollLeft();
        area_y = content_y - textarea->GetScrollTop() + line * line_height;
    } else if (element->IsContentEditable()) {
        auto document = std::dynamic_pointer_cast<Document>(element->GetOwnerDocument());
        auto selection = document ? document->GetSelection() : nullptr;
        auto anchor_node = selection ? selection->GetFocusNode() : nullptr;
        int anchor_offset = selection ? selection->GetFocusOffset() : 0;
        if (contenteditable_handler_ && document && contenteditable_handler_->HasActiveComposition(document)) anchor_offset = contenteditable_handler_->GetCompositionState(document).start;
        auto rect = ComputeContentEditableCaretRect(element, anchor_node, anchor_offset);
        if (rect.valid) { area_x = rect.x; area_y = rect.y; area_h = std::max(1.0f, rect.height); caret_x = rect.x; }
    }
    float scale = std::max(1.0f, window->GetDisplayScale());
    SDL_Rect area{static_cast<int>(std::floor(area_x * scale)), static_cast<int>(std::floor(area_y * scale)), std::max(1, static_cast<int>(std::ceil(area_w * scale))), std::max(1, static_cast<int>(std::ceil(area_h * scale)))};
    SDL_SetTextInputArea(sdl_window, &area, std::max(0, std::min(area.w, static_cast<int>(std::round((caret_x - area_x) * scale)))));
}

bool EditorInputSession::HandleKeyDown(const SDL_Event& event, const std::shared_ptr<Element>& focus_element, const std::shared_ptr<Document>& document, const std::shared_ptr<Window>&, ClipboardManager* clipboard_manager, ContentEditableController* contenteditable_controller, bool ctrl_key, bool shift_key, bool alt_key, bool meta_key, bool default_prevented) {
    if (!focus_element || default_prevented) return false;
    std::string key = SDLKeycodeToKey(event.key.key, shift_key); int key_code = SDLKeycodeToKeyCode(event.key.key);
    if (ctrl_key && !alt_key && !shift_key && focus_element->IsContentEditable() && clipboard_manager && (key_code == 67 || key_code == 88 || key_code == 86)) return clipboard_manager->HandleKeyboardShortcut(document, key_code, ctrl_key, meta_key);
    if (auto input = std::dynamic_pointer_cast<HTMLInputElement>(focus_element)) {
        bool handled = shift_key && key == "ArrowLeft" ? input->ExecuteEditCommand(InputEditCommand::ExtendSelectionLeft()) : shift_key && key == "ArrowRight" ? input->ExecuteEditCommand(InputEditCommand::ExtendSelectionRight()) : shift_key && key == "Home" ? input->ExecuteEditCommand(InputEditCommand::ExtendSelectionToStart()) : shift_key && key == "End" ? input->ExecuteEditCommand(InputEditCommand::ExtendSelectionToEnd()) : key == "Backspace" ? input->ExecuteEditCommand(MakeCommand(InputEditCommandType::DeleteBackward)) : key == "Delete" ? input->ExecuteEditCommand(MakeCommand(InputEditCommandType::DeleteForward)) : key == "ArrowLeft" ? input->ExecuteEditCommand(MakeCommand(InputEditCommandType::MoveCaretLeft)) : key == "ArrowRight" ? input->ExecuteEditCommand(MakeCommand(InputEditCommandType::MoveCaretRight)) : key == "Home" ? input->ExecuteEditCommand(MakeCommand(InputEditCommandType::MoveCaretToStart)) : key == "End" ? input->ExecuteEditCommand(MakeCommand(InputEditCommandType::MoveCaretToEnd)) : ctrl_key && (key == "a" || key == "A") ? input->ExecuteEditCommand(MakeCommand(InputEditCommandType::SelectAll)) : ctrl_key && (key == "x" || key == "X") ? input->ExecuteEditCommand(MakeCommand(InputEditCommandType::CutSelection)) : false;
        if (!handled && ctrl_key && (key == "v" || key == "V")) { char* text = SDL_GetClipboardText(); if (text && text[0] != '\0') handled = input->ExecuteEditCommand(InputEditCommand::PasteText(text)); SDL_free(text); }
        if (!handled) input->HandleKeyPress(key, ctrl_key); return true;
    }
    if (auto textarea = std::dynamic_pointer_cast<HTMLTextAreaElement>(focus_element)) { textarea->HandleKeyPress(key, ctrl_key, shift_key); return true; }
    if (focus_element->IsContentEditable() && contenteditable_controller) { contenteditable_controller->HandleKeyDown(focus_element, key_code, ctrl_key, shift_key, alt_key); return true; }
    return false;
}

bool EditorInputSession::HandleTextInput(const SDL_Event& event, const std::shared_ptr<Element>& focus_element, const std::shared_ptr<Document>& document, ContentEditableController* contenteditable_controller) {
    if (!focus_element) return false; std::string text = event.text.text;
    if (auto input = std::dynamic_pointer_cast<HTMLInputElement>(focus_element)) { auto state = input->GetEditState(); state && state->HasActiveComposition() ? input->ExecuteEditCommand(InputEditCommand::CommitComposition(text, state->composition_state.start, state->composition_state.end)) : input->ExecuteEditCommand(InputEditCommand::InsertText(text)); return true; }
    if (auto textarea = std::dynamic_pointer_cast<HTMLTextAreaElement>(focus_element)) { auto state = textarea->GetEditState(); if (state && state->HasActiveComposition()) { TextAreaEditingController c(textarea.get(), state); CompositionCommandData d{text, state->composition_state.start, state->composition_state.end}; c.CommitComposition(d); } else textarea->HandleTextInput(text); return true; }
    return focus_element->IsContentEditable() && contenteditable_controller && document ? contenteditable_controller->HandleTextInput(focus_element, document, text) : false;
}

bool EditorInputSession::HandleTextEditing(const SDL_Event& event, const std::shared_ptr<Element>& focus_element, const std::shared_ptr<Document>& document, ContentEditableController* contenteditable_controller) {
    if (!focus_element) return false; std::string text = event.edit.text ? event.edit.text : "";
    if (auto input = std::dynamic_pointer_cast<HTMLInputElement>(focus_element)) { auto s = input->GetEditState(); if (!s) return true; if (text.empty()) { if (s->HasActiveComposition()) input->ExecuteEditCommand(InputEditCommand::CancelComposition()); return true; } int start = s->HasActiveComposition() ? s->composition_state.start : s->GetSelectionStart(); int end = s->HasActiveComposition() ? s->composition_state.end : s->GetSelectionEnd(); input->ExecuteEditCommand(s->HasActiveComposition() ? InputEditCommand::UpdateComposition(text, start, end) : InputEditCommand::StartComposition(text, start, end)); return true; }
    if (auto textarea = std::dynamic_pointer_cast<HTMLTextAreaElement>(focus_element)) { auto s = textarea->GetEditState(); if (!s) return true; TextAreaEditingController c(textarea.get(), s); if (text.empty()) { c.CancelComposition(); return true; } CompositionCommandData d{text, s->HasActiveComposition() ? s->composition_state.start : s->GetSelectionStart(), s->HasActiveComposition() ? s->composition_state.end : s->GetSelectionEnd()}; s->HasActiveComposition() ? c.UpdateComposition(d) : c.StartComposition(d); return true; }
    return focus_element->IsContentEditable() && contenteditable_controller && document ? contenteditable_controller->HandleTextEditing(focus_element, document, text) : false;
}

}  // namespace mbink
