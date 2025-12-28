/**
 * @file filter_cache.cpp
 * @brief CSS 滤镜缓存系统实现
 */

#include "filter_cache.h"
#include "utils/transform.h"
#include <sstream>
#include <algorithm>

namespace lightui {

// ============================================================================
// FilterCache 实现
// ============================================================================

FilterCache::FilterCache(size_t max_entries)
    : max_entries_(max_entries)
    , hit_count_(0)
    , miss_count_(0) {
}

sk_sp<SkImageFilter> FilterCache::Get(const CSSFilterList& filter_list) {
    std::string key = MakeKey(filter_list);
    
    auto it = cache_.find(key);
    if (it != cache_.end()) {
        // 命中缓存
        hit_count_++;
        it->second.last_access = std::chrono::steady_clock::now();
        it->second.access_count++;
        return it->second.filter;
    }
    
    // 未命中
    miss_count_++;
    return nullptr;
}

void FilterCache::Put(const CSSFilterList& filter_list, sk_sp<SkImageFilter> skia_filter) {
    if (!skia_filter) return;
    
    // 检查是否需要清理
    if (cache_.size() >= max_entries_) {
        EvictLRU();
    }
    
    std::string key = MakeKey(filter_list);
    
    FilterCacheEntry entry;
    entry.filter = skia_filter;
    entry.filter_string = key;
    entry.last_access = std::chrono::steady_clock::now();
    entry.access_count = 1;
    
    cache_[key] = entry;
}

void FilterCache::Clear() {
    cache_.clear();
}

float FilterCache::GetHitRate() const {
    size_t total = hit_count_ + miss_count_;
    return total > 0 ? static_cast<float>(hit_count_) / total : 0.0f;
}

void FilterCache::ResetStats() {
    hit_count_ = 0;
    miss_count_ = 0;
}

std::string FilterCache::MakeKey(const CSSFilterList& filter_list) const {
    std::ostringstream oss;

    const auto& filters = filter_list.GetFilters();
    for (size_t i = 0; i < filters.size(); ++i) {
        const CSSFilter& filter = filters[i];

        oss << static_cast<int>(filter.type) << ":";

        switch (filter.type) {
            case CSSFilterType::Blur:
                oss << filter.value;
                break;
            case CSSFilterType::Brightness:
            case CSSFilterType::Contrast:
            case CSSFilterType::Grayscale:
            case CSSFilterType::Sepia:
            case CSSFilterType::Saturate:
            case CSSFilterType::Invert:
            case CSSFilterType::Opacity:
                oss << filter.amount;
                break;
            case CSSFilterType::HueRotate:
                oss << filter.angle;
                break;
            case CSSFilterType::DropShadow:
                oss << filter.offset_x << "," << filter.offset_y << ","
                    << filter.blur_radius << "," << filter.color;
                break;
        }

        if (i < filters.size() - 1) {
            oss << ";";
        }
    }

    return oss.str();
}

void FilterCache::EvictLRU() {
    if (cache_.empty()) {
        return;
    }
    
    // 找到最久未使用的条目
    auto oldest_it = cache_.begin();
    auto oldest_time = oldest_it->second.last_access;
    
    for (auto it = cache_.begin(); it != cache_.end(); ++it) {
        if (it->second.last_access < oldest_time) {
            oldest_it = it;
            oldest_time = it->second.last_access;
        }
    }
    
    cache_.erase(oldest_it);
}

// ============================================================================
// TransformMatrixCache 实现
// ============================================================================

TransformMatrixCache::TransformMatrixCache(size_t max_entries)
    : max_entries_(max_entries)
    , hit_count_(0)
    , miss_count_(0) {
}

std::optional<SkMatrix> TransformMatrixCache::Get(const std::string& transform_str) {
    std::string key = MakeKey(transform_str);

    auto it = cache_.find(key);
    if (it != cache_.end()) {
        // 命中缓存
        hit_count_++;
        it->second.last_access = std::chrono::steady_clock::now();
        it->second.access_count++;
        return it->second.matrix;
    }

    // 未命中
    miss_count_++;
    return std::nullopt;
}

void TransformMatrixCache::Put(const std::string& transform_str, const SkMatrix& matrix) {
    // 检查是否需要清理
    if (cache_.size() >= max_entries_) {
        EvictLRU();
    }

    std::string key = MakeKey(transform_str);

    MatrixCacheEntry entry;
    entry.matrix = matrix;
    entry.transform_string = key;
    entry.last_access = std::chrono::steady_clock::now();
    entry.access_count = 1;

    cache_[key] = entry;
}

void TransformMatrixCache::Clear() {
    cache_.clear();
}

float TransformMatrixCache::GetHitRate() const {
    size_t total = hit_count_ + miss_count_;
    return total > 0 ? static_cast<float>(hit_count_) / total : 0.0f;
}

void TransformMatrixCache::ResetStats() {
    hit_count_ = 0;
    miss_count_ = 0;
}

std::string TransformMatrixCache::MakeKey(const std::string& transform_str) const {
    // 直接使用 transform 字符串作为键
    return transform_str;
}

void TransformMatrixCache::EvictLRU() {
    if (cache_.empty()) {
        return;
    }
    
    // 找到最久未使用的条目
    auto oldest_it = cache_.begin();
    auto oldest_time = oldest_it->second.last_access;
    
    for (auto it = cache_.begin(); it != cache_.end(); ++it) {
        if (it->second.last_access < oldest_time) {
            oldest_it = it;
            oldest_time = it->second.last_access;
        }
    }
    
    cache_.erase(oldest_it);
}

// ============================================================================
// RenderOptimizer 实现
// ============================================================================

RenderOptimizer::RenderOptimizer()
    : filter_cache_(50)
    , transform_cache_(100) {
}

RenderOptimizer::OptimizationStats RenderOptimizer::GetStats() const {
    OptimizationStats stats;
    stats.filter_cache_hit_rate = filter_cache_.GetHitRate();
    stats.transform_cache_hit_rate = transform_cache_.GetHitRate();
    stats.filter_cache_entries = filter_cache_.GetEntryCount();
    stats.transform_cache_entries = transform_cache_.GetEntryCount();
    return stats;
}

void RenderOptimizer::ResetStats() {
    filter_cache_.ResetStats();
    transform_cache_.ResetStats();
}

void RenderOptimizer::Clear() {
    filter_cache_.Clear();
    transform_cache_.Clear();
}

} // namespace lightui

