#pragma once

#include <cstddef>

class SkCanvas;

namespace mbink {

class Document;
class Element;
class QuickJSRuntime;
class RenderObject;
class Window;

inline constexpr unsigned int kDevToolsBridgeVersion = 4;

enum class DevToolsDockPosition {
    Bottom,
    Right
};

struct UiDevSnapshotOptionsBridge {
    const char* runtime_epoch = nullptr;
    size_t max_nodes = 2000;
    int max_depth = 64;
    const char* root_selector = nullptr;
    bool include_screenshot = false;
    bool inline_screenshot = false;
    const char* screenshot_file = nullptr;
    bool shutdown_requested = false;
};

struct DevToolsHostContext {
    unsigned int version = kDevToolsBridgeVersion;
    Window* window = nullptr;
    Document* document = nullptr;
    QuickJSRuntime* runtime = nullptr;
    const char* runtime_epoch = nullptr;
    bool shutdown_requested = false;
    void* host_user_data = nullptr;
};

using DevToolsMainThreadTask = void (*)(void*);
using DevToolsHostContextTask = void (*)(const DevToolsHostContext*, void*);

struct DevToolsHostServices {
    unsigned int version = kDevToolsBridgeVersion;
    char* (*copy_string)(const char*) = nullptr;
    void (*free_string)(void*) = nullptr;
    bool (*is_main_thread)(const DevToolsHostContext*) = nullptr;
    int (*run_on_main_thread_sync)(const DevToolsHostContext*, DevToolsMainThreadTask, void*) = nullptr;
    void (*wake_event_loop)(const DevToolsHostContext*) = nullptr;
    void (*set_main_thread_sync_cancelled)(void*, bool) = nullptr;
    int (*with_current_context_sync)(void*, DevToolsHostContextTask, void*) = nullptr;
    int (*handle_http_mcp_json)(void*, const char*, char**) = nullptr;
};

struct DevToolsHttpServerOptionsBridge {
    const char* bind_host = nullptr;
    unsigned short port = 0;
    const char* auth_token = nullptr;
    bool require_auth = true;
};

struct DevToolsHttpServerInfoBridge {
    unsigned short port = 0;
    char* url = nullptr;
    char* auth_token = nullptr;
};

struct DevToolsBounds {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
};

struct DevToolsCoreApi {
    unsigned int version = kDevToolsBridgeVersion;
    bool (*is_open)() = nullptr;
    DevToolsDockPosition (*dock_position)() = nullptr;
    DevToolsBounds (*main_app_bounds)(float, float) = nullptr;
    DevToolsBounds (*panel_bounds)(float, float) = nullptr;
    bool (*handle_keyboard_shortcut)(int, bool, bool, bool) = nullptr;
    bool (*handle_mouse_wheel)(int, int, float, float) = nullptr;
    bool (*is_dragging_panel_border)() = nullptr;
    bool (*is_mouse_on_panel_border)(float, float, float, float) = nullptr;
    bool (*handle_panel_border_drag)(float, float, float, float, bool) = nullptr;
    bool (*update_panel_border_drag)(float, float, float, float) = nullptr;
    bool (*is_dragging_splitter)() = nullptr;
    bool (*handle_mouse_event)(int, int, int, bool) = nullptr;
    bool (*handle_mouse_move)(int, int) = nullptr;
    bool (*is_mouse_on_splitter)(int, int) = nullptr;
    void (*clear_box_model_hover)() = nullptr;
    bool (*is_picker_active)() = nullptr;
    void (*set_picker_hover)(Element*, RenderObject*) = nullptr;
    void (*select_element)(Element*) = nullptr;
    void (*stop_picker)() = nullptr;
    void (*render_highlight)(SkCanvas*) = nullptr;
    void (*render_panel)(SkCanvas*, float, float) = nullptr;
};

struct DevToolsBridgeApi {
    unsigned int version = kDevToolsBridgeVersion;
    int (*http_start)(const DevToolsHostContext*, const DevToolsHttpServerOptionsBridge*, DevToolsHttpServerInfoBridge*, char**) = nullptr;
    int (*http_stop)(const DevToolsHostContext*) = nullptr;
};

using DevToolsRegisterBridgeFn = bool (*)(unsigned int, const DevToolsBridgeApi*);
using DevToolsUnregisterBridgeFn = void (*)(unsigned int, const DevToolsBridgeApi*);
using MbinkDevToolsAttachFn = int (*)(unsigned int,
                                      const DevToolsHostServices*,
                                      DevToolsRegisterBridgeFn,
                                      DevToolsUnregisterBridgeFn);

bool RegisterDevToolsBridge(const DevToolsBridgeApi* api);
void UnregisterDevToolsBridge(const DevToolsBridgeApi* api);
bool HasDevToolsBridge();
const DevToolsBridgeApi* GetDevToolsBridge();

bool RegisterDevToolsCoreApi(const DevToolsCoreApi* api);
void UnregisterDevToolsCoreApi(const DevToolsCoreApi* api);

bool DevToolsIsOpen();
DevToolsDockPosition DevToolsGetDockPosition();
DevToolsBounds DevToolsGetMainAppBounds(float width, float height);
DevToolsBounds DevToolsGetPanelBounds(float width, float height);
bool DevToolsHandleKeyboardShortcut(int key, bool ctrl, bool shift, bool alt);
bool DevToolsHandleMouseWheel(int x, int y, float delta_x, float delta_y);
bool DevToolsIsDraggingPanelBorder();
bool DevToolsIsMouseOnPanelBorder(float x, float y, float width, float height);
bool DevToolsHandlePanelBorderDrag(float x, float y, float width, float height, bool pressed);
bool DevToolsUpdatePanelBorderDrag(float x, float y, float width, float height);
bool DevToolsIsDraggingSplitter();
bool DevToolsHandleMouseEvent(int x, int y, int button, bool pressed);
bool DevToolsHandleMouseMove(int x, int y);
bool DevToolsIsMouseOnSplitter(int x, int y);
void DevToolsClearBoxModelHover();
bool DevToolsIsPickerActive();
void DevToolsSetPickerHover(Element* element, RenderObject* render_object);
void DevToolsSelectElement(Element* element);
void DevToolsStopPicker();
void DevToolsRenderHighlight(SkCanvas* canvas);
void DevToolsRenderPanel(SkCanvas* canvas, float width, float height);

int DevToolsHttpStart(const DevToolsHostContext& context,
                      const DevToolsHttpServerOptionsBridge& options,
                      DevToolsHttpServerInfoBridge& info,
                      char** out_error);
int DevToolsHttpStop(const DevToolsHostContext& context);

}  // namespace mbink
