#pragma once

#include <atomic>
#include <cstddef>
#include <memory>
#include <string>

namespace mblink {
class Document;
class Window;
}

namespace mblink::ui_dev {

struct SnapshotExportOptions {
    std::string runtime_epoch;
    size_t max_nodes = 2000;
    int max_depth = 64;
    std::string root_selector;
    bool include_screenshot = false;
    bool inline_screenshot = false;
    std::string screenshot_path;
    std::shared_ptr<std::atomic<bool>> shutdown_requested;
};

bool ExportUiDevSnapshot(const std::shared_ptr<Window>& window,
                         const std::shared_ptr<Document>& document,
                         const std::string& output_path,
                         std::string* error = nullptr);

bool ExportUiDevSnapshot(const std::shared_ptr<Window>& window,
                         const std::shared_ptr<Document>& document,
                         const std::string& output_path,
                         const SnapshotExportOptions& options,
                         std::string* error = nullptr);

bool ExportUiDevSnapshot(Window* window,
                         Document* document,
                         const std::string& output_path,
                         const SnapshotExportOptions& options,
                         std::string* error = nullptr);

}  // namespace mblink::ui_dev
