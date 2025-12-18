/**
 * @file content_version.cpp
 * @brief Implementation of ContentVersionManager
 * 
 * **Feature: incremental-layout-optimization**
 * **Validates: Requirements 1.1, 1.2, 1.3**
 */

#include "layout/content_version.h"

namespace lightui {

ContentVersionManager::ContentVersionManager() = default;

ContentVersionManager& ContentVersionManager::GetInstance() {
    static ContentVersionManager instance;
    return instance;
}

uint64_t ContentVersionManager::GenerateVersion() {
    // Pre-increment to ensure version is always > 0
    return ++version_counter_;
}

uint64_t ContentVersionManager::GetCurrentVersion() const {
    return version_counter_.load(std::memory_order_acquire);
}

void ContentVersionManager::Reset() {
    version_counter_.store(0, std::memory_order_release);
}

} // namespace lightui
