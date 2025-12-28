/**
 * @file animation_optimizer.cpp
 * @brief 动画性能优化器实现
 */

#include "animation_optimizer.h"
#include "core/render/objects/render_object.h"
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace lightui {

// ============================================================================
// KeyframeInterpolationCache 实现
// ============================================================================

KeyframeInterpolationCache::KeyframeInterpolationCache(size_t max_entries)
    : max_entries_(max_entries)
    , hit_count_(0)
    , miss_count_(0) {
}

std::optional<std::map<std::string, std::string>>
KeyframeInterpolationCache::Get(const std::string& animation_name, float progress) const {
    std::string key = MakeKey(animation_name, progress);

    auto it = cache_.find(key);
    if (it != cache_.end() && it->second.is_valid) {
        // 命中缓存
        hit_count_++;
        // 注意：这里不能修改 timestamp，因为是 const 方法
        // 在实际使用中，可以使用 mutable 关键字
        return it->second.properties;
    }

    // 未命中
    miss_count_++;
    return std::nullopt;
}

void KeyframeInterpolationCache::Put(const std::string& animation_name, float progress,
                                     const std::map<std::string, std::string>& properties) const {
    // 检查是否需要清理
    if (cache_.size() >= max_entries_) {
        const_cast<KeyframeInterpolationCache*>(this)->EvictLRU();
    }

    std::string key = MakeKey(animation_name, progress);

    AnimationCacheEntry entry;
    entry.properties = properties;
    entry.progress = progress;
    entry.timestamp = std::chrono::steady_clock::now();
    entry.is_valid = true;

    const_cast<std::unordered_map<std::string, AnimationCacheEntry>&>(cache_)[key] = entry;
}

void KeyframeInterpolationCache::Invalidate(const std::string& animation_name) {
    // 移除所有与该动画相关的缓存
    for (auto it = cache_.begin(); it != cache_.end(); ) {
        if (it->first.find(animation_name) == 0) {
            it = cache_.erase(it);
        } else {
            ++it;
        }
    }
}

void KeyframeInterpolationCache::Clear() {
    cache_.clear();
}

float KeyframeInterpolationCache::GetHitRate() const {
    size_t total = hit_count_ + miss_count_;
    return total > 0 ? static_cast<float>(hit_count_) / total : 0.0f;
}

void KeyframeInterpolationCache::ResetStats() {
    hit_count_ = 0;
    miss_count_ = 0;
}

std::string KeyframeInterpolationCache::MakeKey(const std::string& animation_name, 
                                                float progress) const {
    // 将进度值量化到 0.01 精度，减少缓存条目数
    int quantized_progress = static_cast<int>(progress * 100.0f);
    
    std::ostringstream oss;
    oss << animation_name << "_" << quantized_progress;
    return oss.str();
}

void KeyframeInterpolationCache::EvictLRU() {
    if (cache_.empty()) {
        return;
    }
    
    // 找到最久未使用的条目
    auto oldest_it = cache_.begin();
    auto oldest_time = oldest_it->second.timestamp;
    
    for (auto it = cache_.begin(); it != cache_.end(); ++it) {
        if (it->second.timestamp < oldest_time) {
            oldest_it = it;
            oldest_time = it->second.timestamp;
        }
    }
    
    cache_.erase(oldest_it);
}

// ============================================================================
// AnimationDirtyTracker 实现
// ============================================================================

void AnimationDirtyTracker::MarkDirty(RenderObject* object, const std::string& animation_name) {
    if (!object) return;
    
    std::string key = MakeKey(object, animation_name);
    dirty_flags_[key] = true;
}

void AnimationDirtyTracker::MarkAllDirty(RenderObject* object) {
    if (!object) return;
    
    // 标记该对象的所有动画为脏
    // 注意：这里简化实现，实际应该遍历对象的所有动画
    std::string prefix = std::to_string(reinterpret_cast<uintptr_t>(object)) + "_";
    for (auto& [key, flag] : dirty_flags_) {
        if (key.find(prefix) == 0) {
            flag = true;
        }
    }
}

bool AnimationDirtyTracker::IsDirty(RenderObject* object, const std::string& animation_name) const {
    if (!object) return false;
    
    std::string key = MakeKey(object, animation_name);
    auto it = dirty_flags_.find(key);
    return it != dirty_flags_.end() && it->second;
}

void AnimationDirtyTracker::ClearDirty(RenderObject* object, const std::string& animation_name) {
    if (!object) return;
    
    std::string key = MakeKey(object, animation_name);
    dirty_flags_[key] = false;
}

void AnimationDirtyTracker::ClearAllDirty(RenderObject* object) {
    if (!object) return;
    
    std::string prefix = std::to_string(reinterpret_cast<uintptr_t>(object)) + "_";
    for (auto& [key, flag] : dirty_flags_) {
        if (key.find(prefix) == 0) {
            flag = false;
        }
    }
}

void AnimationDirtyTracker::Clear() {
    dirty_flags_.clear();
}

std::map<RenderObject*, std::vector<std::string>> 
AnimationDirtyTracker::GetDirtyAnimations() const {
    std::map<RenderObject*, std::vector<std::string>> result;
    
    for (const auto& [key, is_dirty] : dirty_flags_) {
        if (!is_dirty) continue;
        
        // 解析键：object_ptr_animation_name
        size_t underscore_pos = key.find('_');
        if (underscore_pos == std::string::npos) continue;
        
        std::string ptr_str = key.substr(0, underscore_pos);
        std::string animation_name = key.substr(underscore_pos + 1);
        
        uintptr_t ptr_value = std::stoull(ptr_str);
        RenderObject* object = reinterpret_cast<RenderObject*>(ptr_value);
        
        result[object].push_back(animation_name);
    }
    
    return result;
}

std::string AnimationDirtyTracker::MakeKey(RenderObject* object, 
                                           const std::string& animation_name) const {
    std::ostringstream oss;
    oss << reinterpret_cast<uintptr_t>(object) << "_" << animation_name;
    return oss.str();
}

// ============================================================================
// BatchAnimationUpdater 实现
// ============================================================================

void BatchAnimationUpdater::AddUpdateRequest(RenderObject* object, 
                                             const std::string& animation_name,
                                             double current_time) {
    if (!enabled_ || !object) return;
    
    requests_.emplace_back(object, animation_name, current_time);
}

size_t BatchAnimationUpdater::ExecuteAll() {
    if (!enabled_) return 0;
    
    size_t count = requests_.size();
    
    // TODO: 实际执行更新
    // 这里需要与 AnimationController 集成
    
    requests_.clear();
    return count;
}

void BatchAnimationUpdater::Clear() {
    requests_.clear();
}

// ============================================================================
// AnimationOptimizer 实现
// ============================================================================

AnimationOptimizer::AnimationOptimizer()
    : interpolation_cache_(100)
    , dirty_tracker_()
    , batch_updater_() {
}

AnimationOptimizer::OptimizationStats AnimationOptimizer::GetStats() const {
    OptimizationStats stats;
    stats.cache_hit_rate = interpolation_cache_.GetHitRate();
    stats.dirty_animation_count = dirty_tracker_.GetDirtyAnimations().size();
    stats.pending_update_count = batch_updater_.GetRequestCount();
    return stats;
}

void AnimationOptimizer::ResetStats() {
    interpolation_cache_.ResetStats();
}

void AnimationOptimizer::Clear() {
    interpolation_cache_.Clear();
    dirty_tracker_.Clear();
    batch_updater_.Clear();
}

} // namespace lightui

