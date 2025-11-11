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

namespace lightui {

DataTransfer::DataTransfer()
    : effect_allowed_(DragEffect::Uninitialized)
    , drop_effect_(DragEffect::None) {
}

void DataTransfer::SetData(const std::string& format, const std::string& data) {
    data_[format] = data;
}

std::string DataTransfer::GetData(const std::string& format) const {
    auto it = data_.find(format);
    if (it != data_.end()) {
        return it->second;
    }
    return "";
}

void DataTransfer::ClearData(const std::string& format) {
    if (format.empty()) {
        // 清除所有数据
        data_.clear();
    } else {
        // 清除特定格式的数据
        data_.erase(format);
    }
}

bool DataTransfer::HasData(const std::string& format) const {
    return data_.find(format) != data_.end();
}

std::vector<std::string> DataTransfer::GetTypes() const {
    std::vector<std::string> types;
    types.reserve(data_.size());
    
    for (const auto& [format, _] : data_) {
        types.push_back(format);
    }
    
    return types;
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

} // namespace lightui

