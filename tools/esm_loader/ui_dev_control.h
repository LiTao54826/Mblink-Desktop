#pragma once

#include <string>

namespace mbink {
class QuickJSRuntime;
}

namespace mbink::ui_dev {

bool TryHandleUiDevCommand(QuickJSRuntime* runtime,
                           const std::string& command_path,
                           const std::string& response_path,
                           std::string* last_command_id,
                           bool* handled,
                           std::string* error = nullptr);

}  // namespace mbink::ui_dev
