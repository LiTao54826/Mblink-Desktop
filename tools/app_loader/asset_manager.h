/**
 * @file asset_manager.h
 * @brief 嵌入资源管理器 - 管理打包在 exe 中的资源
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include "payload.h"

namespace lightui {

/**
 * @brief 资源管理器（单例）
 */
class AssetManager {
public:
    static AssetManager& Instance();

    /**
     * @brief 初始化资源（从 payload 数据）
     */
    void Initialize(const std::vector<uint8_t>& assets_data,
                    const std::vector<mbink::AssetInfo>& assets_index);

    /**
     * @brief 检查资源是否存在
     */
    bool HasAsset(const std::string& path) const;

    /**
     * @brief 获取资源数据
     * @return 成功返回 true，数据写入 out_data
     */
    bool GetAsset(const std::string& path, std::vector<uint8_t>& out_data) const;

    /**
     * @brief 获取资源的 data URL（用于 img src 等）
     * @return data:mime/type;base64,... 格式的 URL
     */
    std::string GetAssetDataUrl(const std::string& path) const;

    /**
     * @brief 获取所有资源路径
     */
    std::vector<std::string> GetAssetPaths() const;

    /**
     * @brief 获取资源数量
     */
    size_t GetAssetCount() const { return index_.size(); }

    /**
     * @brief 清理资源
     */
    void Clear();

private:
    AssetManager() = default;
    ~AssetManager() = default;
    AssetManager(const AssetManager&) = delete;
    AssetManager& operator=(const AssetManager&) = delete;

    /**
     * @brief 根据文件扩展名获取 MIME 类型
     */
    static std::string GetMimeType(const std::string& path);

    /**
     * @brief Base64 编码
     */
    static std::string Base64Encode(const uint8_t* data, size_t length);

    struct AssetEntry {
        uint32_t offset;
        uint32_t size;
    };

    std::vector<uint8_t> data_;
    std::unordered_map<std::string, AssetEntry> index_;
};

}  // namespace lightui
