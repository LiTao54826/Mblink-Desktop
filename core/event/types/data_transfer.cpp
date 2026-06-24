/**
 * @file data_transfer.cpp
 * @brief DataTransfer类实现
 * 
 * 参考：
 * - W3C HTML5 - DataTransfer
 * - MDN Web Docs - DataTransfer
 */

#include "data_transfer.h"
#include <algorithm>
#include <cctype>
#include <iterator>
#include <utility>

namespace mbink {

// 格式规范化：转换为小写，处理别名
static std::string NormalizeFormat(const std::string& format) {
    if (format.empty()) {
        return format;
    }

    // 转换为小写
    std::string normalized = format;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    // 处理 W3C 规范中的别名
    // 参考：https://html.spec.whatwg.org/multipage/dnd.html#dom-datatransfer-setdata
    if (normalized == "text") {
        return "text/plain";
    } else if (normalized == "url") {
        return "text/uri-list";
    }

    return normalized;
}

DataTransfer::DataTransfer()
    : effect_allowed_(DragEffect::Uninitialized)
    , drop_effect_(DragEffect::None) {
}

void DataTransfer::SetData(const std::string& format, const std::string& data) {
    if (format.empty()) {
        return;  // 忽略空格式
    }

    std::string normalized = NormalizeFormat(format);
    
    // 如果是新格式，记录插入顺序
    if (data_.find(normalized) == data_.end()) {
        format_order_.push_back(normalized);
    }
    
    data_[normalized] = data;
}

std::string DataTransfer::GetData(const std::string& format) const {
    std::string normalized = NormalizeFormat(format);
    auto it = data_.find(normalized);
    if (it != data_.end()) {
        return it->second;
    }
    return "";
}

void DataTransfer::ClearData(const std::string& format) {
    if (format.empty()) {
        // 清除所有数据
        data_.clear();
        format_order_.clear();
        files_.clear();
    } else {
        // 清除特定格式的数据
        std::string normalized = NormalizeFormat(format);
        data_.erase(normalized);
        
        // 从顺序列表中移除
        auto it = std::find(format_order_.begin(), format_order_.end(), normalized);
        if (it != format_order_.end()) {
            format_order_.erase(it);
        }
    }
}

bool DataTransfer::HasData(const std::string& format) const {
    std::string normalized = NormalizeFormat(format);
    return data_.find(normalized) != data_.end();
}

std::vector<std::string> DataTransfer::GetTypes() const {
    // 返回按插入顺序排列的格式列表
    auto types = format_order_;
    if (!files_.empty() && std::find(types.begin(), types.end(), "Files") == types.end()) {
        types.push_back("Files");
    }
    return types;
}

void DataTransfer::SetFiles(FileList files) {
    files_ = std::move(files);
}

void DataTransfer::SetFilesFromPaths(const std::vector<std::string>& paths) {
    files_.clear();
    for (const auto& path : paths) {
        if (path.empty()) {
            continue;
        }

        auto file = BuildFileInfoFromPath(path);
        if (file.is_directory) {
            auto directory_files = BuildFileListFromDirectoryPath(file.path);
            files_.insert(files_.end(),
                          std::make_move_iterator(directory_files.begin()),
                          std::make_move_iterator(directory_files.end()));
        } else if (!file.path.empty()) {
            files_.push_back(std::move(file));
        }
    }
}

void DataTransfer::SetEffectAllowed(DragEffect effect) {
    effect_allowed_ = effect;
}

DragEffect DataTransfer::GetEffectAllowed() const {
    return effect_allowed_;
}

void DataTransfer::SetDropEffect(DragEffect effect) {
    drop_effect_ = effect;
}

DragEffect DataTransfer::GetDropEffect() const {
    return drop_effect_;
}

std::string DataTransfer::EffectToString(DragEffect effect) {
    switch (effect) {
        case DragEffect::None:
            return "none";
        case DragEffect::Copy:
            return "copy";
        case DragEffect::Move:
            return "move";
        case DragEffect::Link:
            return "link";
        case DragEffect::CopyMove:
            return "copyMove";
        case DragEffect::CopyLink:
            return "copyLink";
        case DragEffect::LinkMove:
            return "linkMove";
        case DragEffect::All:
            return "all";
        case DragEffect::Uninitialized:
            return "uninitialized";
        default:
            return "none";
    }
}

DragEffect DataTransfer::StringToEffect(const std::string& str) {
    if (str == "none") {
        return DragEffect::None;
    } else if (str == "copy") {
        return DragEffect::Copy;
    } else if (str == "move") {
        return DragEffect::Move;
    } else if (str == "link") {
        return DragEffect::Link;
    } else if (str == "copyMove") {
        return DragEffect::CopyMove;
    } else if (str == "copyLink") {
        return DragEffect::CopyLink;
    } else if (str == "linkMove") {
        return DragEffect::LinkMove;
    } else if (str == "all") {
        return DragEffect::All;
    } else if (str == "uninitialized") {
        return DragEffect::Uninitialized;
    } else {
        return DragEffect::None;
    }
}

} // namespace mbink

