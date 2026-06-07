#pragma once

#include <atomic>
#include <functional>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace mbink {

class AsyncResourceContext {
public:
    using AssetProvider = std::function<bool(const std::string&, std::vector<uint8_t>&)>;

    struct Snapshot {
        std::string base_path;
        AssetProvider asset_provider;
        bool alive = false;
    };

    void SetBasePath(std::string path) {
        std::lock_guard<std::mutex> lock(mutex_);
        base_path_ = std::move(path);
    }

    std::string GetBasePath() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return base_path_;
    }

    void SetAssetProvider(AssetProvider provider) {
        std::lock_guard<std::mutex> lock(mutex_);
        asset_provider_ = std::move(provider);
    }

    AssetProvider GetAssetProvider() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return asset_provider_;
    }

    Snapshot MakeSnapshot() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return Snapshot{base_path_, asset_provider_, alive_.load()};
    }

    void MarkDead() {
        alive_ = false;
    }

    bool IsAlive() const {
        return alive_.load();
    }

private:
    mutable std::mutex mutex_;
    std::string base_path_;
    AssetProvider asset_provider_;
    std::atomic<bool> alive_{true};
};

} // namespace mbink
