#pragma once

#include <string>
#include <vector>

#include "common.h"
#include "third_party/nlohmann/json.hpp"

namespace mbink::ui_dev {

bool IsProcessRunning(int pid);
bool StartDetachedDaemon(const std::filesystem::path& executable_path,
                         const std::string& project_root,
                         const std::string& project_id,
                         int* spawned_pid,
                         std::string* error = nullptr);
nlohmann::json DispatchDaemonRequest(const nlohmann::json& request, DaemonState* state, bool* should_exit);
int RunDaemonServer(const std::vector<std::string>& args);

}  // namespace mbink::ui_dev
