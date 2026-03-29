/**
 * @file animation_optimizer.h
 * @brief 动画性能优化器
 *
 * 功能：
 * - 动画属性缓存
 * - 脏标记系统
 * - 批量更新优化
 * - 关键帧插值缓存
 */

#pragma once

#include "animation.h"
#include "keyframes.h"
#include <string>
#include <map>
#include <unordered_map>
#include <vector>
#include <optional>
#include <chrono>

namespace mbink {

// 前向声明
class RenderObject;

/**
 * @brief 动画属性缓存条目
 */
struct AnimationCacheEntry {
    std::map<std::string, std::string> properties;  ///< 缓存的属性值
    float progress;                                  ///< 进度值
    std::chrono::steady_clock::time_point timestamp; ///< 时间戳
    bool is_valid;                                   ///< 是否有效

    AnimationCacheEntry()
        : progress(0.0f)
        , timestamp(std::chrono::steady_clock::now())
        , is_valid(false) {
    }
};

/**
 * @brief 关键帧插值缓存
 * 
 * 缓存关键帧之间的插值结果，避免重复计算
 */
class KeyframeInterpolationCache {
public:
    /**
     * @brief 构造函数
     * @param max_entries 最大缓存条目数
     */
    explicit KeyframeInterpolationCache(size_t max_entries = 100);

    /**
     * @brief 获取缓存的插值结果
     * @param animation_name 动画名称
     * @param progress 进度值
     * @return 缓存的属性值，如果未命中返回 nullopt
     */
    std::optional<std::map<std::string, std::string>>
        Get(const std::string& animation_name, float progress) const;

    /**
     * @brief 添加插值结果到缓存
     * @param animation_name 动画名称
     * @param progress 进度值
     * @param properties 属性值
     */
    void Put(const std::string& animation_name, float progress,
             const std::map<std::string, std::string>& properties) const;

    /**
     * @brief 清空指定动画的缓存
     * @param animation_name 动画名称
     */
    void Invalidate(const std::string& animation_name);

    /**
     * @brief 清空所有缓存
     */
    void Clear();

    /**
     * @brief 获取缓存命中率
     */
    float GetHitRate() const;

    /**
     * @brief 重置统计信息
     */
    void ResetStats();

private:
    /**
     * @brief 生成缓存键
     */
    std::string MakeKey(const std::string& animation_name, float progress) const;

    /**
     * @brief 清理过期缓存（LRU）
     */
    void EvictLRU();

    size_t max_entries_;                                    ///< 最大条目数
    mutable std::unordered_map<std::string, AnimationCacheEntry> cache_; ///< 缓存映射
    mutable size_t hit_count_;                              ///< 命中次数
    mutable size_t miss_count_;                             ///< 未命中次数
};

/**
 * @brief 动画脏标记管理器
 * 
 * 跟踪哪些动画需要更新，避免不必要的计算
 */
class AnimationDirtyTracker {
public:
    /**
     * @brief 标记动画为脏
     * @param object 渲染对象
     * @param animation_name 动画名称
     */
    void MarkDirty(RenderObject* object, const std::string& animation_name);

    /**
     * @brief 标记对象的所有动画为脏
     * @param object 渲染对象
     */
    void MarkAllDirty(RenderObject* object);

    /**
     * @brief 检查动画是否为脏
     * @param object 渲染对象
     * @param animation_name 动画名称
     * @return 是否为脏
     */
    bool IsDirty(RenderObject* object, const std::string& animation_name) const;

    /**
     * @brief 清除脏标记
     * @param object 渲染对象
     * @param animation_name 动画名称
     */
    void ClearDirty(RenderObject* object, const std::string& animation_name);

    /**
     * @brief 清除对象的所有脏标记
     * @param object 渲染对象
     */
    void ClearAllDirty(RenderObject* object);

    /**
     * @brief 清除所有脏标记
     */
    void Clear();

    /**
     * @brief 获取所有脏动画
     * @return 脏动画列表 (对象 -> 动画名称列表)
     */
    std::map<RenderObject*, std::vector<std::string>> GetDirtyAnimations() const;

private:
    /**
     * @brief 生成键
     */
    std::string MakeKey(RenderObject* object, const std::string& animation_name) const;

    std::unordered_map<std::string, bool> dirty_flags_; ///< 脏标记映射
};

/**
 * @brief 批量动画更新器
 * 
 * 收集多个动画更新，批量处理以提高性能
 */
class BatchAnimationUpdater {
public:
    /**
     * @brief 动画更新请求
     */
    struct UpdateRequest {
        RenderObject* object;
        std::string animation_name;
        double current_time;
        
        UpdateRequest(RenderObject* obj, const std::string& name, double time)
            : object(obj), animation_name(name), current_time(time) {
        }
    };

    /**
     * @brief 添加更新请求
     * @param object 渲染对象
     * @param animation_name 动画名称
     * @param current_time 当前时间
     */
    void AddUpdateRequest(RenderObject* object, const std::string& animation_name, 
                          double current_time);

    /**
     * @brief 执行所有更新请求
     * @return 更新的动画数量
     */
    size_t ExecuteAll();

    /**
     * @brief 清空所有请求
     */
    void Clear();

    /**
     * @brief 获取请求数量
     */
    size_t GetRequestCount() const { return requests_.size(); }

    /**
     * @brief 获取所有待处理的请求
     * @return 请求列表
     */
    const std::vector<UpdateRequest>& GetPendingRequests() const { return requests_; }

    /**
     * @brief 启用/禁用批量更新
     * @param enabled 是否启用
     */
    void SetEnabled(bool enabled) { enabled_ = enabled; }

    /**
     * @brief 检查是否启用
     */
    bool IsEnabled() const { return enabled_; }

private:
    std::vector<UpdateRequest> requests_; ///< 更新请求列表
    bool enabled_ = true;                 ///< 是否启用批量更新
};

/**
 * @brief 动画性能优化器
 * 
 * 集成所有动画优化功能
 */
class AnimationOptimizer {
public:
    /**
     * @brief 构造函数
     */
    AnimationOptimizer();

    /**
     * @brief 析构函数
     */
    ~AnimationOptimizer() = default;

    // ========== 缓存管理 ==========

    /**
     * @brief 获取关键帧插值缓存
     */
    KeyframeInterpolationCache& GetInterpolationCache() {
        return interpolation_cache_;
    }

    /**
     * @brief 获取关键帧插值缓存（const）
     */
    const KeyframeInterpolationCache& GetInterpolationCache() const {
        return interpolation_cache_;
    }

    // ========== 脏标记管理 ==========

    /**
     * @brief 获取脏标记跟踪器
     */
    AnimationDirtyTracker& GetDirtyTracker() {
        return dirty_tracker_;
    }

    /**
     * @brief 获取脏标记跟踪器（const）
     */
    const AnimationDirtyTracker& GetDirtyTracker() const {
        return dirty_tracker_;
    }

    // ========== 批量更新 ==========

    /**
     * @brief 获取批量更新器
     */
    BatchAnimationUpdater& GetBatchUpdater() {
        return batch_updater_;
    }

    /**
     * @brief 获取批量更新器（const）
     */
    const BatchAnimationUpdater& GetBatchUpdater() const {
        return batch_updater_;
    }

    // ========== 统计信息 ==========

    /**
     * @brief 获取优化统计信息
     */
    struct OptimizationStats {
        float cache_hit_rate;
        size_t dirty_animation_count;
        size_t pending_update_count;
    };

    OptimizationStats GetStats() const;

    /**
     * @brief 重置所有统计信息
     */
    void ResetStats();

    /**
     * @brief 清空所有缓存和状态
     */
    void Clear();

    /**
     * @brief 重置优化器（清空缓存和状态）
     */
    void Reset() { Clear(); }

private:
    KeyframeInterpolationCache interpolation_cache_; ///< 插值缓存
    AnimationDirtyTracker dirty_tracker_;            ///< 脏标记跟踪器
    BatchAnimationUpdater batch_updater_;            ///< 批量更新器
};

} // namespace mbink

