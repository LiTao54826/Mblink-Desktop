#pragma once

#include <memory>
#include <string>

namespace mbink {
class Document;
class Window;
}

namespace mbink::ui_dev {

bool ExportUiDevSnapshot(const std::shared_ptr<Window>& window,
                         const std::shared_ptr<Document>& document,
                         const std::string& output_path,
                         std::string* error = nullptr);

}  // namespace mbink::ui_dev
