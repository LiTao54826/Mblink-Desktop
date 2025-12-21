/**
 * @file upx_compressor.cpp
 * @brief UPX 压缩器实现
 */

#include "upx_compressor.h"
#include <filesystem>
#include <fstream>
#include <cstdlib>
#include <array>
#include <memory>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#define popen _popen
#define pclose _pclose
#endif

namespace fs = std::filesystem;

// 嵌入的 UPX.exe 数据（由 CMake 生成）
#if __has_include("generated/upx_exe.inc")
#include "generated/upx_exe.inc"
#define HAS_EMBEDDED_UPX 1
#else
#define HAS_EMBEDDED_UPX 0
static const unsigned char upx_exe_data[] = {};
static const size_t upx_exe_size = 0;
#endif

namespace mbink {

bool UPXCompressor::IsAvailable() const {
#if HAS_EMBEDDED_UPX
    return upx_exe_size > 0;
#else
    return false;
#endif
}

std::string UPXCompressor::ExtractUPX() {
#if !HAS_EMBEDDED_UPX
    error_ = "UPX not embedded in this build";
    return "";
#else
    if (upx_exe_size == 0) {
        error_ = "Embedded UPX data is empty";
        return "";
    }

    // 获取临时目录
    fs::path temp_dir = fs::temp_directory_path();
    fs::path upx_path = temp_dir / "mbink_upx.exe";

    // 检查是否已经存在且大小匹配
    if (fs::exists(upx_path)) {
        auto existing_size = fs::file_size(upx_path);
        if (existing_size == upx_exe_size) {
            return upx_path.string();
        }
        // 大小不匹配，删除重新释放
        fs::remove(upx_path);
    }

    // 写入临时文件
    std::ofstream file(upx_path, std::ios::binary);
    if (!file.is_open()) {
        error_ = "Failed to create temporary UPX file: " + upx_path.string();
        return "";
    }

    file.write(reinterpret_cast<const char*>(upx_exe_data), upx_exe_size);
    file.close();

    if (!fs::exists(upx_path)) {
        error_ = "Failed to write UPX to temporary file";
        return "";
    }

    return upx_path.string();
#endif
}

bool UPXCompressor::ExecuteUPX(const std::string& upx_path, const std::string& exe_path,
                                CompressionLevel level) {
    // 构建命令行
    std::string level_arg;
    switch (level) {
        case CompressionLevel::Fast:
            level_arg = "--fast";
            break;
        case CompressionLevel::Default:
            level_arg = "-7";
            break;
        case CompressionLevel::Best:
            level_arg = "--best";
            break;
        case CompressionLevel::UltraBrute:
            level_arg = "--ultra-brute";
            break;
    }

    // 构建完整命令
    std::string cmd = "\"" + upx_path + "\" " + level_arg + " -q \"" + exe_path + "\" 2>&1";

#ifdef _WIN32
    // Windows: 使用 CreateProcess 避免显示控制台窗口
    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    std::string full_cmd = "cmd /c \"" + cmd + "\"";
    
    if (!CreateProcessA(NULL, const_cast<char*>(full_cmd.c_str()), 
                        NULL, NULL, FALSE, CREATE_NO_WINDOW, 
                        NULL, NULL, &si, &pi)) {
        error_ = "Failed to execute UPX";
        return false;
    }

    // 等待完成
    WaitForSingleObject(pi.hProcess, INFINITE);
    
    DWORD exit_code;
    GetExitCodeProcess(pi.hProcess, &exit_code);
    
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    if (exit_code != 0) {
        error_ = "UPX returned error code: " + std::to_string(exit_code);
        return false;
    }
#else
    // Unix: 使用 system
    int result = std::system(cmd.c_str());
    if (result != 0) {
        error_ = "UPX returned error code: " + std::to_string(result);
        return false;
    }
#endif

    return true;
}

bool UPXCompressor::Compress(const std::string& exe_path, CompressionLevel level) {
    error_.clear();
    original_size_ = 0;
    compressed_size_ = 0;

    // 检查文件是否存在
    if (!fs::exists(exe_path)) {
        error_ = "File not found: " + exe_path;
        return false;
    }

    // 记录原始大小
    original_size_ = fs::file_size(exe_path);

    // 释放 UPX
    std::string upx_path = ExtractUPX();
    if (upx_path.empty()) {
        return false;
    }

    // 执行压缩
    if (!ExecuteUPX(upx_path, exe_path, level)) {
        return false;
    }

    // 记录压缩后大小
    compressed_size_ = fs::file_size(exe_path);

    return true;
}

}  // namespace mbink
