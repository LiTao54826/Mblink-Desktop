/**
 * @file payload.h
 * @brief Payload 格式定义和构建/解析工具
 *
 * Payload 格式 v2:
 * [原始 app_loader.exe]
 * [Config JSON]
 * [Bytecode 数据]
 * [Assets 数据: asset1_data + asset2_data + ...]
 * [Asset Index: count(4) + [name_len(2) + name + offset(4) + size(4)] * count]
 * [Footer: config_size(4) + bytecode_size(4) + assets_size(4) + index_size(4) + crc32(4) + magic(4)]
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <map>

namespace mblink {

// Magic number: "MBPK" in little-endian
constexpr uint32_t PAYLOAD_MAGIC = 0x4B50424D;
constexpr uint32_t PAYLOAD_VERSION = 2;
constexpr size_t FOOTER_SIZE = 24;  // 4 + 4 + 4 + 4 + 4 + 4 bytes

/**
 * @brief Payload 配置数据
 */
struct PayloadConfig {
    uint32_t version = PAYLOAD_VERSION;
    int32_t width = 800;
    int32_t height = 600;
    std::string title = "MBlink App";
    uint32_t module_count = 0;
    bool borderless = false;
    bool transparent = false;
    bool gpu = true;          // 是否启用GPU加速（false时强制CPU渲染）
    int32_t min_width = 0;   // 窗口最小宽度（0 表示不限制）
    int32_t min_height = 0;  // 窗口最小高度（0 表示不限制）
    int32_t max_width = 0;   // 窗口最大宽度（0 表示不限制）
    int32_t max_height = 0;  // 窗口最大高度（0 表示不限制）
};

/**
 * @brief 单个资源信息
 */
struct AssetInfo {
    std::string name;       // 资源路径名 (如 "images/logo.png")
    uint32_t offset = 0;    // 在 assets 数据块中的偏移
    uint32_t size = 0;      // 资源大小
};

/**
 * @brief 解析后的 Payload 数据
 */
struct PayloadData {
    PayloadConfig config;
    std::vector<uint8_t> bytecode;
    std::vector<uint8_t> assets_data;           // 所有资源的原始数据
    std::vector<AssetInfo> assets_index;        // 资源索引
    uint32_t checksum = 0;
    bool valid = false;
    
    // 根据名称获取资源数据
    bool GetAsset(const std::string& name, std::vector<uint8_t>& out_data) const;
    bool HasAsset(const std::string& name) const;
};

/**
 * @brief 要打包的资源
 */
struct AssetEntry {
    std::string name;                   // 资源路径名
    std::vector<uint8_t> data;          // 资源数据
};

/**
 * @brief Payload 构建器
 */
class PayloadBuilder {
public:
    PayloadBuilder() = default;

    // 设置配置
    void SetConfig(const PayloadConfig& config) { config_ = config; }
    void SetWidth(int width) { config_.width = width; }
    void SetHeight(int height) { config_.height = height; }
    void SetTitle(const std::string& title) { config_.title = title; }
    void SetModuleCount(uint32_t count) { config_.module_count = count; }

    // 设置字节码
    void SetBytecode(const std::vector<uint8_t>& bytecode) { bytecode_ = bytecode; }
    void SetBytecode(std::vector<uint8_t>&& bytecode) { bytecode_ = std::move(bytecode); }

    // 添加资源
    void AddAsset(const std::string& name, const std::vector<uint8_t>& data);
    void AddAsset(const std::string& name, std::vector<uint8_t>&& data);
    
    // 从文件添加资源
    bool AddAssetFromFile(const std::string& name, const std::string& filepath);
    
    // 从目录递归添加资源
    bool AddAssetsFromDirectory(const std::string& dir_path, const std::string& prefix = "");

    // 获取资源数量
    size_t GetAssetCount() const { return assets_.size(); }

    // 构建完整的 payload
    std::vector<uint8_t> Build() const;

    // 从数据解析 payload（静态方法）
    static bool Parse(const std::vector<uint8_t>& data, PayloadData& out_data);

    // 从文件末尾检测并解析 payload
    static bool ParseFromFile(const std::string& filepath, PayloadData& out_data);

    // 检查数据末尾是否有有效的 payload magic
    static bool HasPayload(const std::vector<uint8_t>& data);

private:
    PayloadConfig config_;
    std::vector<uint8_t> bytecode_;
    std::vector<AssetEntry> assets_;

    // 序列化配置为 JSON
    std::string SerializeConfig() const;

    // 从 JSON 反序列化配置
    static bool DeserializeConfig(const std::string& json, PayloadConfig& config);
    
    // 构建资源数据和索引
    void BuildAssets(std::vector<uint8_t>& assets_data, 
                     std::vector<uint8_t>& assets_index) const;
    
    // 解析资源索引
    static bool ParseAssetsIndex(const uint8_t* data, size_t size,
                                 std::vector<AssetInfo>& out_index);
};

/**
 * @brief 计算 CRC32 校验和
 */
uint32_t CalculateCRC32(const uint8_t* data, size_t length);
uint32_t CalculateCRC32(const std::vector<uint8_t>& data);

}  // namespace mblink
