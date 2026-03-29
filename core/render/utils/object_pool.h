/**
 * @file object_pool.h
 * @brief 对象池实现
 *
 * 功能：
 * - 对象复用，减少内存分配
 * - 支持泛型对象池
 * - 自动扩容
 * - 线程安全（可选）
 */

#pragma once

#include <vector>
#include <memory>
#include <functional>
#include <mutex>

namespace mbink {

/**
 * @brief 泛型对象池
 * 
 * 复用对象，减少内存分配和释放开销
 * 
 * @tparam T 对象类型
 */
template<typename T>
class ObjectPool {
public:
    /**
     * @brief 构造函数
     * @param initial_size 初始对象数量
     * @param max_size 最大对象数量（0 表示无限制）
     * @param thread_safe 是否线程安全
     */
    explicit ObjectPool(size_t initial_size = 10, 
                       size_t max_size = 1000,
                       bool thread_safe = false)
        : max_size_(max_size)
        , thread_safe_(thread_safe)
        , allocated_count_(0)
        , reused_count_(0) {
        
        // 预分配对象
        for (size_t i = 0; i < initial_size; ++i) {
            available_.push_back(std::make_unique<T>());
        }
    }

    /**
     * @brief 析构函数
     */
    ~ObjectPool() = default;

    /**
     * @brief 获取对象
     * @return 对象指针
     */
    T* Acquire() {
        if (thread_safe_) {
            std::lock_guard<std::mutex> lock(mutex_);
            return AcquireInternal();
        } else {
            return AcquireInternal();
        }
    }

    /**
     * @brief 归还对象
     * @param obj 对象指针
     */
    void Release(T* obj) {
        if (!obj) return;

        if (thread_safe_) {
            std::lock_guard<std::mutex> lock(mutex_);
            ReleaseInternal(obj);
        } else {
            ReleaseInternal(obj);
        }
    }

    /**
     * @brief 清空对象池
     */
    void Clear() {
        if (thread_safe_) {
            std::lock_guard<std::mutex> lock(mutex_);
        }
        
        available_.clear();
        in_use_.clear();
    }

    /**
     * @brief 获取可用对象数量
     */
    size_t GetAvailableCount() const {
        if (thread_safe_) {
            std::lock_guard<std::mutex> lock(mutex_);
        }
        return available_.size();
    }

    /**
     * @brief 获取使用中对象数量
     */
    size_t GetInUseCount() const {
        if (thread_safe_) {
            std::lock_guard<std::mutex> lock(mutex_);
        }
        return in_use_.size();
    }

    /**
     * @brief 获取总对象数量
     */
    size_t GetTotalCount() const {
        return GetAvailableCount() + GetInUseCount();
    }

    /**
     * @brief 获取分配次数
     */
    size_t GetAllocatedCount() const { return allocated_count_; }

    /**
     * @brief 获取复用次数
     */
    size_t GetReusedCount() const { return reused_count_; }

    /**
     * @brief 获取复用率
     */
    float GetReuseRate() const {
        size_t total = allocated_count_ + reused_count_;
        return total > 0 ? static_cast<float>(reused_count_) / total : 0.0f;
    }

    /**
     * @brief 重置统计信息
     */
    void ResetStats() {
        allocated_count_ = 0;
        reused_count_ = 0;
    }

private:
    /**
     * @brief 内部获取对象（无锁）
     */
    T* AcquireInternal() {
        T* obj = nullptr;

        if (!available_.empty()) {
            // 从可用列表中获取
            auto ptr = std::move(available_.back());
            available_.pop_back();
            obj = ptr.get();
            in_use_[obj] = std::move(ptr);
            reused_count_++;
        } else {
            // 检查是否超过最大限制
            if (max_size_ > 0 && GetTotalCount() >= max_size_) {
                return nullptr;
            }

            // 创建新对象
            auto ptr = std::make_unique<T>();
            obj = ptr.get();
            in_use_[obj] = std::move(ptr);
            allocated_count_++;
        }

        return obj;
    }

    /**
     * @brief 内部归还对象（无锁）
     */
    void ReleaseInternal(T* obj) {
        auto it = in_use_.find(obj);
        if (it != in_use_.end()) {
            // 重置对象状态（如果需要）
            ResetObject(obj);
            
            // 移回可用列表
            available_.push_back(std::move(it->second));
            in_use_.erase(it);
        }
    }

    /**
     * @brief 重置对象状态
     * 
     * 可以通过特化此函数来自定义重置逻辑
     */
    void ResetObject(T* obj) {
        // 默认不做任何操作
        // 派生类可以重写此方法
        (void)obj;
    }

    std::vector<std::unique_ptr<T>> available_;           ///< 可用对象列表
    std::unordered_map<T*, std::unique_ptr<T>> in_use_;  ///< 使用中对象映射
    size_t max_size_;                                     ///< 最大对象数量
    bool thread_safe_;                                    ///< 是否线程安全
    mutable std::mutex mutex_;                            ///< 互斥锁
    size_t allocated_count_;                              ///< 分配次数
    size_t reused_count_;                                 ///< 复用次数
};

/**
 * @brief RAII 对象池包装器
 * 
 * 自动管理对象的获取和归还
 * 
 * @tparam T 对象类型
 */
template<typename T>
class PooledObject {
public:
    /**
     * @brief 构造函数
     * @param pool 对象池
     */
    explicit PooledObject(ObjectPool<T>& pool)
        : pool_(&pool)
        , object_(pool.Acquire()) {
    }

    /**
     * @brief 析构函数（自动归还对象）
     */
    ~PooledObject() {
        if (object_ && pool_) {
            pool_->Release(object_);
        }
    }

    // 禁止拷贝
    PooledObject(const PooledObject&) = delete;
    PooledObject& operator=(const PooledObject&) = delete;

    // 允许移动
    PooledObject(PooledObject&& other) noexcept
        : pool_(other.pool_)
        , object_(other.object_) {
        other.pool_ = nullptr;
        other.object_ = nullptr;
    }

    PooledObject& operator=(PooledObject&& other) noexcept {
        if (this != &other) {
            if (object_ && pool_) {
                pool_->Release(object_);
            }
            pool_ = other.pool_;
            object_ = other.object_;
            other.pool_ = nullptr;
            other.object_ = nullptr;
        }
        return *this;
    }

    /**
     * @brief 获取对象指针
     */
    T* Get() { return object_; }

    /**
     * @brief 获取对象指针（const）
     */
    const T* Get() const { return object_; }

    /**
     * @brief 解引用操作符
     */
    T& operator*() { return *object_; }

    /**
     * @brief 解引用操作符（const）
     */
    const T& operator*() const { return *object_; }

    /**
     * @brief 成员访问操作符
     */
    T* operator->() { return object_; }

    /**
     * @brief 成员访问操作符（const）
     */
    const T* operator->() const { return object_; }

    /**
     * @brief 检查是否有效
     */
    explicit operator bool() const { return object_ != nullptr; }

private:
    ObjectPool<T>* pool_;  ///< 对象池指针
    T* object_;            ///< 对象指针
};

} // namespace mbink

