/**
 * @file test_payload_properties.cpp
 * @brief Payload 属性测试
 *
 * **Feature: app-bundler, Property 7: Payload 检测往返**
 * **Validates: Requirements 7.1, 7.2**
 */

#include <gtest/gtest.h>
#include "tools/app_bundler/payload.h"
#include <random>
#include <string>

using namespace mbink;

class PayloadPropertyTest : public ::testing::Test {
protected:
    std::mt19937 rng_{42};  // 固定种子以便复现

    // 生成随机字符串
    std::string RandomString(size_t min_len = 1, size_t max_len = 100) {
        std::uniform_int_distribution<size_t> len_dist(min_len, max_len);
        std::uniform_int_distribution<int> char_dist(32, 126);  // 可打印 ASCII
        
        size_t len = len_dist(rng_);
        std::string result;
        result.reserve(len);
        for (size_t i = 0; i < len; i++) {
            result += static_cast<char>(char_dist(rng_));
        }
        return result;
    }

    // 生成随机字节码
    std::vector<uint8_t> RandomBytecode(size_t min_len = 0, size_t max_len = 1000) {
        std::uniform_int_distribution<size_t> len_dist(min_len, max_len);
        std::uniform_int_distribution<int> byte_dist(0, 255);
        
        size_t len = len_dist(rng_);
        std::vector<uint8_t> result(len);
        for (size_t i = 0; i < len; i++) {
            result[i] = static_cast<uint8_t>(byte_dist(rng_));
        }
        return result;
    }

    // 生成随机配置
    PayloadConfig RandomConfig() {
        std::uniform_int_distribution<int> size_dist(100, 2000);
        std::uniform_int_distribution<uint32_t> count_dist(0, 100);
        
        PayloadConfig config;
        config.width = size_dist(rng_);
        config.height = size_dist(rng_);
        config.title = RandomString(1, 50);
        config.module_count = count_dist(rng_);
        return config;
    }
};


/**
 * Property 7: Payload 往返测试
 * 对于任意配置和字节码，Build() 后 Parse() 应该恢复原始数据
 */
TEST_F(PayloadPropertyTest, PayloadRoundTrip) {
    const int NUM_ITERATIONS = 100;
    
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        // 生成随机配置和字节码
        PayloadConfig original_config = RandomConfig();
        std::vector<uint8_t> original_bytecode = RandomBytecode();
        
        // 构建 payload
        PayloadBuilder builder;
        builder.SetConfig(original_config);
        builder.SetBytecode(original_bytecode);
        std::vector<uint8_t> payload = builder.Build();
        
        // 验证 payload 有效
        ASSERT_TRUE(PayloadBuilder::HasPayload(payload)) 
            << "Iteration " << i << ": HasPayload should return true";
        
        // 解析 payload
        PayloadData parsed;
        ASSERT_TRUE(PayloadBuilder::Parse(payload, parsed))
            << "Iteration " << i << ": Parse should succeed";
        
        // 验证配置恢复
        EXPECT_EQ(parsed.config.version, original_config.version)
            << "Iteration " << i << ": version mismatch";
        EXPECT_EQ(parsed.config.width, original_config.width)
            << "Iteration " << i << ": width mismatch";
        EXPECT_EQ(parsed.config.height, original_config.height)
            << "Iteration " << i << ": height mismatch";
        EXPECT_EQ(parsed.config.title, original_config.title)
            << "Iteration " << i << ": title mismatch";
        EXPECT_EQ(parsed.config.module_count, original_config.module_count)
            << "Iteration " << i << ": module_count mismatch";
        
        // 验证字节码恢复
        EXPECT_EQ(parsed.bytecode, original_bytecode)
            << "Iteration " << i << ": bytecode mismatch";
        
        EXPECT_TRUE(parsed.valid)
            << "Iteration " << i << ": parsed.valid should be true";
    }
}

/**
 * Property 7: 空字节码往返测试
 */
TEST_F(PayloadPropertyTest, EmptyBytecodeRoundTrip) {
    const int NUM_ITERATIONS = 50;
    
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        PayloadConfig config = RandomConfig();
        std::vector<uint8_t> empty_bytecode;
        
        PayloadBuilder builder;
        builder.SetConfig(config);
        builder.SetBytecode(empty_bytecode);
        std::vector<uint8_t> payload = builder.Build();
        
        PayloadData parsed;
        ASSERT_TRUE(PayloadBuilder::Parse(payload, parsed));
        EXPECT_TRUE(parsed.bytecode.empty());
        EXPECT_EQ(parsed.config.title, config.title);
    }
}

/**
 * Property 7: 带前缀数据的往返测试（模拟 exe + payload）
 */
TEST_F(PayloadPropertyTest, PayloadWithPrefixRoundTrip) {
    const int NUM_ITERATIONS = 50;
    
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        // 生成随机 "exe" 前缀
        std::vector<uint8_t> prefix = RandomBytecode(100, 10000);
        
        // 生成 payload
        PayloadConfig config = RandomConfig();
        std::vector<uint8_t> bytecode = RandomBytecode(10, 500);
        
        PayloadBuilder builder;
        builder.SetConfig(config);
        builder.SetBytecode(bytecode);
        std::vector<uint8_t> payload = builder.Build();
        
        // 合并：prefix + payload
        std::vector<uint8_t> combined = prefix;
        combined.insert(combined.end(), payload.begin(), payload.end());
        
        // 解析应该成功
        PayloadData parsed;
        ASSERT_TRUE(PayloadBuilder::Parse(combined, parsed))
            << "Iteration " << i << ": Parse with prefix should succeed";
        
        EXPECT_EQ(parsed.bytecode, bytecode);
        EXPECT_EQ(parsed.config.title, config.title);
    }
}


/**
 * Property 6: 校验和完整性测试
 * 任意修改 payload 数据应导致校验失败
 * **Feature: app-bundler, Property 6: 校验和完整性**
 * **Validates: Requirements 8.1, 8.2, 8.3**
 */
TEST_F(PayloadPropertyTest, ChecksumIntegrity) {
    const int NUM_ITERATIONS = 50;
    std::uniform_int_distribution<size_t> pos_dist;
    std::uniform_int_distribution<int> byte_dist(1, 255);  // 非零以确保修改
    
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        // 构建有效 payload
        PayloadConfig config = RandomConfig();
        std::vector<uint8_t> bytecode = RandomBytecode(10, 500);
        
        PayloadBuilder builder;
        builder.SetConfig(config);
        builder.SetBytecode(bytecode);
        std::vector<uint8_t> payload = builder.Build();
        
        // 验证原始 payload 有效
        PayloadData parsed;
        ASSERT_TRUE(PayloadBuilder::Parse(payload, parsed));
        
        // 修改 payload 中的一个字节（不包括最后 4 字节的 magic）
        if (payload.size() > FOOTER_SIZE) {
            std::vector<uint8_t> corrupted = payload;
            pos_dist = std::uniform_int_distribution<size_t>(0, payload.size() - FOOTER_SIZE - 1);
            size_t pos = pos_dist(rng_);
            corrupted[pos] ^= static_cast<uint8_t>(byte_dist(rng_));  // XOR 修改
            
            // 解析应该失败（校验和不匹配）
            PayloadData corrupted_parsed;
            EXPECT_FALSE(PayloadBuilder::Parse(corrupted, corrupted_parsed))
                << "Iteration " << i << ": Parse should fail for corrupted payload at pos " << pos;
        }
    }
}

/**
 * Property 6: Magic 损坏测试
 */
TEST_F(PayloadPropertyTest, CorruptedMagicDetection) {
    PayloadConfig config;
    config.title = "Test App";
    std::vector<uint8_t> bytecode = {1, 2, 3, 4, 5};
    
    PayloadBuilder builder;
    builder.SetConfig(config);
    builder.SetBytecode(bytecode);
    std::vector<uint8_t> payload = builder.Build();
    
    // 损坏 magic（最后 4 字节）
    payload[payload.size() - 1] ^= 0xFF;
    
    EXPECT_FALSE(PayloadBuilder::HasPayload(payload));
    
    PayloadData parsed;
    EXPECT_FALSE(PayloadBuilder::Parse(payload, parsed));
}

/**
 * 边界测试：最小有效 payload
 */
TEST_F(PayloadPropertyTest, MinimalPayload) {
    PayloadConfig config;
    config.title = "";
    config.width = 0;
    config.height = 0;
    config.module_count = 0;
    
    PayloadBuilder builder;
    builder.SetConfig(config);
    builder.SetBytecode({});
    
    std::vector<uint8_t> payload = builder.Build();
    
    PayloadData parsed;
    ASSERT_TRUE(PayloadBuilder::Parse(payload, parsed));
    EXPECT_TRUE(parsed.bytecode.empty());
    EXPECT_TRUE(parsed.config.title.empty());
}

/**
 * 边界测试：大字节码
 */
TEST_F(PayloadPropertyTest, LargeBytecode) {
    PayloadConfig config;
    config.title = "Large App";
    
    // 1MB 字节码
    std::vector<uint8_t> large_bytecode(1024 * 1024);
    for (size_t i = 0; i < large_bytecode.size(); i++) {
        large_bytecode[i] = static_cast<uint8_t>(i & 0xFF);
    }
    
    PayloadBuilder builder;
    builder.SetConfig(config);
    builder.SetBytecode(large_bytecode);
    
    std::vector<uint8_t> payload = builder.Build();
    
    PayloadData parsed;
    ASSERT_TRUE(PayloadBuilder::Parse(payload, parsed));
    EXPECT_EQ(parsed.bytecode.size(), large_bytecode.size());
    EXPECT_EQ(parsed.bytecode, large_bytecode);
}
