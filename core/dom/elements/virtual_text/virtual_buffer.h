/**
 * @file virtual_buffer.h
 * @brief 泛型环形缓冲区模板
 *
 * 提供固定容量的环形缓冲区，支持 O(1) 追加和随机访问。
 * 当缓冲区满时自动丢弃最旧的条目。
 */

#pragma once

#include <cstddef>
#include <deque>
#include <stdexcept>

namespace mbink {

/**
 * @brief 泛型环形缓冲区
 *
 * VirtualBuffer 是一个固定容量的环形缓冲区，用于存储大量数据。
 * 当缓冲区达到最大容量时，自动丢弃最旧的条目。
 *
 * @tparam T 存储的元素类型
 *
 * 特性：
 * - O(1) 追加操作
 * - O(1) 随机访问
 * - 固定内存上限
 * - 自动淘汰旧数据
 *
 * @note 线程不安全，需要外部同步
 */
template <typename T>
class VirtualBuffer {
public:
    /**
     * @brief 构造函数
     * @param max_capacity 最大容量，超过时自动丢弃旧数据
     */
    explicit VirtualBuffer(size_t max_capacity)
        : max_capacity_(max_capacity) {
        if (max_capacity == 0) {
            throw std::invalid_argument("max_capacity must be greater than 0");
        }
    }

    /**
     * @brief 追加元素（移动语义）
     * @param item 要追加的元素
     */
    void Append(T&& item) {
        data_.push_back(std::move(item));
        TrimToCapacity();
    }

    /**
     * @brief 追加元素（拷贝语义）
     * @param item 要追加的元素
     */
    void Append(const T& item) {
        data_.push_back(item);
        TrimToCapacity();
    }

    /**
     * @brief 原地构造元素
     * @param args 构造函数参数
     */
    template <typename... Args>
    void Emplace(Args&&... args) {
        data_.emplace_back(std::forward<Args>(args)...);
        TrimToCapacity();
    }

    /**
     * @brief 清空缓冲区
     */
    void Clear() {
        data_.clear();
    }

    /**
     * @brief 随机访问元素
     * @param index 索引（0 为最旧的元素）
     * @return 元素的常量引用
     * @throws std::out_of_range 索引越界时抛出
     */
    const T& operator[](size_t index) const {
        if (index >= data_.size()) {
            throw std::out_of_range("VirtualBuffer index out of range");
        }
        return data_[index];
    }

    /**
     * @brief 随机访问元素（可修改）
     * @param index 索引
     * @return 元素的引用
     */
    T& operator[](size_t index) {
        if (index >= data_.size()) {
            throw std::out_of_range("VirtualBuffer index out of range");
        }
        return data_[index];
    }

    /**
     * @brief 安全访问元素
     * @param index 索引
     * @return 元素的常量引用
     * @throws std::out_of_range 索引越界时抛出
     */
    const T& At(size_t index) const {
        return data_.at(index);
    }

    /**
     * @brief 获取第一个元素（最旧）
     * @return 第一个元素的常量引用
     * @throws std::out_of_range 缓冲区为空时抛出
     */
    const T& Front() const {
        if (data_.empty()) {
            throw std::out_of_range("VirtualBuffer is empty");
        }
        return data_.front();
    }

    /**
     * @brief 获取最后一个元素（最新）
     * @return 最后一个元素的常量引用
     * @throws std::out_of_range 缓冲区为空时抛出
     */
    const T& Back() const {
        if (data_.empty()) {
            throw std::out_of_range("VirtualBuffer is empty");
        }
        return data_.back();
    }

    /**
     * @brief 获取当前元素数量
     * @return 元素数量
     */
    size_t size() const { return data_.size(); }

    /**
     * @brief 获取最大容量
     * @return 最大容量
     */
    size_t capacity() const { return max_capacity_; }

    /**
     * @brief 检查缓冲区是否为空
     * @return 为空返回 true
     */
    bool empty() const { return data_.empty(); }

    /**
     * @brief 检查缓冲区是否已满
     * @return 已满返回 true
     */
    bool full() const { return data_.size() >= max_capacity_; }

    // 迭代器支持
    using iterator = typename std::deque<T>::iterator;
    using const_iterator = typename std::deque<T>::const_iterator;

    iterator begin() { return data_.begin(); }
    iterator end() { return data_.end(); }
    const_iterator begin() const { return data_.begin(); }
    const_iterator end() const { return data_.end(); }
    const_iterator cbegin() const { return data_.cbegin(); }
    const_iterator cend() const { return data_.cend(); }

private:
    std::deque<T> data_;
    size_t max_capacity_;

    /**
     * @brief 裁剪到最大容量
     *
     * 当元素数量超过最大容量时，从前端移除最旧的元素。
     */
    void TrimToCapacity() {
        while (data_.size() > max_capacity_) {
            data_.pop_front();
        }
    }
};

}  // namespace mbink
