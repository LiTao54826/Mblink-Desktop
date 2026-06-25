/**
 * @file render_cache.h
 * @brief 渲染缓存系统
 *
 * 功能：
 * - 缓存静态层级
 * - 避免重复渲染
 * - 实现缓存失效机制
 * - 管理缓存内存
 */

#pragma once

#include "include/core/SkSurface.h"
#include "include/core/SkImage.h"
#include "include/core/SkRect.h"
#include "include/core/SkPaint.h"
#include <memory>
#include <unordered_map>
#include <string>
#include <chrono>

namespace mblink {

/**
 * @brief 缓存条目
 */
struct CacheEntry {
    sk_sp<SkImage> image;                                    // 缓存的图像
    SkRect bounds;                                           // 边界
    std::chrono::steady_clock::time_point last_access_time;  // 最后访问时间
    size_t memory_size;                                      // 内存大小（字节）
    bool is_dirty;                                           // 是否脏（需要更新）

    CacheEntry()
        : bounds(SkRect::MakeEmpty())
        , last_access_time(std::chrono::steady_clock::now())
        , memory_size(0)
        , is_dirty(false) {
    }
};

/**
 * @brief 渲染缓存
 *
 * 缓存渲染结果，避免重复渲染
 */
class RenderCache {
public:
    /**
     * @brief 构造函数
     * @param max_memory_bytes 最大内存限制（字节）
     */
    explicit RenderCache(size_t max_memory_bytes = 64 * 1024 * 1024); // 默认 64MB

    /**
     * @brief 析构函数
     */
    ~RenderCache() = default;

    // ========== 缓存操作 ==========

    /**
     * @brief 添加缓存条目
     * @param key 缓存键
     * @param image 图像
     * @param bounds 边界
     */
    void Put(const std::string& key, sk_sp<SkImage> image, const SkRect& bounds);

    /**
     * @brief 获取缓存条目
     * @param key 缓存键
     * @return 缓存条目指针，未找到返回 nullptr
     */
    CacheEntry* Get(const std::string& key);

    /**
     * @brief 检查缓存是否存在
     * @param key 缓存键
     * @return 是否存在
     */
    bool Has(const std::string& key) const;

    /**
     * @brief 移除缓存条目
     * @param key 缓存键
     */
    void Remove(const std::string& key);

    /**
     * @brief 清空所有缓存
     */
    void Clear();

    // ========== 缓存失效 ==========

    /**
     * @brief 标记缓存为脏
     * @param key 缓存键
     */
    void MarkDirty(const std::string& key);

    /**
     * @brief 标记所有缓存为脏
     */
    void MarkAllDirty();

    /**
     * @brief 清理脏缓存
     */
    void CleanDirtyEntries();

    // ========== 内存管理 ==========

    /**
     * @brief 获取当前内存使用量（字节）
     */
    size_t GetMemoryUsage() const { return current_memory_usage_; }

    /**
     * @brief 获取最大内存限制（字节）
     */
    size_t GetMaxMemory() const { return max_memory_bytes_; }

    /**
     * @brief 设置最大内存限制（字节）
     */
    void SetMaxMemory(size_t max_memory_bytes) { max_memory_bytes_ = max_memory_bytes; }

    /**
     * @brief 清理过期缓存（LRU 策略）
     */
    void EvictLRU();

    /**
     * @brief 清理超过内存限制的缓存
     */
    void EvictIfNeeded();

    // ========== 统计信息 ==========

    /**
     * @brief 获取缓存条目数量
     */
    size_t GetEntryCount() const { return cache_.size(); }

    /**
     * @brief 获取命中次数
     */
    size_t GetHitCount() const { return hit_count_; }

    /**
     * @brief 获取未命中次数
     */
    size_t GetMissCount() const { return miss_count_; }

    /**
     * @brief 获取命中率
     */
    float GetHitRate() const {
        size_t total = hit_count_ + miss_count_;
        return total > 0 ? static_cast<float>(hit_count_) / total : 0.0f;
    }

    /**
     * @brief 重置统计信息
     */
    void ResetStats() {
        hit_count_ = 0;
        miss_count_ = 0;
    }

private:
    /**
     * @brief 计算图像内存大小
     * @param image 图像
     * @return 内存大小（字节）
     */
    size_t CalculateImageSize(sk_sp<SkImage> image) const;

    std::unordered_map<std::string, CacheEntry> cache_; // 缓存映射
    size_t max_memory_bytes_;                           // 最大内存限制
    size_t current_memory_usage_;                       // 当前内存使用量
    size_t hit_count_;                                  // 命中次数
    size_t miss_count_;                                 // 未命中次数
};

/**
 * @brief 批量渲染命令
 */
enum class RenderCommandType {
    DRAW_RECT,
    FILL_RECT,
    DRAW_CIRCLE,
    FILL_CIRCLE,
    DRAW_LINE,
    DRAW_TEXT,
    DRAW_IMAGE,
};

/**
 * @brief 渲染命令
 */
struct RenderCommand {
    RenderCommandType type;
    float x, y, width, height;
    float radius;
    std::string text;
    sk_sp<SkImage> image;
    SkPaint paint;

    RenderCommand(RenderCommandType t) : type(t), x(0), y(0), width(0), height(0), radius(0) {}
};

/**
 * @brief 批量渲染器
 *
 * 合并多个绘制操作，减少 GPU 调用
 */
class BatchRenderer {
public:
    /**
     * @brief 构造函数
     */
    BatchRenderer() = default;

    /**
     * @brief 析构函数
     */
    ~BatchRenderer() = default;

    // ========== 命令记录 ==========

    /**
     * @brief 添加绘制矩形命令
     */
    void AddDrawRect(float x, float y, float width, float height, const SkPaint& paint);

    /**
     * @brief 添加填充矩形命令
     */
    void AddFillRect(float x, float y, float width, float height, const SkPaint& paint);

    /**
     * @brief 添加绘制圆形命令
     */
    void AddDrawCircle(float x, float y, float radius, const SkPaint& paint);

    /**
     * @brief 添加填充圆形命令
     */
    void AddFillCircle(float x, float y, float radius, const SkPaint& paint);

    /**
     * @brief 添加绘制线条命令
     */
    void AddDrawLine(float x1, float y1, float x2, float y2, const SkPaint& paint);

    /**
     * @brief 添加绘制文本命令
     */
    void AddDrawText(const std::string& text, float x, float y, const SkPaint& paint);

    /**
     * @brief 添加绘制图片命令
     */
    void AddDrawImage(sk_sp<SkImage> image, float x, float y);

    // ========== 批量执行 ==========

    /**
     * @brief 执行所有命令
     * @param canvas 画布
     */
    void Execute(SkCanvas* canvas);

    /**
     * @brief 清空所有命令
     */
    void Clear();

    /**
     * @brief 获取命令数量
     */
    size_t GetCommandCount() const { return commands_.size(); }

    // ========== 优化 ==========

    /**
     * @brief 优化命令列表
     *
     * 合并相邻的相同类型命令
     */
    void Optimize();

private:
    std::vector<RenderCommand> commands_; // 命令列表
};

} // namespace mblink

