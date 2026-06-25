/**
 * @file style_cache.cpp
 * @brief 样式缓存系统实现
 */

#include "style_cache.h"
#include "core/dom/element.h"
#include "core/dom/node.h"
#include <algorithm>

namespace mblink {

// ========== 构造函数和析构函数 ==========

StyleCache::StyleCache()
    : hits_(0)
    , misses_(0)
    , max_cache_size_(0)  // 0表示无限制
    , access_counter_(0) {
}

StyleCache::~StyleCache() {
    Clear();
}

// ========== 缓存访问 ==========

const std::map<std::string, std::string>* StyleCache::GetCachedStyle(Element* element) const {
    if (!element) {
        return nullptr;
    }

    auto it = cache_.find(element);
    if (it != cache_.end()) {
        // 更新访问顺序（需要const_cast因为这是内部统计）
        auto* non_const_this = const_cast<StyleCache*>(this);
        non_const_this->access_order_[element] = non_const_this->access_counter_++;
        return &it->second;
    }

    return nullptr;
}

void StyleCache::SetCachedStyle(Element* element, const std::map<std::string, std::string>& style) {
    if (!element) {
        return;
    }

    // 检查是否需要淘汰
    EvictIfNeeded();

    // 设置缓存
    cache_[element] = style;
    access_order_[element] = access_counter_++;
}

bool StyleCache::HasCachedStyle(Element* element) const {
    if (!element) {
        return false;
    }

    return cache_.find(element) != cache_.end();
}

// ========== 缓存失效 ==========

void StyleCache::InvalidateElement(Element* element) {
    if (!element) {
        return;
    }

    auto it = cache_.find(element);
    if (it != cache_.end()) {
        cache_.erase(it);
        access_order_.erase(element);
    }
}

void StyleCache::InvalidateSubtree(Element* element) {
    if (!element) {
        return;
    }

    InvalidateSubtreeRecursive(element);
}

void StyleCache::InvalidateSubtreeRecursive(Element* element) {
    if (!element) {
        return;
    }

    // 失效当前元素
    InvalidateElement(element);

    // 递归失效子元素
    auto children = element->GetChildNodes();
    for (const auto& child : children) {
        if (child->GetNodeType() == NodeType::ELEMENT_NODE) {
            InvalidateSubtreeRecursive(static_cast<Element*>(child.get()));
        }
    }
}

void StyleCache::InvalidateAll() {
    cache_.clear();
    access_order_.clear();
}

// ========== 统计信息 ==========

size_t StyleCache::GetCacheSize() const {
    return cache_.size();
}

double StyleCache::GetHitRate() const {
    size_t total = hits_ + misses_;
    if (total == 0) {
        return 0.0;
    }
    return static_cast<double>(hits_) / static_cast<double>(total);
}

size_t StyleCache::GetHits() const {
    return hits_;
}

size_t StyleCache::GetMisses() const {
    return misses_;
}

void StyleCache::ResetStats() {
    hits_ = 0;
    misses_ = 0;
}

void StyleCache::RecordHit() {
    hits_++;
}

void StyleCache::RecordMiss() {
    misses_++;
}

// ========== 缓存管理 ==========

void StyleCache::Clear() {
    cache_.clear();
    access_order_.clear();
}

void StyleCache::SetMaxCacheSize(size_t max_size) {
    max_cache_size_ = max_size;
    EvictIfNeeded();
}

size_t StyleCache::GetMaxCacheSize() const {
    return max_cache_size_;
}

void StyleCache::EvictIfNeeded() {
    // 如果没有限制或未超过限制，不需要淘汰
    if (max_cache_size_ == 0 || cache_.size() < max_cache_size_) {
        return;
    }

    // 找到最久未访问的元素（LRU）
    Element* lru_element = nullptr;
    size_t min_access = SIZE_MAX;

    for (const auto& [element, access_time] : access_order_) {
        if (access_time < min_access) {
            min_access = access_time;
            lru_element = element;
        }
    }

    // 淘汰最久未访问的元素
    if (lru_element) {
        InvalidateElement(lru_element);
    }
}

} // namespace mblink

