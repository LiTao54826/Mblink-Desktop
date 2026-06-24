#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace mbink {

struct FileInfo {
    std::string path;
    std::string name;
    std::string type;
    std::string webkit_relative_path;
    std::uint64_t size = 0;
    std::int64_t last_modified = 0;
    bool is_directory = false;
};

using FileList = std::vector<FileInfo>;

FileInfo BuildFileInfoFromPath(const std::string& path);
FileList BuildFileListFromPaths(const std::vector<std::string>& paths);
FileList BuildFileListFromDirectoryPath(const std::string& path);
std::string FileListValueString(const FileList& files);
bool FileListsEqual(const FileList& a, const FileList& b);

} // namespace mbink
