/**
 * @file payload.cpp
 * @brief Payload 构建和解析实现
 */

#include "payload.h"
#include <fstream>
#include <sstream>
#include <cstring>
#include <filesystem>

namespace fs = std::filesystem;

namespace mbink {

// CRC32 查找表
static uint32_t crc32_table[256];
static bool crc32_table_initialized = false;

static void InitCRC32Table() {
    if (crc32_table_initialized) return;
    
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t crc = i;
        for (int j = 0; j < 8; j++) {
            crc = (crc >> 1) ^ ((crc & 1) ? 0xEDB88320 : 0);
        }
        crc32_table[i] = crc;
    }
    crc32_table_initialized = true;
}

uint32_t CalculateCRC32(const uint8_t* data, size_t length) {
    InitCRC32Table();
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < length; i++) {
        crc = (crc >> 8) ^ crc32_table[(crc ^ data[i]) & 0xFF];
    }
    return crc ^ 0xFFFFFFFF;
}

uint32_t CalculateCRC32(const std::vector<uint8_t>& data) {
    return CalculateCRC32(data.data(), data.size());
}

// PayloadData 方法实现
bool PayloadData::GetAsset(const std::string& name, std::vector<uint8_t>& out_data) const {
    for (const auto& asset : assets_index) {
        if (asset.name == name) {
            if (asset.offset + asset.size <= assets_data.size()) {
                out_data.assign(
                    assets_data.begin() + asset.offset,
                    assets_data.begin() + asset.offset + asset.size
                );
                return true;
            }
            return false;
        }
    }
    return false;
}

bool PayloadData::HasAsset(const std::string& name) const {
    for (const auto& asset : assets_index) {
        if (asset.name == name) return true;
    }
    return false;
}

// 简单的 JSON 序列化（不依赖外部库）
std::string PayloadBuilder::SerializeConfig() const {
    std::ostringstream ss;
    ss << "{";
    ss << "\"version\":" << config_.version << ",";
    ss << "\"width\":" << config_.width << ",";
    ss << "\"height\":" << config_.height << ",";
    ss << "\"title\":\"";
    for (char c : config_.title) {
        if (c == '"') ss << "\\\"";
        else if (c == '\\') ss << "\\\\";
        else if (c == '\n') ss << "\\n";
        else if (c == '\r') ss << "\\r";
        else if (c == '\t') ss << "\\t";
        else ss << c;
    }
    ss << "\",";
    ss << "\"module_count\":" << config_.module_count << ",";
    ss << "\"borderless\":" << (config_.borderless ? "true" : "false") << ",";
    ss << "\"transparent\":" << (config_.transparent ? "true" : "false");
    ss << "}";
    return ss.str();
}

bool PayloadBuilder::DeserializeConfig(const std::string& json, PayloadConfig& config) {
    auto pos = json.find("\"version\":");
    if (pos != std::string::npos) {
        config.version = std::stoul(json.substr(pos + 10));
    }
    
    pos = json.find("\"width\":");
    if (pos != std::string::npos) {
        config.width = std::stoi(json.substr(pos + 8));
    }
    
    pos = json.find("\"height\":");
    if (pos != std::string::npos) {
        config.height = std::stoi(json.substr(pos + 9));
    }
    
    pos = json.find("\"title\":\"");
    if (pos != std::string::npos) {
        size_t start = pos + 9;
        std::string title;
        bool escaped = false;
        for (size_t i = start; i < json.size(); i++) {
            char c = json[i];
            if (escaped) {
                switch (c) {
                    case '"': title += '"'; break;
                    case '\\': title += '\\'; break;
                    case 'n': title += '\n'; break;
                    case 'r': title += '\r'; break;
                    case 't': title += '\t'; break;
                    default: title += c; break;
                }
                escaped = false;
            } else if (c == '\\') {
                escaped = true;
            } else if (c == '"') {
                break;
            } else {
                title += c;
            }
        }
        config.title = title;
    }
    
    pos = json.find("\"module_count\":");
    if (pos != std::string::npos) {
        config.module_count = std::stoul(json.substr(pos + 15));
    }

    pos = json.find("\"borderless\":");
    if (pos != std::string::npos) {
        config.borderless = (json.substr(pos + 13, 4) == "true");
    }

    pos = json.find("\"transparent\":");
    if (pos != std::string::npos) {
        config.transparent = (json.substr(pos + 14, 4) == "true");
    }

    return true;
}

// 资源相关方法
void PayloadBuilder::AddAsset(const std::string& name, const std::vector<uint8_t>& data) {
    assets_.push_back({name, data});
}

void PayloadBuilder::AddAsset(const std::string& name, std::vector<uint8_t>&& data) {
    assets_.push_back({name, std::move(data)});
}

bool PayloadBuilder::AddAssetFromFile(const std::string& name, const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return false;
    
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::vector<uint8_t> data(size);
    if (!file.read(reinterpret_cast<char*>(data.data()), size)) {
        return false;
    }
    
    AddAsset(name, std::move(data));
    return true;
}

bool PayloadBuilder::AddAssetsFromDirectory(const std::string& dir_path, const std::string& prefix) {
    if (!fs::exists(dir_path) || !fs::is_directory(dir_path)) {
        return false;
    }
    
    for (const auto& entry : fs::recursive_directory_iterator(dir_path)) {
        if (entry.is_regular_file()) {
            // 计算相对路径
            fs::path rel_path = fs::relative(entry.path(), dir_path);
            std::string name = rel_path.generic_string();  // 使用 / 作为分隔符
            
            if (!prefix.empty()) {
                name = prefix + "/" + name;
            }
            
            if (!AddAssetFromFile(name, entry.path().string())) {
                return false;
            }
        }
    }
    
    return true;
}

void PayloadBuilder::BuildAssets(std::vector<uint8_t>& assets_data, 
                                  std::vector<uint8_t>& assets_index) const {
    assets_data.clear();
    assets_index.clear();
    
    if (assets_.empty()) return;
    
    // 构建资源数据
    std::vector<AssetInfo> index;
    for (const auto& asset : assets_) {
        AssetInfo info;
        info.name = asset.name;
        info.offset = static_cast<uint32_t>(assets_data.size());
        info.size = static_cast<uint32_t>(asset.data.size());
        index.push_back(info);
        
        assets_data.insert(assets_data.end(), asset.data.begin(), asset.data.end());
    }
    
    // 构建索引: count(4) + [name_len(2) + name + offset(4) + size(4)] * count
    auto write_u32 = [&assets_index](uint32_t value) {
        assets_index.push_back(static_cast<uint8_t>(value & 0xFF));
        assets_index.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
        assets_index.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
        assets_index.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
    };
    
    auto write_u16 = [&assets_index](uint16_t value) {
        assets_index.push_back(static_cast<uint8_t>(value & 0xFF));
        assets_index.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    };
    
    write_u32(static_cast<uint32_t>(index.size()));
    
    for (const auto& info : index) {
        write_u16(static_cast<uint16_t>(info.name.size()));
        assets_index.insert(assets_index.end(), info.name.begin(), info.name.end());
        write_u32(info.offset);
        write_u32(info.size);
    }
}

bool PayloadBuilder::ParseAssetsIndex(const uint8_t* data, size_t size,
                                       std::vector<AssetInfo>& out_index) {
    out_index.clear();
    if (size < 4) return true;  // 空索引是有效的
    
    auto read_u32 = [](const uint8_t* p) -> uint32_t {
        return static_cast<uint32_t>(p[0]) |
               (static_cast<uint32_t>(p[1]) << 8) |
               (static_cast<uint32_t>(p[2]) << 16) |
               (static_cast<uint32_t>(p[3]) << 24);
    };
    
    auto read_u16 = [](const uint8_t* p) -> uint16_t {
        return static_cast<uint16_t>(p[0]) |
               (static_cast<uint16_t>(p[1]) << 8);
    };
    
    uint32_t count = read_u32(data);
    size_t offset = 4;
    
    for (uint32_t i = 0; i < count; i++) {
        if (offset + 2 > size) return false;
        
        uint16_t name_len = read_u16(data + offset);
        offset += 2;
        
        if (offset + name_len + 8 > size) return false;
        
        AssetInfo info;
        info.name = std::string(reinterpret_cast<const char*>(data + offset), name_len);
        offset += name_len;
        
        info.offset = read_u32(data + offset);
        offset += 4;
        
        info.size = read_u32(data + offset);
        offset += 4;
        
        out_index.push_back(info);
    }
    
    return true;
}

std::vector<uint8_t> PayloadBuilder::Build() const {
    std::string config_json = SerializeConfig();
    
    // 构建资源数据和索引
    std::vector<uint8_t> assets_data, assets_index;
    BuildAssets(assets_data, assets_index);
    
    uint32_t config_size = static_cast<uint32_t>(config_json.size());
    uint32_t bytecode_size = static_cast<uint32_t>(bytecode_.size());
    uint32_t assets_size = static_cast<uint32_t>(assets_data.size());
    uint32_t index_size = static_cast<uint32_t>(assets_index.size());
    
    // 构建 payload: [config] + [bytecode] + [assets] + [index] + [footer]
    std::vector<uint8_t> payload;
    payload.reserve(config_size + bytecode_size + assets_size + index_size + FOOTER_SIZE);
    
    // 写入配置 JSON
    payload.insert(payload.end(), config_json.begin(), config_json.end());
    
    // 写入字节码
    payload.insert(payload.end(), bytecode_.begin(), bytecode_.end());
    
    // 写入资源数据
    payload.insert(payload.end(), assets_data.begin(), assets_data.end());
    
    // 写入资源索引
    payload.insert(payload.end(), assets_index.begin(), assets_index.end());
    
    // 计算 CRC32（不包括 footer）
    uint32_t crc = CalculateCRC32(payload);
    
    // 写入 footer
    auto write_u32 = [&payload](uint32_t value) {
        payload.push_back(static_cast<uint8_t>(value & 0xFF));
        payload.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
        payload.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
        payload.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
    };
    
    write_u32(config_size);
    write_u32(bytecode_size);
    write_u32(assets_size);
    write_u32(index_size);
    write_u32(crc);
    write_u32(PAYLOAD_MAGIC);
    
    return payload;
}

bool PayloadBuilder::HasPayload(const std::vector<uint8_t>& data) {
    if (data.size() < FOOTER_SIZE) return false;
    
    size_t offset = data.size() - 4;
    uint32_t magic = 
        static_cast<uint32_t>(data[offset]) |
        (static_cast<uint32_t>(data[offset + 1]) << 8) |
        (static_cast<uint32_t>(data[offset + 2]) << 16) |
        (static_cast<uint32_t>(data[offset + 3]) << 24);
    
    return magic == PAYLOAD_MAGIC;
}

bool PayloadBuilder::Parse(const std::vector<uint8_t>& data, PayloadData& out_data) {
    out_data.valid = false;
    
    if (!HasPayload(data)) {
        return false;
    }
    
    auto read_u32 = [&data](size_t offset) -> uint32_t {
        return static_cast<uint32_t>(data[offset]) |
               (static_cast<uint32_t>(data[offset + 1]) << 8) |
               (static_cast<uint32_t>(data[offset + 2]) << 16) |
               (static_cast<uint32_t>(data[offset + 3]) << 24);
    };
    
    size_t footer_start = data.size() - FOOTER_SIZE;
    uint32_t config_size = read_u32(footer_start);
    uint32_t bytecode_size = read_u32(footer_start + 4);
    uint32_t assets_size = read_u32(footer_start + 8);
    uint32_t index_size = read_u32(footer_start + 12);
    uint32_t stored_crc = read_u32(footer_start + 16);
    
    size_t payload_size = config_size + bytecode_size + assets_size + index_size + FOOTER_SIZE;
    if (payload_size > data.size()) {
        return false;
    }
    
    size_t payload_start = data.size() - payload_size;
    
    // 验证 CRC32
    uint32_t calculated_crc = CalculateCRC32(
        data.data() + payload_start, 
        config_size + bytecode_size + assets_size + index_size
    );
    
    if (calculated_crc != stored_crc) {
        return false;
    }
    
    out_data.checksum = stored_crc;
    
    // 解析配置
    std::string config_json(
        data.begin() + payload_start,
        data.begin() + payload_start + config_size
    );
    
    if (!DeserializeConfig(config_json, out_data.config)) {
        return false;
    }
    
    // 提取字节码
    size_t bytecode_start = payload_start + config_size;
    out_data.bytecode.assign(
        data.begin() + bytecode_start,
        data.begin() + bytecode_start + bytecode_size
    );
    
    // 提取资源数据
    size_t assets_start = bytecode_start + bytecode_size;
    out_data.assets_data.assign(
        data.begin() + assets_start,
        data.begin() + assets_start + assets_size
    );
    
    // 解析资源索引
    size_t index_start = assets_start + assets_size;
    if (!ParseAssetsIndex(data.data() + index_start, index_size, out_data.assets_index)) {
        return false;
    }
    
    out_data.valid = true;
    return true;
}

bool PayloadBuilder::ParseFromFile(const std::string& filepath, PayloadData& out_data) {
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return false;
    }
    
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::vector<uint8_t> data(size);
    if (!file.read(reinterpret_cast<char*>(data.data()), size)) {
        return false;
    }
    
    return Parse(data, out_data);
}

}  // namespace mbink
