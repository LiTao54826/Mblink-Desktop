#pragma once

#include <memory>
#include <string>

namespace mbink {
class Document;
class EventLoop;
class QuickJSRuntime;
class Window;
}

namespace mbink::ui_dev {

struct RuntimeSupportOptions {
    std::string snapshot_file;
    std::string command_file;
    std::string response_file;
    std::string console_file;
    std::string errors_file;
    float quit_after_seconds = 0.0f;
};

void AttachStructuredRuntimeBuffers(QuickJSRuntime* runtime,
                                    const RuntimeSupportOptions& options);

void ConfigureRuntimeControl(EventLoop* event_loop,
                             QuickJSRuntime* runtime,
                             const std::shared_ptr<Window>& window,
                             const std::shared_ptr<Document>& document,
                             const RuntimeSupportOptions& options);

}  // namespace mbink::ui_dev
