#pragma once

#include <atomic>
#include <memory>
#include <string>

namespace mblink {
class Document;
class QuickJSRuntime;
class Window;
}

namespace mblink::ui_dev {

bool TryHandleUiDevCommand(QuickJSRuntime* runtime,
                           Window* window,
                           Document* document,
                           const std::string& command_path,
                           const std::string& response_path,
                           std::string* last_command_id,
                           std::string* runtime_epoch,
                           const std::shared_ptr<std::atomic<bool>>& shutdown_requested,
                           bool* handled,
                           std::string* error = nullptr);

}  // namespace mblink::ui_dev
