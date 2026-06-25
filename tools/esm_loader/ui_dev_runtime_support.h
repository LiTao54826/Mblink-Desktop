#pragma once

#include <memory>
#include <string>

namespace mblink {
class Document;
class EventLoop;
class QuickJSRuntime;
class Window;
}

namespace mblink::ui_dev {

struct RuntimeSupportOptions {
    std::string snapshot_file;
    std::string command_file;
    std::string response_file;
    std::string console_file;
    std::string errors_file;
    std::string lifecycle_file;
    std::string runtime_epoch;
    size_t snapshot_max_nodes = 2000;
    int snapshot_max_depth = 64;
    std::string snapshot_root_selector;
    bool snapshot_include_screenshot = false;
    bool snapshot_inline_screenshot = false;
    std::string snapshot_screenshot_file;
    float quit_after_seconds = 0.0f;
};

void AttachStructuredRuntimeBuffers(QuickJSRuntime* runtime,
                                    const RuntimeSupportOptions& options);

void ConfigureRuntimeControl(EventLoop* event_loop,
                             QuickJSRuntime* runtime,
                             const std::shared_ptr<Window>& window,
                             const std::shared_ptr<Document>& document,
                             const RuntimeSupportOptions& options);

}  // namespace mblink::ui_dev
