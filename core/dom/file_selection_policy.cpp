#include "file_selection_policy.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <unordered_set>
#include <utility>

namespace mblink {
namespace {

std::string Trim(const std::string& value) {
    const char* whitespace = " \t\r\n";
    const auto start = value.find_first_not_of(whitespace);
    if (start == std::string::npos) {
        return "";
    }
    const auto end = value.find_last_not_of(whitespace);
    return value.substr(start, end - start + 1);
}

std::string ToLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return value;
}

std::vector<std::string> SplitAcceptTokens(const std::string& accept) {
    std::vector<std::string> tokens;
    std::stringstream stream(accept);
    std::string token;
    while (std::getline(stream, token, ',')) {
        token = ToLower(Trim(token));
        if (!token.empty()) {
            tokens.push_back(std::move(token));
        }
    }
    return tokens;
}

bool IsExtensionToken(const std::string& token) {
    return token.size() > 1 && token[0] == '.' && token.find('/') == std::string::npos;
}

bool IsMimeToken(const std::string& token) {
    const auto slash = token.find('/');
    return slash != std::string::npos &&
           slash > 0 &&
           slash + 1 < token.size() &&
           token.find(';') == std::string::npos;
}

bool IsWildcardMimeToken(const std::string& token) {
    return token.size() > 2 &&
           token.compare(token.size() - 2, 2, "/*") == 0 &&
           token.find('/') == token.size() - 2;
}

std::string FileExtension(const FileInfo& file) {
    const auto slash = file.path.find_last_of("/\\");
    const auto dot = file.path.find_last_of('.');
    if (dot == std::string::npos || (slash != std::string::npos && dot < slash)) {
        return "";
    }
    return ToLower(file.path.substr(dot));
}

bool MatchesAcceptToken(const FileInfo& file, const std::string& token) {
    if (IsExtensionToken(token)) {
        return FileExtension(file) == token;
    }

    if (!IsMimeToken(token)) {
        return false;
    }

    const std::string file_type = ToLower(file.type);
    if (file_type.empty()) {
        return false;
    }

    if (IsWildcardMimeToken(token)) {
        const std::string prefix = token.substr(0, token.size() - 1);
        return file_type.compare(0, prefix.size(), prefix) == 0;
    }

    return file_type == token;
}

std::vector<std::string> ValidAcceptTokens(const std::string& accept) {
    std::vector<std::string> valid;
    for (const auto& token : SplitAcceptTokens(accept)) {
        if (IsExtensionToken(token) || IsMimeToken(token)) {
            valid.push_back(token);
        }
    }
    return valid;
}

bool AcceptsFile(const FileInfo& file, const std::vector<std::string>& accept_tokens) {
    if (accept_tokens.empty()) {
        return true;
    }

    return std::any_of(accept_tokens.begin(), accept_tokens.end(),
                       [&file](const std::string& token) {
                           return MatchesAcceptToken(file, token);
                       });
}

std::string DialogPatternForMimeToken(const std::string& token) {
    if (token == "image/*") return "png;jpg;jpeg;gif;webp;svg";
    if (token == "audio/*") return "wav;mp3;ogg;flac;m4a";
    if (token == "video/*") return "mp4;webm;mov;mkv;avi";
    if (token == "text/*") return "txt;log;md;html;htm;css;csv";
    if (token == "application/*") return "json;pdf;zip";

    if (token == "image/png") return "png";
    if (token == "image/jpeg") return "jpg;jpeg";
    if (token == "image/gif") return "gif";
    if (token == "image/webp") return "webp";
    if (token == "image/svg+xml") return "svg";
    if (token == "text/plain") return "txt;log;md";
    if (token == "text/html") return "html;htm";
    if (token == "text/css") return "css";
    if (token == "text/csv") return "csv";
    if (token == "text/javascript") return "js;mjs";
    if (token == "application/json") return "json";
    if (token == "application/pdf") return "pdf";
    if (token == "application/zip") return "zip";
    return "";
}

} // namespace

FileList SanitizeFileSelection(FileList candidates, const FileSelectionOptions& options) {
    const auto accept_tokens = ValidAcceptTokens(options.accept);
    FileList files;
    files.reserve(candidates.size());

    for (auto& candidate : candidates) {
        if (candidate.is_directory) {
            if (!options.allow_directories) {
                continue;
            }

            auto directory_files = BuildFileListFromDirectoryPath(candidate.path);
            for (auto& file : directory_files) {
                if (AcceptsFile(file, accept_tokens)) {
                    files.push_back(std::move(file));
                }
            }
            if (!options.allow_multiple && !files.empty()) {
                break;
            }
            continue;
        }

        if (!AcceptsFile(candidate, accept_tokens)) {
            continue;
        }

        const bool expanded_directory_file = options.allow_directories &&
                                             !candidate.webkit_relative_path.empty();
        files.push_back(std::move(candidate));
        if (!options.allow_multiple && !expanded_directory_file && !files.empty()) {
            break;
        }
    }

    return files;
}

FileSelectionResult ResolveFileSelection(const FileList& current,
                                         FileList candidates,
                                         const FileSelectionOptions& options) {
    FileSelectionResult result;
    result.files = SanitizeFileSelection(std::move(candidates), options);
    result.changed = !FileListsEqual(current, result.files);
    return result;
}

std::vector<FileDialogAcceptFilter> BuildFileDialogAcceptFilters(const std::string& accept) {
    std::vector<FileDialogAcceptFilter> filters;
    std::unordered_set<std::string> seen;

    for (const auto& token : ValidAcceptTokens(accept)) {
        std::string pattern;
        if (IsExtensionToken(token)) {
            pattern = token.substr(1);
        } else {
            pattern = DialogPatternForMimeToken(token);
        }

        if (pattern.empty() || seen.find(pattern) != seen.end()) {
            continue;
        }
        seen.insert(pattern);

        filters.push_back(FileDialogAcceptFilter{
            token,
            pattern
        });
    }

    return filters;
}

} // namespace mblink
