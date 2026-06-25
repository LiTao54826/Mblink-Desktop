#include "file_list.h"

#include "core/utils/encoding_utils.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <utility>

namespace mblink {
namespace {

std::filesystem::path Utf8PathToFsPath(const std::string& path) {
#ifdef _WIN32
    return std::filesystem::path(utils::UTF8ToWide(path));
#else
    return std::filesystem::path(path);
#endif
}

std::string FsPathToUtf8String(const std::filesystem::path& path) {
#ifdef _WIN32
    return utils::WideToUTF8(path.wstring());
#else
    return path.string();
#endif
}

std::string NormalizePathString(const std::filesystem::path& path) {
    auto normalized = FsPathToUtf8String(path.lexically_normal());
    std::replace(normalized.begin(), normalized.end(), '\\', '/');
    return normalized;
}

std::int64_t FileTimeToUnixMs(std::filesystem::file_time_type time) {
    using namespace std::chrono;
    const auto system_time = time_point_cast<milliseconds>(
        time - std::filesystem::file_time_type::clock::now() + system_clock::now());
    return system_time.time_since_epoch().count();
}

std::string MimeTypeFromExtension(std::string extension) {
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });

    if (extension == ".txt" || extension == ".log" || extension == ".md") return "text/plain";
    if (extension == ".html" || extension == ".htm") return "text/html";
    if (extension == ".css") return "text/css";
    if (extension == ".csv") return "text/csv";
    if (extension == ".json") return "application/json";
    if (extension == ".js" || extension == ".mjs") return "text/javascript";
    if (extension == ".png") return "image/png";
    if (extension == ".jpg" || extension == ".jpeg") return "image/jpeg";
    if (extension == ".gif") return "image/gif";
    if (extension == ".webp") return "image/webp";
    if (extension == ".svg") return "image/svg+xml";
    if (extension == ".pdf") return "application/pdf";
    if (extension == ".zip") return "application/zip";
    return "";
}

} // namespace

FileInfo BuildFileInfoFromPath(const std::string& path) {
    FileInfo info;

    std::error_code ec;
    const auto fs_path = Utf8PathToFsPath(path);
    const auto absolute = std::filesystem::absolute(fs_path, ec);
    const auto normalized_path = ec ? fs_path.lexically_normal() : absolute.lexically_normal();

    info.path = NormalizePathString(normalized_path);
    info.name = FsPathToUtf8String(normalized_path.filename());

    ec.clear();
    info.is_directory = std::filesystem::is_directory(normalized_path, ec);

    ec.clear();
    if (!info.is_directory && std::filesystem::is_regular_file(normalized_path, ec)) {
        ec.clear();
        info.size = static_cast<std::uint64_t>(std::filesystem::file_size(normalized_path, ec));
        if (ec) info.size = 0;
    }

    ec.clear();
    const auto last_write = std::filesystem::last_write_time(normalized_path, ec);
    if (!ec) {
        info.last_modified = FileTimeToUnixMs(last_write);
    }

    if (!info.is_directory) {
        info.type = MimeTypeFromExtension(FsPathToUtf8String(normalized_path.extension()));
    }

    return info;
}

FileList BuildFileListFromPaths(const std::vector<std::string>& paths) {
    FileList files;
    files.reserve(paths.size());
    for (const auto& path : paths) {
        if (!path.empty()) {
            files.push_back(BuildFileInfoFromPath(path));
        }
    }
    return files;
}

FileList BuildFileListFromDirectoryPath(const std::string& path) {
    FileList files;

    std::error_code ec;
    auto root_path = std::filesystem::absolute(Utf8PathToFsPath(path), ec);
    if (ec) {
        root_path = Utf8PathToFsPath(path).lexically_normal();
    } else {
        root_path = root_path.lexically_normal();
    }

    ec.clear();
    if (!std::filesystem::is_directory(root_path, ec)) {
        return files;
    }

    std::vector<std::filesystem::path> file_paths;
    std::filesystem::recursive_directory_iterator it(
        root_path,
        std::filesystem::directory_options::skip_permission_denied,
        ec);
    std::filesystem::recursive_directory_iterator end;
    while (!ec && it != end) {
        const auto current_path = it->path();
        std::error_code file_ec;
        if (std::filesystem::is_regular_file(current_path, file_ec)) {
            file_paths.push_back(current_path.lexically_normal());
        }
        it.increment(ec);
    }

    std::sort(file_paths.begin(), file_paths.end(),
              [](const std::filesystem::path& a, const std::filesystem::path& b) {
                  return NormalizePathString(a) < NormalizePathString(b);
              });

    const std::string root_name = FsPathToUtf8String(root_path.filename());
    files.reserve(file_paths.size());
    for (const auto& file_path : file_paths) {
        auto info = BuildFileInfoFromPath(FsPathToUtf8String(file_path));

        std::error_code relative_ec;
        auto relative = std::filesystem::relative(file_path, root_path, relative_ec);
        std::string relative_path = relative_ec
            ? info.name
            : NormalizePathString(relative);
        if (!root_name.empty()) {
            relative_path = root_name + "/" + relative_path;
        }
        info.webkit_relative_path = relative_path;
        files.push_back(std::move(info));
    }

    return files;
}

std::string FileListValueString(const FileList& files) {
    if (files.empty()) {
        return "";
    }
    return "C:\\fakepath\\" + files[0].name;
}

bool FileListsEqual(const FileList& a, const FileList& b) {
    if (a.size() != b.size()) {
        return false;
    }

    for (size_t i = 0; i < a.size(); ++i) {
        if (a[i].path != b[i].path ||
            a[i].name != b[i].name ||
            a[i].type != b[i].type ||
            a[i].webkit_relative_path != b[i].webkit_relative_path ||
            a[i].size != b[i].size ||
            a[i].last_modified != b[i].last_modified ||
            a[i].is_directory != b[i].is_directory) {
            return false;
        }
    }

    return true;
}

} // namespace mblink
