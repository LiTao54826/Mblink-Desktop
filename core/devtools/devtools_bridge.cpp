#include "devtools_bridge.h"

#include <mutex>

namespace mbink {
namespace {

std::mutex g_bridge_mutex;
const DevToolsBridgeApi* g_bridge_api = nullptr;
std::mutex g_core_mutex;
const DevToolsCoreApi* g_core_api = nullptr;

DevToolsBounds FullBounds(float width, float height) {
    DevToolsBounds bounds;
    bounds.width = width;
    bounds.height = height;
    return bounds;
}

const DevToolsBridgeApi* SnapshotBridge() {
    std::lock_guard<std::mutex> lock(g_bridge_mutex);
    return g_bridge_api;
}

const DevToolsCoreApi* SnapshotCoreApi() {
    std::lock_guard<std::mutex> lock(g_core_mutex);
    return g_core_api;
}

}  // namespace

bool RegisterDevToolsBridge(const DevToolsBridgeApi* api) {
    if (!api || api->version != kDevToolsBridgeVersion) {
        return false;
    }
    std::lock_guard<std::mutex> lock(g_bridge_mutex);
    g_bridge_api = api;
    return true;
}

void UnregisterDevToolsBridge(const DevToolsBridgeApi* api) {
    std::lock_guard<std::mutex> lock(g_bridge_mutex);
    if (!api || g_bridge_api == api) {
        g_bridge_api = nullptr;
    }
}

bool HasDevToolsBridge() {
    std::lock_guard<std::mutex> lock(g_bridge_mutex);
    return g_bridge_api != nullptr;
}

const DevToolsBridgeApi* GetDevToolsBridge() {
    std::lock_guard<std::mutex> lock(g_bridge_mutex);
    return g_bridge_api;
}

bool RegisterDevToolsCoreApi(const DevToolsCoreApi* api) {
    if (!api || api->version != kDevToolsBridgeVersion) {
        return false;
    }
    std::lock_guard<std::mutex> lock(g_core_mutex);
    g_core_api = api;
    return true;
}

void UnregisterDevToolsCoreApi(const DevToolsCoreApi* api) {
    std::lock_guard<std::mutex> lock(g_core_mutex);
    if (!api || g_core_api == api) {
        g_core_api = nullptr;
    }
}

bool DevToolsIsOpen() {
    auto* api = SnapshotCoreApi();
    return api && api->is_open && api->is_open();
}

DevToolsDockPosition DevToolsGetDockPosition() {
    auto* api = SnapshotCoreApi();
    return api && api->dock_position ? api->dock_position() : DevToolsDockPosition::Bottom;
}

DevToolsBounds DevToolsGetMainAppBounds(float width, float height) {
    auto* api = SnapshotCoreApi();
    return api && api->main_app_bounds ? api->main_app_bounds(width, height) : FullBounds(width, height);
}

DevToolsBounds DevToolsGetPanelBounds(float width, float height) {
    auto* api = SnapshotCoreApi();
    return api && api->panel_bounds ? api->panel_bounds(width, height) : DevToolsBounds{};
}

bool DevToolsHandleKeyboardShortcut(int key, bool ctrl, bool shift, bool alt) {
    auto* api = SnapshotCoreApi();
    return api && api->handle_keyboard_shortcut &&
           api->handle_keyboard_shortcut(key, ctrl, shift, alt);
}

bool DevToolsHandleMouseWheel(int x, int y, float delta_x, float delta_y) {
    auto* api = SnapshotCoreApi();
    return api && api->handle_mouse_wheel &&
           api->handle_mouse_wheel(x, y, delta_x, delta_y);
}

bool DevToolsIsDraggingPanelBorder() {
    auto* api = SnapshotCoreApi();
    return api && api->is_dragging_panel_border && api->is_dragging_panel_border();
}

bool DevToolsIsMouseOnPanelBorder(float x, float y, float width, float height) {
    auto* api = SnapshotCoreApi();
    return api && api->is_mouse_on_panel_border &&
           api->is_mouse_on_panel_border(x, y, width, height);
}

bool DevToolsHandlePanelBorderDrag(float x, float y, float width, float height, bool pressed) {
    auto* api = SnapshotCoreApi();
    return api && api->handle_panel_border_drag &&
           api->handle_panel_border_drag(x, y, width, height, pressed);
}

bool DevToolsUpdatePanelBorderDrag(float x, float y, float width, float height) {
    auto* api = SnapshotCoreApi();
    return api && api->update_panel_border_drag &&
           api->update_panel_border_drag(x, y, width, height);
}

bool DevToolsIsDraggingSplitter() {
    auto* api = SnapshotCoreApi();
    return api && api->is_dragging_splitter && api->is_dragging_splitter();
}

bool DevToolsHandleMouseEvent(int x, int y, int button, bool pressed) {
    auto* api = SnapshotCoreApi();
    return api && api->handle_mouse_event &&
           api->handle_mouse_event(x, y, button, pressed);
}

bool DevToolsHandleMouseMove(int x, int y) {
    auto* api = SnapshotCoreApi();
    return api && api->handle_mouse_move && api->handle_mouse_move(x, y);
}

bool DevToolsIsMouseOnSplitter(int x, int y) {
    auto* api = SnapshotCoreApi();
    return api && api->is_mouse_on_splitter && api->is_mouse_on_splitter(x, y);
}

void DevToolsClearBoxModelHover() {
    auto* api = SnapshotCoreApi();
    if (api && api->clear_box_model_hover) api->clear_box_model_hover();
}

bool DevToolsIsPickerActive() {
    auto* api = SnapshotCoreApi();
    return api && api->is_picker_active && api->is_picker_active();
}

void DevToolsSetPickerHover(Element* element, RenderObject* render_object) {
    auto* api = SnapshotCoreApi();
    if (api && api->set_picker_hover) api->set_picker_hover(element, render_object);
}

void DevToolsSelectElement(Element* element) {
    auto* api = SnapshotCoreApi();
    if (api && api->select_element) api->select_element(element);
}

void DevToolsStopPicker() {
    auto* api = SnapshotCoreApi();
    if (api && api->stop_picker) api->stop_picker();
}

void DevToolsRenderHighlight(SkCanvas* canvas) {
    auto* api = SnapshotCoreApi();
    if (api && api->render_highlight) api->render_highlight(canvas);
}

void DevToolsRenderPanel(SkCanvas* canvas, float width, float height) {
    auto* api = SnapshotCoreApi();
    if (api && api->render_panel) api->render_panel(canvas, width, height);
}

int DevToolsHttpStart(const DevToolsHostContext& context,
                      const DevToolsHttpServerOptionsBridge& options,
                      DevToolsHttpServerInfoBridge& info,
                      char** out_error) {
    auto* api = SnapshotBridge();
    return api && api->http_start
               ? api->http_start(&context, &options, &info, out_error)
               : -3;
}

int DevToolsHttpStop(const DevToolsHostContext& context) {
    auto* api = SnapshotBridge();
    return api && api->http_stop ? api->http_stop(&context) : -3;
}

}  // namespace mbink
