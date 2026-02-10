/**
 * @file exe_writer.cpp
 * @brief Exe 写入器实现
 */

#include "exe_writer.h"
#include <fstream>
#include <filesystem>
#include <cstring>

namespace fs = std::filesystem;

namespace mbink {

// PE 文件常量
constexpr uint16_t IMAGE_SUBSYSTEM_WINDOWS_GUI = 2;
constexpr uint16_t IMAGE_SUBSYSTEM_WINDOWS_CUI = 3;

bool ExeWriter::LoadTemplate(const std::string& template_path) {
    error_.clear();
    template_data_.clear();

    if (!fs::exists(template_path)) {
        error_ = "Template file not found: " + template_path;
        return false;
    }

    std::ifstream file(template_path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        error_ = "Failed to open template file: " + template_path;
        return false;
    }

    std::streamsize size = file.tellg();
    if (size <= 0) {
        error_ = "Template file is empty: " + template_path;
        return false;
    }

    file.seekg(0, std::ios::beg);
    template_data_.resize(static_cast<size_t>(size));

    if (!file.read(reinterpret_cast<char*>(template_data_.data()), size)) {
        error_ = "Failed to read template file: " + template_path;
        template_data_.clear();
        return false;
    }

    return true;
}

bool ExeWriter::SetSubsystem(std::vector<uint8_t>& data, bool use_console) {
    // PE 文件结构:
    // - DOS Header (64 bytes), e_lfanew at offset 0x3C points to PE signature
    // - PE Signature "PE\0\0" (4 bytes)
    // - COFF File Header (20 bytes)
    // - Optional Header: Subsystem at offset 68 (0x44) from Optional Header start
    
    if (data.size() < 64) {
        error_ = "Invalid PE file: too small for DOS header";
        return false;
    }

    // 检查 DOS 签名 "MZ"
    if (data[0] != 'M' || data[1] != 'Z') {
        error_ = "Invalid PE file: missing MZ signature";
        return false;
    }

    // 获取 PE 头偏移 (e_lfanew at offset 0x3C)
    uint32_t pe_offset = *reinterpret_cast<uint32_t*>(&data[0x3C]);
    
    if (pe_offset + 4 + 20 + 70 > data.size()) {
        error_ = "Invalid PE file: PE header out of bounds";
        return false;
    }

    // 检查 PE 签名
    if (data[pe_offset] != 'P' || data[pe_offset + 1] != 'E' ||
        data[pe_offset + 2] != 0 || data[pe_offset + 3] != 0) {
        error_ = "Invalid PE file: missing PE signature";
        return false;
    }

    // Subsystem 位于 Optional Header 的偏移 68 处
    // Optional Header 起始于 PE signature (4) + COFF header (20) = pe_offset + 24
    size_t subsystem_offset = pe_offset + 24 + 68;
    
    if (subsystem_offset + 2 > data.size()) {
        error_ = "Invalid PE file: subsystem field out of bounds";
        return false;
    }

    // 设置 subsystem
    uint16_t subsystem = use_console ? IMAGE_SUBSYSTEM_WINDOWS_CUI : IMAGE_SUBSYSTEM_WINDOWS_GUI;
    std::memcpy(&data[subsystem_offset], &subsystem, sizeof(subsystem));

    return true;
}

bool ExeWriter::WriteOutput(const std::string& output_path,
                            const std::vector<uint8_t>& payload,
                            bool show_console) {
    error_.clear();

    if (template_data_.empty()) {
        error_ = "No template loaded";
        return false;
    }

    // 复制模板数据以便修改
    std::vector<uint8_t> output_data = template_data_;

    // 修改 subsystem
    if (!SetSubsystem(output_data, show_console)) {
        return false;
    }

    // 确保输出目录存在
    fs::path output_dir = fs::path(output_path).parent_path();
    if (!output_dir.empty() && !fs::exists(output_dir)) {
        try {
            fs::create_directories(output_dir);
        } catch (const std::exception& e) {
            error_ = "Failed to create output directory: " + std::string(e.what());
            return false;
        }
    }

    std::ofstream file(output_path, std::ios::binary);
    if (!file.is_open()) {
        error_ = "Failed to create output file: " + output_path;
        return false;
    }

    // 写入修改后的模板 exe
    if (!file.write(reinterpret_cast<const char*>(output_data.data()),
                    static_cast<std::streamsize>(output_data.size()))) {
        error_ = "Failed to write template data";
        return false;
    }

    // 追加 payload
    if (!payload.empty()) {
        if (!file.write(reinterpret_cast<const char*>(payload.data()),
                        static_cast<std::streamsize>(payload.size()))) {
            error_ = "Failed to write payload data";
            return false;
        }
    }

    return true;
}

std::string ExeWriter::FindTemplate(const std::string& bundler_path) {
    fs::path bundler = fs::path(bundler_path);

    // 搜索路径列表：优先查找 esm_loader.exe（ESM 基座），其次 app_loader.exe（旧基座）
    std::vector<fs::path> search_paths = {
        // esm_loader（新基座，支持 ESM import/export）
        bundler.parent_path() / "esm_loader.exe",
        bundler.parent_path().parent_path() / "esm_loader.exe",
        bundler.parent_path() / "Debug" / "esm_loader.exe",
        bundler.parent_path() / "Release" / "esm_loader.exe",
        bundler.parent_path().parent_path() / "Debug" / "esm_loader.exe",
        bundler.parent_path().parent_path() / "Release" / "esm_loader.exe",
        // app_loader（旧基座，兼容回退）
        bundler.parent_path() / "app_loader.exe",
        bundler.parent_path().parent_path() / "app_loader.exe",
        bundler.parent_path() / "Debug" / "app_loader.exe",
        bundler.parent_path() / "Release" / "app_loader.exe",
        bundler.parent_path().parent_path() / "Debug" / "app_loader.exe",
        bundler.parent_path().parent_path() / "Release" / "app_loader.exe",
    };

    for (const auto& path : search_paths) {
        if (fs::exists(path)) {
            return fs::canonical(path).string();
        }
    }

    return "";
}

}  // namespace mbink
