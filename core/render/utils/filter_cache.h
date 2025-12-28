/**
 * @file filter_cache.h
 * @brief CSS 滤镜缓存系统
 *
 * 功能：
 * - 缓存滤镜渲染结果
 * - 缓存 Skia 滤镜对象
 * - 避免重复创建滤镜
 * - LRU 缓存策略
 */

#pragma once

#include "css/css_filters.h"
#include "transform.h"
#include "include/core/SkImageFilter.h"
#include "include/core/SkMatrix.h"
#include <string>
#include <unordered_map>
#include <chrono>
#include <optional>

namespace lightui {

/**
 * @brief 滤镜缓存条目
 */
struct FilterCacheEntry {
    sk_sp<SkImageFilter> filter;                         ///< 缓存的 Skia 滤镜
    std::string filter_string;                           ///< 滤镜字符串表示
    std::chrono::steady_clock::time_point last_access;   ///< 最后访问时间
    size_t access_count;                                 ///< 访问次数

    FilterCacheEntry()
        : last_access(std::chrono::steady_clock::now())
        , access_count(0) {
    }
};

/**
 * @brief CSS 滤镜缓存
 * 
 * 缓存 CSS 滤镜的 Skia 对象，避免重复创建
 */
class FilterCache {
public:
    /**
     * @brief 构造函数
     * @param max_entries 最大缓存条目数
     */
    explicit FilterCache(size_t max_entries = 50);

    /**
     * @brief 析构函数
     */
    ~FilterCache() = default;

    /**
     * @brief 获取缓存的滤镜
     * @param filter_list 滤镜列表
     * @return 缓存的 Skia 滤镜，未命中返回 nullptr
     */
    sk_sp<SkImageFilter> Get(const CSSFilterList& filter_list);

    /**
     * @brief 添加滤镜到缓存
     * @param filter_list 滤镜列表
     * @param skia_filter Skia 滤镜对象
     */
    void Put(const CSSFilterList& filter_list, sk_sp<SkImageFilter> skia_filter);

    /**
     * @brief 清空缓存
     */
    void Clear();

    /**
     * @brief 获取缓存命中率
     */
    float GetHitRate() const;

    /**
     * @brief 获取缓存条目数
     */
    size_t GetEntryCount() const { return cache_.size(); }

    /**
     * @brief 重置统计信息
     */
    void ResetStats();

private:
    /**
     * @brief 生成滤镜的缓存键
     * @param filter_list 滤镜列表
     * @return 缓存键
     */
    std::string MakeKey(const CSSFilterList& filter_list) const;

    /**
     * @brief 清理最久未使用的条目（LRU）
     */
    void EvictLRU();

    size_t max_entries_;                                    ///< 最大条目数
    std::unordered_map<std::string, FilterCacheEntry> cache_; ///< 缓存映射
    size_t hit_count_;                                      ///< 命中次数
    size_t miss_count_;                                     ///< 未命中次数
};

/**
 * @brief 变换矩阵缓存
 * 
 * 缓存 CSS Transform 的矩阵计算结果
 */
class TransformMatrixCache {
public:
    /**
     * @brief 矩阵缓存条目
     */
    struct MatrixCacheEntry {
        SkMatrix matrix;                                     ///< 缓存的矩阵
        std::string transform_string;                        ///< Transform 字符串
        std::chrono::steady_clock::time_point last_access;   ///< 最后访问时间
        size_t access_count;                                 ///< 访问次数

        MatrixCacheEntry()
            : last_access(std::chrono::steady_clock::now())
            , access_count(0) {
        }
    };

    /**
     * @brief 构造函数
     * @param max_entries 最大缓存条目数
     */
    explicit TransformMatrixCache(size_t max_entries = 100);

    /**
     * @brief 析构函数
     */
    ~TransformMatrixCache() = default;

    /**
     * @brief 获取缓存的矩阵
     * @param transform_str Transform 字符串
     * @return 缓存的矩阵，未命中返回 nullopt
     */
    std::optional<SkMatrix> Get(const std::string& transform_str);

    /**
     * @brief 添加矩阵到缓存
     * @param transform_str Transform 字符串
     * @param matrix 矩阵
     */
    void Put(const std::string& transform_str, const SkMatrix& matrix);

    /**
     * @brief 清空缓存
     */
    void Clear();

    /**
     * @brief 获取缓存命中率
     */
    float GetHitRate() const;

    /**
     * @brief 获取缓存条目数
     */
    size_t GetEntryCount() const { return cache_.size(); }

    /**
     * @brief 重置统计信息
     */
    void ResetStats();

private:
    /**
     * @brief 生成 Transform 的缓存键
     * @param transform_str Transform 字符串
     * @return 缓存键
     */
    std::string MakeKey(const std::string& transform_str) const;

    /**
     * @brief 清理最久未使用的条目（LRU）
     */
    void EvictLRU();

    size_t max_entries_;                                    ///< 最大条目数
    std::unordered_map<std::string, MatrixCacheEntry> cache_; ///< 缓存映射
    size_t hit_count_;                                      ///< 命中次数
    size_t miss_count_;                                     ///< 未命中次数
};

/**
 * @brief 渲染优化器
 * 
 * 集成滤镜缓存和变换矩阵缓存
 */
class RenderOptimizer {
public:
    /**
     * @brief 构造函数
     */
    RenderOptimizer();

    /**
     * @brief 析构函数
     */
    ~RenderOptimizer() = default;

    /**
     * @brief 获取滤镜缓存
     */
    FilterCache& GetFilterCache() { return filter_cache_; }

    /**
     * @brief 获取滤镜缓存（const）
     */
    const FilterCache& GetFilterCache() const { return filter_cache_; }

    /**
     * @brief 获取变换矩阵缓存
     */
    TransformMatrixCache& GetTransformCache() { return transform_cache_; }

    /**
     * @brief 获取变换矩阵缓存（const）
     */
    const TransformMatrixCache& GetTransformCache() const { return transform_cache_; }

    /**
     * @brief 获取优化统计信息
     */
    struct OptimizationStats {
        float filter_cache_hit_rate;
        float transform_cache_hit_rate;
        size_t filter_cache_entries;
        size_t transform_cache_entries;
    };

    OptimizationStats GetStats() const;

    /**
     * @brief 重置所有统计信息
     */
    void ResetStats();

    /**
     * @brief 清空所有缓存
     */
    void Clear();

private:
    FilterCache filter_cache_;                ///< 滤镜缓存
    TransformMatrixCache transform_cache_;    ///< 变换矩阵缓存
};

} // namespace lightui

