/**
 * @file style_cache.h
 * @brief 样式缓存系统
 * 
 * 提供样式计算结果的缓存机制，优化重复计算性能
 */

#ifndef MBLINK_LEXBOR_STYLE_CACHE_H
#define MBLINK_LEXBOR_STYLE_CACHE_H

#include <map>
#include <string>
#include <unordered_map>
#include <memory>

namespace mblink {

// 前向声明
class Element;

/**
 * @brief 样式缓存类
 * 
 * 缓存元素的计算样式，提供失效机制和统计信息
 */
class StyleCache {
public:
    /**
     * @brief 构造函数
     */
    StyleCache();

    /**
     * @brief 析构函数
     */
    ~StyleCache();

    // ========== 缓存访问 ==========

    /**
     * @brief 获取缓存的样式
     * @param element 元素指针
     * @return 缓存的样式，如果不存在返回nullptr
     */
    const std::map<std::string, std::string>* GetCachedStyle(Element* element) const;

    /**
     * @brief 设置缓存
     * @param element 元素指针
     * @param style 样式映射
     */
    void SetCachedStyle(Element* element, const std::map<std::string, std::string>& style);

    /**
     * @brief 检查元素是否有缓存
     * @param element 元素指针
     * @return 如果有缓存返回true
     */
    bool HasCachedStyle(Element* element) const;

    // ========== 缓存失效 ==========

    /**
     * @brief 失效单个元素的缓存
     * @param element 元素指针
     */
    void InvalidateElement(Element* element);

    /**
     * @brief 失效元素及其子树的缓存
     * @param element 元素指针
     */
    void InvalidateSubtree(Element* element);

    /**
     * @brief 失效所有缓存
     */
    void InvalidateAll();

    // ========== 统计信息 ==========

    /**
     * @brief 获取缓存大小
     * @return 缓存的元素数量
     */
    size_t GetCacheSize() const;

    /**
     * @brief 获取缓存命中率
     * @return 命中率（0.0-1.0）
     */
    double GetHitRate() const;

    /**
     * @brief 获取命中次数
     * @return 命中次数
     */
    size_t GetHits() const;

    /**
     * @brief 获取未命中次数
     * @return 未命中次数
     */
    size_t GetMisses() const;

    /**
     * @brief 重置统计信息
     */
    void ResetStats();

    // ========== 缓存管理 ==========

    /**
     * @brief 清空缓存（保留统计信息）
     */
    void Clear();

    /**
     * @brief 设置最大缓存大小
     * @param max_size 最大缓存元素数量（0表示无限制）
     */
    void SetMaxCacheSize(size_t max_size);

    /**
     * @brief 获取最大缓存大小
     * @return 最大缓存元素数量（0表示无限制）
     */
    size_t GetMaxCacheSize() const;

    /**
     * @brief 记录缓存命中
     * @internal 内部使用，用于统计
     */
    void RecordHit();

    /**
     * @brief 记录缓存未命中
     * @internal 内部使用，用于统计
     */
    void RecordMiss();

private:
    /**
     * @brief 失效子树的递归辅助函数
     * @param element 元素指针
     */
    void InvalidateSubtreeRecursive(Element* element);

    /**
     * @brief 检查并执行缓存淘汰（LRU策略）
     */
    void EvictIfNeeded();

    // ========== 成员变量 ==========

    /// 样式缓存：Element* -> 计算样式
    std::unordered_map<Element*, std::map<std::string, std::string>> cache_;

    /// 缓存命中次数
    size_t hits_;

    /// 缓存未命中次数
    size_t misses_;

    /// 最大缓存大小（0表示无限制）
    size_t max_cache_size_;

    /// LRU访问顺序（用于缓存淘汰）
    std::unordered_map<Element*, size_t> access_order_;

    /// 当前访问计数器
    size_t access_counter_;
};

} // namespace mblink

#endif // MBLINK_LEXBOR_STYLE_CACHE_H

