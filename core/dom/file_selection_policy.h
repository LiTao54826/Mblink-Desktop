#pragma once

#include "file_list.h"

#include <string>
#include <vector>

namespace mbink {

struct FileSelectionOptions {
    bool allow_multiple = false;
    bool allow_directories = false;
    std::string accept;
};

struct FileSelectionResult {
    FileList files;
    bool changed = false;
};

struct FileDialogAcceptFilter {
    std::string name;
    std::string pattern;
};

FileList SanitizeFileSelection(FileList candidates, const FileSelectionOptions& options);
FileSelectionResult ResolveFileSelection(const FileList& current,
                                         FileList candidates,
                                         const FileSelectionOptions& options);
std::vector<FileDialogAcceptFilter> BuildFileDialogAcceptFilters(const std::string& accept);

} // namespace mbink
