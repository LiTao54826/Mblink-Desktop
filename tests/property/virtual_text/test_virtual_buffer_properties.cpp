/**
 * @file test_virtual_buffer_properties.cpp
 * @brief VirtualBuffer 属性测试
 *
 * 测试 VirtualBuffer 的正确性属性：
 * - 属性 1: 缓冲区溢出处理
 * - 属性 2: 清除重置状态
 */

#include "core/dom/elements/virtual_text/virtual_buffer.h"

#include <gtest/gtest.h>
#include <random>
#include <string>
#include <vector>

namespace lightui {
namespace {

// 随机数生成器
class RandomGenerator {
public:
    RandomGenerator() : gen_(std::random_device{}()) {}

    int Int(int min, int max) {
        return std::uniform_int_distribution<int>(min, max)(gen_);
    }

    size_t Size(size_t min, size_t max) {
        return std::uniform_int_distribution<size_t>(min, max)(gen_);
    }

    std::string String(size_t length) {
        static const char chars[] = "abcdefghijklmnopqrstuvwxyz0123456789";
        std::string result;
        result.reserve(length);
        for (size_t i = 0; i < length; ++i) {
            result += chars[Int(0, sizeof(chars) - 2)];
        }
        return result;
    }

    std::vector<int> IntVector(size_t size, int min, int max) {
        std::vector<int> result;
        result.reserve(size);
        for (size_t i = 0; i < size; ++i) {
            result.push_back(Int(min, max));
        }
        return result;
    }

private:
    std::mt19937 gen_;
};

/**
 * **Feature: virtual-text-components, Property 1: 缓冲区溢出处理**
 * **Validates: Requirements 1.1, 3.3, 9.3**
 *
 * *对于任何*写入序列使缓冲区超过最大容量，缓冲区 SHALL 恰好包含 max_capacity 条目，
 * 且这些条目 SHALL 是最近写入的。
 */
class VirtualBufferOverflowPropertyTest : public ::testing::Test {
protected:
    RandomGenerator rng_;
    static constexpr int kIterations = 100;
};

TEST_F(VirtualBufferOverflowPropertyTest, OverflowKeepsNewestItems) {
    for (int iter = 0; iter < kIterations; ++iter) {
        // 生成随机容量 (10-100)
        size_t capacity = rng_.Size(10, 100);
        
        // 生成随机数量的元素 (可能超过容量)
        size_t num_items = rng_.Size(1, capacity * 3);
        auto items = rng_.IntVector(num_items, 0, 1000);
        
        // 创建缓冲区并追加所有元素
        VirtualBuffer<int> buffer(capacity);
        for (int item : items) {
            buffer.Append(item);
        }
        
        // 验证：缓冲区大小不超过容量
        ASSERT_LE(buffer.size(), capacity)
            << "Buffer size exceeds capacity";
        
        // 验证：如果写入超过容量，缓冲区应该恰好等于容量
        if (num_items > capacity) {
            ASSERT_EQ(buffer.size(), capacity)
                << "Buffer should be at capacity when overflow occurs";
        }
        
        // 验证：保留的是最新的元素
        size_t expected_start = (num_items > capacity) ? (num_items - capacity) : 0;
        for (size_t i = 0; i < buffer.size(); ++i) {
            ASSERT_EQ(buffer[i], items[expected_start + i])
                << "Buffer should contain newest items at index " << i;
        }
    }
}

TEST_F(VirtualBufferOverflowPropertyTest, OverflowWithStrings) {
    for (int iter = 0; iter < kIterations; ++iter) {
        size_t capacity = rng_.Size(5, 50);
        size_t num_items = rng_.Size(1, capacity * 2);
        
        // 生成随机字符串
        std::vector<std::string> items;
        items.reserve(num_items);
        for (size_t i = 0; i < num_items; ++i) {
            items.push_back(rng_.String(rng_.Size(1, 20)));
        }
        
        VirtualBuffer<std::string> buffer(capacity);
        for (const auto& item : items) {
            buffer.Append(item);
        }
        
        // 验证大小约束
        ASSERT_LE(buffer.size(), capacity);
        
        // 验证内容正确性
        size_t expected_start = (num_items > capacity) ? (num_items - capacity) : 0;
        for (size_t i = 0; i < buffer.size(); ++i) {
            ASSERT_EQ(buffer[i], items[expected_start + i]);
        }
    }
}

TEST_F(VirtualBufferOverflowPropertyTest, SingleItemCapacity) {
    // 边界情况：容量为 1
    for (int iter = 0; iter < kIterations; ++iter) {
        VirtualBuffer<int> buffer(1);
        
        size_t num_items = rng_.Size(1, 100);
        int last_item = 0;
        for (size_t i = 0; i < num_items; ++i) {
            last_item = rng_.Int(0, 1000);
            buffer.Append(last_item);
        }
        
        ASSERT_EQ(buffer.size(), 1u);
        ASSERT_EQ(buffer[0], last_item);
    }
}

/**
 * **Feature: virtual-text-components, Property 2: 清除重置状态**
 * **Validates: Requirements 1.4, 6.2**
 *
 * *对于任何*缓冲区状态，调用 Clear() SHALL 导致空缓冲区，size() 返回 0。
 */
class VirtualBufferClearPropertyTest : public ::testing::Test {
protected:
    RandomGenerator rng_;
    static constexpr int kIterations = 100;
};

TEST_F(VirtualBufferClearPropertyTest, ClearResetsToEmpty) {
    for (int iter = 0; iter < kIterations; ++iter) {
        size_t capacity = rng_.Size(10, 100);
        size_t num_items = rng_.Size(0, capacity * 2);
        
        VirtualBuffer<int> buffer(capacity);
        
        // 添加随机数量的元素
        for (size_t i = 0; i < num_items; ++i) {
            buffer.Append(rng_.Int(0, 1000));
        }
        
        // 清除
        buffer.Clear();
        
        // 验证：缓冲区为空
        ASSERT_EQ(buffer.size(), 0u)
            << "Buffer should be empty after Clear()";
        ASSERT_TRUE(buffer.empty())
            << "Buffer.empty() should return true after Clear()";
    }
}

TEST_F(VirtualBufferClearPropertyTest, ClearThenAppendWorks) {
    for (int iter = 0; iter < kIterations; ++iter) {
        size_t capacity = rng_.Size(10, 100);
        
        VirtualBuffer<int> buffer(capacity);
        
        // 第一轮：添加元素
        size_t first_round = rng_.Size(1, capacity);
        for (size_t i = 0; i < first_round; ++i) {
            buffer.Append(rng_.Int(0, 1000));
        }
        
        // 清除
        buffer.Clear();
        
        // 第二轮：添加新元素
        size_t second_round = rng_.Size(1, capacity * 2);
        auto new_items = rng_.IntVector(second_round, 0, 1000);
        for (int item : new_items) {
            buffer.Append(item);
        }
        
        // 验证：只包含第二轮的元素
        size_t expected_size = std::min(second_round, capacity);
        ASSERT_EQ(buffer.size(), expected_size);
        
        size_t expected_start = (second_round > capacity) ? (second_round - capacity) : 0;
        for (size_t i = 0; i < buffer.size(); ++i) {
            ASSERT_EQ(buffer[i], new_items[expected_start + i]);
        }
    }
}

TEST_F(VirtualBufferClearPropertyTest, MultipleClearsAreIdempotent) {
    for (int iter = 0; iter < kIterations; ++iter) {
        size_t capacity = rng_.Size(10, 100);
        VirtualBuffer<int> buffer(capacity);
        
        // 添加一些元素
        for (size_t i = 0; i < rng_.Size(1, capacity); ++i) {
            buffer.Append(rng_.Int(0, 1000));
        }
        
        // 多次清除
        int clear_count = rng_.Int(1, 5);
        for (int i = 0; i < clear_count; ++i) {
            buffer.Clear();
            ASSERT_EQ(buffer.size(), 0u);
            ASSERT_TRUE(buffer.empty());
        }
    }
}

}  // namespace
}  // namespace lightui
