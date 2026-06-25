/**
 * @file upx_compressor.h
 * @brief UPX 压缩器 - 使用嵌入的 UPX.exe 压缩可执行文件
 */

#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace mblink {

/**
 * @brief UPX 压缩器
 */
class UPXCompressor {
public:
    enum class CompressionLevel {
        Fast,       // --fast (-1)
        Default,    // 默认 (-7)
        Best,       // --best (-9)
        UltraBrute  // --ultra-brute (最慢但最小)
    };

    UPXCompressor() = default;
    ~UPXCompressor() = default;

    /**
     * @brief 压缩可执行文件
     * @param exe_path 要压缩的 exe 路径
     * @param level 压缩级别
     * @return 是否成功
     */
    bool Compress(const std::string& exe_path, CompressionLevel level = CompressionLevel::Best);

    /**
     * @brief 检查 UPX 是否可用
     */
    bool IsAvailable() const;

    /**
     * @brief 获取错误信息
     */
    const std::string& GetError() const { return error_; }

    /**
     * @brief 获取压缩前后的大小
     */
    size_t GetOriginalSize() const { return original_size_; }
    size_t GetCompressedSize() const { return compressed_size_; }

private:
    /**
     * @brief 释放嵌入的 UPX.exe 到临时目录
     * @return 临时文件路径，失败返回空
     */
    std::string ExtractUPX();

    /**
     * @brief 执行 UPX 命令
     */
    bool ExecuteUPX(const std::string& upx_path, const std::string& exe_path, 
                    CompressionLevel level);

    std::string error_;
    size_t original_size_ = 0;
    size_t compressed_size_ = 0;
};

}  // namespace mblink
