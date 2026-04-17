#pragma once

#include <string>

#include "third_party/nlohmann/json.hpp"

namespace mbink::ui_dev {

bool SendDaemonRequest(const std::string& pipe_name,
                       const nlohmann::json& request,
                       nlohmann::json* response,
                       std::string* error = nullptr,
                       int timeout_ms = 3000);

bool WaitForDaemonReady(const std::string& pipe_name,
                        int timeout_ms,
                        std::string* error = nullptr);

}  // namespace mbink::ui_dev
