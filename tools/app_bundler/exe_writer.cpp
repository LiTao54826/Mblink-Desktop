/**
 * @file exe_writer.cpp
 * @brief Exe 写入器实现
 */

#include "exe_writer.h"
#include <fstream>
#include <filesystem>
#include <cstring>
#include <iostream>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace fs = std::filesystem;

namespace mblink {

// PE 文件常量（避免与 windows.h 宏冲突）
constexpr uint16_t kSubsystemWindowsGUI = 2;
constexpr uint16_t kSubsystemWindowsCUI = 3;

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
    uint16_t subsystem = use_console ? kSubsystemWindowsCUI : kSubsystemWindowsGUI;
    std::memcpy(&data[subsystem_offset], &subsystem, sizeof(subsystem));

    return true;
}

bool ExeWriter::WriteOutput(const std::string& output_path,
                            const std::vector<uint8_t>& payload,
                            bool show_console,
                            const std::string& icon_path) {
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

    // 步骤1: 先写模板 exe（不含 payload）
    {
        std::ofstream file(output_path, std::ios::binary);
        if (!file.is_open()) {
            error_ = "Failed to create output file: " + output_path;
            return false;
        }
        if (!file.write(reinterpret_cast<const char*>(output_data.data()),
                        static_cast<std::streamsize>(output_data.size()))) {
            error_ = "Failed to write template data";
            return false;
        }
    }  // 文件关闭

    // 步骤2: 如果指定了图标，用 UpdateResource 注入（必须在 payload 追加之前）
    if (!icon_path.empty()) {
        if (!SetIcon(output_path, icon_path)) {
            // SetIcon 已设置 error_，但不阻断打包流程，仅警告
            std::cerr << "  ⚠ 图标设置失败: " << error_ << "\n";
            error_.clear();
        }
    }

    // 步骤3: 追加 payload
    if (!payload.empty()) {
        std::ofstream file(output_path, std::ios::binary | std::ios::app);
        if (!file.is_open()) {
            error_ = "Failed to reopen output file for payload: " + output_path;
            return false;
        }
        if (!file.write(reinterpret_cast<const char*>(payload.data()),
                        static_cast<std::streamsize>(payload.size()))) {
            error_ = "Failed to write payload data";
            return false;
        }
    }

    return true;
}

#ifdef _WIN32

// ICO 文件格式结构体
#pragma pack(push, 1)
struct ICONDIR_FILE {
    uint16_t reserved;   // 保留，必须为 0
    uint16_t type;       // 资源类型，1 = ICO
    uint16_t count;      // 图像数量
};

struct ICONDIRENTRY_FILE {
    uint8_t  width;       // 宽度（0 表示 256）
    uint8_t  height;      // 高度（0 表示 256）
    uint8_t  colorCount;  // 颜色数（0 表示 >=256）
    uint8_t  reserved;
    uint16_t planes;
    uint16_t bitCount;
    uint32_t bytesInRes;  // 图像数据大小
    uint32_t imageOffset; // 图像数据在文件中的偏移
};

// RT_GROUP_ICON 中的条目（最后字段是 nID 而不是 imageOffset）
struct GRPICONDIRENTRY {
    uint8_t  width;
    uint8_t  height;
    uint8_t  colorCount;
    uint8_t  reserved;
    uint16_t planes;
    uint16_t bitCount;
    uint32_t bytesInRes;
    uint16_t nID;         // RT_ICON 资源 ID
};
#pragma pack(pop)

bool ExeWriter::SetIcon(const std::string& exe_path, const std::string& ico_path) {
    // 1. 读取 ICO 文件
    std::ifstream ico_file(ico_path, std::ios::binary | std::ios::ate);
    if (!ico_file.is_open()) {
        error_ = "Failed to open icon file: " + ico_path;
        return false;
    }

    std::streamsize ico_size = ico_file.tellg();
    ico_file.seekg(0, std::ios::beg);

    if (ico_size < static_cast<std::streamsize>(sizeof(ICONDIR_FILE))) {
        error_ = "Invalid ICO file: too small";
        return false;
    }

    std::vector<uint8_t> ico_data(static_cast<size_t>(ico_size));
    if (!ico_file.read(reinterpret_cast<char*>(ico_data.data()), ico_size)) {
        error_ = "Failed to read icon file";
        return false;
    }
    ico_file.close();

    // 2. 解析 ICONDIR
    auto* icon_dir = reinterpret_cast<const ICONDIR_FILE*>(ico_data.data());
    if (icon_dir->reserved != 0 || icon_dir->type != 1 || icon_dir->count == 0) {
        error_ = "Invalid ICO file format";
        return false;
    }

    uint16_t image_count = icon_dir->count;
    size_t entries_end = sizeof(ICONDIR_FILE) + image_count * sizeof(ICONDIRENTRY_FILE);
    if (entries_end > ico_data.size()) {
        error_ = "Invalid ICO file: truncated directory";
        return false;
    }

    auto* entries = reinterpret_cast<const ICONDIRENTRY_FILE*>(
        ico_data.data() + sizeof(ICONDIR_FILE));

    // 3. 使用 UpdateResource 注入图标
    std::wstring wide_path(exe_path.begin(), exe_path.end());
    HANDLE hUpdate = BeginUpdateResourceW(wide_path.c_str(), FALSE);
    if (!hUpdate) {
        error_ = "BeginUpdateResource failed (error " + std::to_string(GetLastError()) + ")";
        return false;
    }

    // 4. 写入每个 RT_ICON 资源（ID 从 1 开始）
    for (uint16_t i = 0; i < image_count; i++) {
        const auto& entry = entries[i];

        if (entry.imageOffset + entry.bytesInRes > ico_data.size()) {
            EndUpdateResourceW(hUpdate, TRUE);  // 丢弃
            error_ = "Invalid ICO file: image data out of bounds";
            return false;
        }

        if (!UpdateResourceW(hUpdate,
                             MAKEINTRESOURCEW(3),  // RT_ICON = 3
                             MAKEINTRESOURCEW(i + 1),
                             MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                             static_cast<LPVOID>(const_cast<uint8_t*>(ico_data.data() + entry.imageOffset)),
                             entry.bytesInRes)) {
            EndUpdateResourceW(hUpdate, TRUE);
            error_ = "UpdateResource RT_ICON failed (error " + std::to_string(GetLastError()) + ")";
            return false;
        }
    }

    // 5. 构建 RT_GROUP_ICON 资源
    size_t grp_size = sizeof(ICONDIR_FILE) + image_count * sizeof(GRPICONDIRENTRY);
    std::vector<uint8_t> grp_data(grp_size);

    auto* grp_header = reinterpret_cast<ICONDIR_FILE*>(grp_data.data());
    grp_header->reserved = 0;
    grp_header->type = 1;
    grp_header->count = image_count;

    auto* grp_entries = reinterpret_cast<GRPICONDIRENTRY*>(
        grp_data.data() + sizeof(ICONDIR_FILE));

    for (uint16_t i = 0; i < image_count; i++) {
        grp_entries[i].width      = entries[i].width;
        grp_entries[i].height     = entries[i].height;
        grp_entries[i].colorCount = entries[i].colorCount;
        grp_entries[i].reserved   = entries[i].reserved;
        grp_entries[i].planes     = entries[i].planes;
        grp_entries[i].bitCount   = entries[i].bitCount;
        grp_entries[i].bytesInRes = entries[i].bytesInRes;
        grp_entries[i].nID        = i + 1;  // 对应 RT_ICON 的 ID
    }

    if (!UpdateResourceW(hUpdate,
                         MAKEINTRESOURCEW(14),  // RT_GROUP_ICON = 14
                         MAKEINTRESOURCEW(1),   // 主图标组 ID = 1
                         MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                         static_cast<LPVOID>(grp_data.data()),
                         static_cast<DWORD>(grp_size))) {
        EndUpdateResourceW(hUpdate, TRUE);
        error_ = "UpdateResource RT_GROUP_ICON failed (error " + std::to_string(GetLastError()) + ")";
        return false;
    }

    // 6. 提交更改
    if (!EndUpdateResourceW(hUpdate, FALSE)) {
        error_ = "EndUpdateResource failed (error " + std::to_string(GetLastError()) + ")";
        return false;
    }

    return true;
}

#else

bool ExeWriter::SetIcon(const std::string& /*exe_path*/, const std::string& /*ico_path*/) {
    error_ = "Icon injection is only supported on Windows";
    return false;
}

#endif  // _WIN32

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

}  // namespace mblink
