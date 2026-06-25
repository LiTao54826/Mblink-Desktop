/**
 * @file test_content_version_properties.cpp
 * @brief Property-based tests for ContentVersionManager
 * 
 * This file implements property-based testing for the ContentVersionManager
 * class. Each property test runs 100 iterations with randomly generated inputs.
 * 
 * **Feature: incremental-layout-optimization, Property 1: 内容版本号递增一致性**
 * **Validates: Requirements 1.1, 1.2, 1.3**
 */

#include <gtest/gtest.h>
#include "layout/content_version.h"
#include <random>
#include <vector>
#include <thread>
#include <algorithm>

using namespace mblink;

// Random number generator for property tests
class ContentVersionPropertyTestRng {
public:
    ContentVersionPropertyTestRng() : gen_(std::random_device{}()) {}
    
    int randInt(int min, int max) {
        std::uniform_int_distribution<int> dist(min, max);
        return dist(gen_);
    }
    
private:
    std::mt19937 gen_;
};

class ContentVersionPropertyTest : public ::testing::Test {
protected:
    ContentVersionPropertyTestRng rng_;
    static constexpr int NUM_ITERATIONS = 100;
    
    void SetUp() override {
        // Reset the version manager before each test for deterministic behavior
        ContentVersionManager::GetInstance().Reset();
    }
};

/**
 * **Feature: incremental-layout-optimization, Property 1: 内容版本号递增一致性**
 * 
 * For any sequence of GenerateVersion() calls, each returned version
 * SHALL be strictly greater than all previously returned versions.
 * 
 * **Validates: Requirements 1.1, 1.2, 1.3**
 */
TEST_F(ContentVersionPropertyTest, VersionAlwaysIncreases) {
    auto& manager = ContentVersionManager::GetInstance();
    
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        // Generate a random number of version increments (1-50)
        int num_increments = rng_.randInt(1, 50);
        
        uint64_t previous_version = 0;
        
        for (int j = 0; j < num_increments; ++j) {
            uint64_t new_version = manager.GenerateVersion();
            
            EXPECT_GT(new_version, previous_version)
                << "Version should always increase. "
                << "Previous: " << previous_version << ", "
                << "New: " << new_version << ", "
                << "Iteration: " << i << ", "
                << "Increment: " << j;
            
            previous_version = new_version;
        }
    }
}

/**
 * **Feature: incremental-layout-optimization, Property 1: 内容版本号递增一致性**
 * 
 * For any GenerateVersion() call, the returned version SHALL be
 * greater than zero (versions start at 1, not 0).
 * 
 * **Validates: Requirements 1.1, 1.2, 1.3**
 */
TEST_F(ContentVersionPropertyTest, VersionAlwaysPositive) {
    auto& manager = ContentVersionManager::GetInstance();
    
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        uint64_t version = manager.GenerateVersion();
        
        EXPECT_GT(version, 0u)
            << "Version should always be positive (> 0). "
            << "Got: " << version << ", "
            << "Iteration: " << i;
    }
}

/**
 * **Feature: incremental-layout-optimization, Property 1: 内容版本号递增一致性**
 * 
 * For any sequence of N GenerateVersion() calls starting from reset state,
 * the final version SHALL equal N (since versions increment by 1 each time).
 * 
 * **Validates: Requirements 1.1, 1.2, 1.3**
 */
TEST_F(ContentVersionPropertyTest, VersionEqualsCallCount) {
    auto& manager = ContentVersionManager::GetInstance();
    
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        // Reset before each iteration
        manager.Reset();
        
        // Generate a random number of versions (1-100)
        int num_calls = rng_.randInt(1, 100);
        
        uint64_t last_version = 0;
        for (int j = 0; j < num_calls; ++j) {
            last_version = manager.GenerateVersion();
        }
        
        EXPECT_EQ(last_version, static_cast<uint64_t>(num_calls))
            << "After " << num_calls << " calls, version should be " << num_calls << ". "
            << "Got: " << last_version << ", "
            << "Iteration: " << i;
    }
}

/**
 * **Feature: incremental-layout-optimization, Property 1: 内容版本号递增一致性**
 * 
 * GetCurrentVersion() SHALL return the same value as the most recent
 * GenerateVersion() call (without incrementing).
 * 
 * **Validates: Requirements 1.1, 1.2, 1.3**
 */
TEST_F(ContentVersionPropertyTest, GetCurrentVersionMatchesLastGenerated) {
    auto& manager = ContentVersionManager::GetInstance();
    
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        // Generate a random number of versions (1-50)
        int num_calls = rng_.randInt(1, 50);
        
        uint64_t last_generated = 0;
        for (int j = 0; j < num_calls; ++j) {
            last_generated = manager.GenerateVersion();
        }
        
        // GetCurrentVersion should match the last generated version
        uint64_t current = manager.GetCurrentVersion();
        
        EXPECT_EQ(current, last_generated)
            << "GetCurrentVersion() should return the last generated version. "
            << "Last generated: " << last_generated << ", "
            << "GetCurrentVersion(): " << current << ", "
            << "Iteration: " << i;
        
        // Calling GetCurrentVersion multiple times should not change the value
        uint64_t current2 = manager.GetCurrentVersion();
        EXPECT_EQ(current2, current)
            << "GetCurrentVersion() should not change the version. "
            << "First call: " << current << ", "
            << "Second call: " << current2;
    }
}

/**
 * **Feature: incremental-layout-optimization, Property 1: 内容版本号递增一致性**
 * 
 * For any two consecutive GenerateVersion() calls, the difference
 * between versions SHALL be exactly 1.
 * 
 * **Validates: Requirements 1.1, 1.2, 1.3**
 */
TEST_F(ContentVersionPropertyTest, VersionIncrementsByOne) {
    auto& manager = ContentVersionManager::GetInstance();
    
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        uint64_t version1 = manager.GenerateVersion();
        uint64_t version2 = manager.GenerateVersion();
        
        EXPECT_EQ(version2 - version1, 1u)
            << "Consecutive versions should differ by exactly 1. "
            << "Version1: " << version1 << ", "
            << "Version2: " << version2 << ", "
            << "Difference: " << (version2 - version1) << ", "
            << "Iteration: " << i;
    }
}

/**
 * **Feature: incremental-layout-optimization, Property 1: 内容版本号递增一致性**
 * 
 * The singleton instance SHALL always return the same object reference.
 * 
 * **Validates: Requirements 1.1, 1.2, 1.3**
 */
TEST_F(ContentVersionPropertyTest, SingletonConsistency) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        auto& instance1 = ContentVersionManager::GetInstance();
        auto& instance2 = ContentVersionManager::GetInstance();
        
        EXPECT_EQ(&instance1, &instance2)
            << "GetInstance() should always return the same object. "
            << "Iteration: " << i;
    }
}

/**
 * **Feature: incremental-layout-optimization, Property 1: 内容版本号递增一致性**
 * 
 * After Reset(), the next GenerateVersion() call SHALL return 1.
 * 
 * **Validates: Requirements 1.1, 1.2, 1.3**
 */
TEST_F(ContentVersionPropertyTest, ResetResetsToZero) {
    auto& manager = ContentVersionManager::GetInstance();
    
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        // Generate some versions first
        int num_calls = rng_.randInt(1, 50);
        for (int j = 0; j < num_calls; ++j) {
            manager.GenerateVersion();
        }
        
        // Reset
        manager.Reset();
        
        // GetCurrentVersion should be 0 after reset
        EXPECT_EQ(manager.GetCurrentVersion(), 0u)
            << "GetCurrentVersion() should return 0 after Reset(). "
            << "Iteration: " << i;
        
        // Next GenerateVersion should return 1
        uint64_t first_after_reset = manager.GenerateVersion();
        EXPECT_EQ(first_after_reset, 1u)
            << "First GenerateVersion() after Reset() should return 1. "
            << "Got: " << first_after_reset << ", "
            << "Iteration: " << i;
    }
}

