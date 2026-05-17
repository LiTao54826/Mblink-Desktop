#pragma once

#include <SDL3/SDL.h>
#include <memory>
#include <string>
#include <unordered_set>

namespace mbink {

class Document;
class Element;
class Window;
class ClipboardManager;
class ContentEditableController;
class ContentEditableHandler;

class EditorInputSession {
public:
    EditorInputSession();
    ~EditorInputSession();

    void SetContentEditableHandler(ContentEditableHandler* handler) { contenteditable_handler_ = handler; }

    bool IsEditorTarget(const std::shared_ptr<Element>& element) const;
    void PrepareTextInput(Window* window);
    void SyncTextInputState(Window* window, const std::shared_ptr<Element>& element);
    void UpdateTextInputArea(Window* window, const std::shared_ptr<Element>& element);

    bool HandleKeyDown(const SDL_Event& event,
                       const std::shared_ptr<Element>& focus_element,
                       const std::shared_ptr<Document>& document,
                       const std::shared_ptr<Window>& window,
                       ClipboardManager* clipboard_manager,
                       ContentEditableController* contenteditable_controller,
                       bool ctrl_key,
                       bool shift_key,
                       bool alt_key,
                       bool meta_key,
                       bool default_prevented);

    bool HandleTextInput(const SDL_Event& event,
                         const std::shared_ptr<Element>& focus_element,
                         const std::shared_ptr<Document>& document,
                         ContentEditableController* contenteditable_controller);

    bool HandleTextEditing(const SDL_Event& event,
                           const std::shared_ptr<Element>& focus_element,
                           const std::shared_ptr<Document>& document,
                           ContentEditableController* contenteditable_controller);

private:
    ContentEditableHandler* contenteditable_handler_ = nullptr;
    std::unordered_set<SDL_Window*> prepared_text_input_windows_;
};

}  // namespace mbink
