/**
 * @file content_version.h
 * @brief Content version manager for incremental layout optimization
 * 
 * This file implements a global content version manager that generates
 * unique version numbers for tracking content changes in the layout tree.
 * When content changes (text, children, or layout-affecting styles),
 * nodes get new version numbers, allowing the cache to automatically
 * invalidate stale entries.
 * 
 * **Feature: incremental-layout-optimization**
 * **Validates: Requirements 1.1, 1.2, 1.3**
 */

#ifndef LIGHTUI_CONTENT_VERSION_H
#define LIGHTUI_CONTENT_VERSION_H

#include <atomic>
#include <cstdint>

namespace lightui {

/**
 * @brief Content version manager for tracking layout-affecting changes
 * 
 * Uses a global atomic counter to generate unique version numbers.
 * Each content change (text, children, layout-affecting style) triggers
 * a new version number, ensuring cache entries can detect staleness.
 * 
 * Thread-safe: Uses atomic operations for version generation.
 * 
 * Usage:
 * @code
 * auto& manager = ContentVersionManager::GetInstance();
 * uint64_t new_version = manager.GenerateVersion();
 * @endcode
 */
class ContentVersionManager {
public:
    /**
     * @brief Get the singleton instance
     * @return Reference to the global ContentVersionManager
     */
    static ContentVersionManager& GetInstance();

    /**
     * @brief Generate a new unique version number
     * 
     * Atomically increments the internal counter and returns the new value.
     * Each call is guaranteed to return a strictly greater value than
     * any previous call.
     * 
     * @return New unique version number (always > 0)
     */
    uint64_t GenerateVersion();

    /**
     * @brief Get the current version number without incrementing
     * 
     * Returns the most recently generated version number.
     * Useful for debugging or checking the current state.
     * 
     * @return Current version number (0 if no versions generated yet)
     */
    uint64_t GetCurrentVersion() const;

    /**
     * @brief Reset the version counter (for testing only)
     * 
     * Resets the counter to 0. Should only be used in tests
     * to ensure deterministic behavior.
     */
    void Reset();

    // Prevent copying and moving
    ContentVersionManager(const ContentVersionManager&) = delete;
    ContentVersionManager& operator=(const ContentVersionManager&) = delete;
    ContentVersionManager(ContentVersionManager&&) = delete;
    ContentVersionManager& operator=(ContentVersionManager&&) = delete;

private:
    ContentVersionManager();
    ~ContentVersionManager() = default;

    /// Atomic counter for version generation
    std::atomic<uint64_t> version_counter_{0};
};

} // namespace lightui

#endif // LIGHTUI_CONTENT_VERSION_H
