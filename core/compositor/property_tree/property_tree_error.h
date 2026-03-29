/**
 * @file property_tree_error.h
 * @brief 属性树系统错误处理
 *
 * 定义属性树系统的错误类型和处理策略：
 * - 错误码枚举
 * - 错误信息结构
 * - 错误处理回调
 * - GPU 上下文丢失回退
 */

#pragma once

#include <string>
#include <functional>
#include <vector>
#include <memory>

namespace mbink {

/**
 * @brief 属性树错误码
 */
enum class PropertyTreeErrorCode {
    kNone = 0,                      // 无错误

    // 节点错误 (100-199)
    kInvalidNodeId = 100,           // 无效节点 ID
    kNodeNotFound = 101,            // 节点未找到
    kNodeAlreadyExists = 102,       // 节点已存在
    kInvalidParentNode = 103,       // 无效父节点
    kCircularReference = 104,       // 循环引用
    kOrphanNode = 105,              // 孤立节点

    // 变换错误 (200-299)
    kInvalidTransformMatrix = 200,  // 无效变换矩阵
    kSingularMatrix = 201,          // 奇异矩阵（不可逆）
    kTransformOverflow = 202,       // 变换溢出
    kInvalidTransformOrigin = 203,  // 无效变换原点

    // 裁剪错误 (300-399)
    kInvalidClipRect = 300,         // 无效裁剪矩形
    kInvalidClipPath = 301,         // 无效裁剪路径
    kClipRectOverflow = 302,        // 裁剪区域溢出

    // 效果错误 (400-499)
    kInvalidOpacity = 400,          // 无效透明度值
    kInvalidFilter = 401,           // 无效滤镜
    kInvalidBlendMode = 402,        // 无效混合模式
    kFilterTooComplex = 403,        // 滤镜过于复杂

    // 滚动错误 (500-599)
    kInvalidScrollOffset = 500,     // 无效滚动偏移
    kInvalidContainerSize = 501,    // 无效容器尺寸
    kInvalidContentSize = 502,      // 无效内容尺寸
    kScrollOutOfBounds = 503,       // 滚动超出边界

    // 几何映射错误 (600-699)
    kMappingFailed = 600,           // 映射失败
    kInvalidSourceState = 601,      // 无效源状态
    kInvalidTargetState = 602,      // 无效目标状态
    kNoCommonAncestor = 603,        // 无公共祖先

    // 层化错误 (700-799)
    kLayerizationFailed = 700,      // 层化失败
    kTooManyLayers = 701,           // 层数过多
    kInvalidPaintChunk = 702,       // 无效绘制块
    kChunkMergeFailed = 703,        // 绘制块合并失败

    // 光栅化错误 (800-899)
    kRasterizationFailed = 800,     // 光栅化失败
    kInvalidationFailed = 801,      // 失效计算失败
    kTextureAllocationFailed = 802, // 纹理分配失败
    kTextureUploadFailed = 803,     // 纹理上传失败

    // 合成错误 (900-999)
    kCompositionFailed = 900,       // 合成失败
    kGPUContextLost = 901,          // GPU 上下文丢失
    kGPUOutOfMemory = 902,          // GPU 内存不足
    kShaderCompileFailed = 903,     // 着色器编译失败

    // 系统错误 (1000+)
    kOutOfMemory = 1000,            // 内存不足
    kInternalError = 1001,          // 内部错误
    kNotImplemented = 1002,         // 未实现
    kInvalidState = 1003,           // 无效状态
};

/**
 * @brief 错误严重级别
 */
enum class PropertyTreeErrorSeverity {
    kInfo,      // 信息（不影响功能）
    kWarning,   // 警告（可能影响性能或质量）
    kError,     // 错误（功能受损但可恢复）
    kFatal,     // 致命（无法恢复）
};

/**
 * @brief 错误恢复策略
 */
enum class PropertyTreeRecoveryStrategy {
    kIgnore,            // 忽略错误
    kRetry,             // 重试操作
    kFallback,          // 回退到备用方案
    kReset,             // 重置状态
    kAbort,             // 中止操作
};

/**
 * @brief 属性树错误信息
 */
struct PropertyTreeError {
    PropertyTreeErrorCode code = PropertyTreeErrorCode::kNone;
    PropertyTreeErrorSeverity severity = PropertyTreeErrorSeverity::kInfo;
    std::string message;
    std::string source;         // 错误来源（文件:行号）
    std::string context;        // 上下文信息
    uint64_t timestamp = 0;     // 时间戳

    bool IsError() const {
        return code != PropertyTreeErrorCode::kNone;
    }

    bool IsFatal() const {
        return severity == PropertyTreeErrorSeverity::kFatal;
    }

    static PropertyTreeError None() {
        return PropertyTreeError{};
    }

    static PropertyTreeError Make(PropertyTreeErrorCode code,
                                   const std::string& message,
                                   PropertyTreeErrorSeverity severity =
                                       PropertyTreeErrorSeverity::kError) {
        PropertyTreeError error;
        error.code = code;
        error.message = message;
        error.severity = severity;
        return error;
    }
};

/**
 * @brief 错误处理回调类型
 */
using PropertyTreeErrorCallback =
    std::function<PropertyTreeRecoveryStrategy(const PropertyTreeError&)>;

/**
 * @brief 属性树错误处理器
 *
 * 集中管理属性树系统的错误处理。
 */
class PropertyTreeErrorHandler {
public:
    /**
     * @brief 获取单例实例
     */
    static PropertyTreeErrorHandler& Instance();

    /**
     * @brief 报告错误
     * @param error 错误信息
     * @return 恢复策略
     */
    PropertyTreeRecoveryStrategy ReportError(const PropertyTreeError& error);

    /**
     * @brief 设置错误回调
     */
    void SetErrorCallback(PropertyTreeErrorCallback callback) {
        error_callback_ = std::move(callback);
    }

    /**
     * @brief 清除错误回调
     */
    void ClearErrorCallback() {
        error_callback_ = nullptr;
    }

    /**
     * @brief 获取最近的错误
     */
    const PropertyTreeError& GetLastError() const { return last_error_; }

    /**
     * @brief 清除最近的错误
     */
    void ClearLastError() { last_error_ = PropertyTreeError::None(); }

    /**
     * @brief 获取错误历史
     */
    const std::vector<PropertyTreeError>& GetErrorHistory() const {
        return error_history_;
    }

    /**
     * @brief 清除错误历史
     */
    void ClearErrorHistory() { error_history_.clear(); }

    /**
     * @brief 设置是否记录错误历史
     */
    void SetRecordHistory(bool record) { record_history_ = record; }

    /**
     * @brief 设置历史记录最大数量
     */
    void SetMaxHistorySize(size_t max_size) { max_history_size_ = max_size; }

    /**
     * @brief 获取错误码描述
     */
    static std::string GetErrorCodeDescription(PropertyTreeErrorCode code);

    /**
     * @brief 获取严重级别描述
     */
    static std::string GetSeverityDescription(PropertyTreeErrorSeverity severity);

private:
    PropertyTreeErrorHandler() = default;
    ~PropertyTreeErrorHandler() = default;

    PropertyTreeErrorHandler(const PropertyTreeErrorHandler&) = delete;
    PropertyTreeErrorHandler& operator=(const PropertyTreeErrorHandler&) = delete;

    PropertyTreeErrorCallback error_callback_;
    PropertyTreeError last_error_;
    std::vector<PropertyTreeError> error_history_;
    bool record_history_ = false;
    size_t max_history_size_ = 100;
};

/**
 * @brief GPU 上下文状态
 */
enum class GPUContextState {
    kValid,         // 有效
    kLost,          // 丢失
    kRestoring,     // 恢复中
    kInvalid,       // 无效
};

/**
 * @brief GPU 上下文丢失处理器
 *
 * 处理 GPU 上下文丢失和恢复。
 */
class GPUContextLossHandler {
public:
    /**
     * @brief 获取单例实例
     */
    static GPUContextLossHandler& Instance();

    /**
     * @brief 获取当前 GPU 上下文状态
     */
    GPUContextState GetState() const { return state_; }

    /**
     * @brief 通知 GPU 上下文丢失
     */
    void OnContextLost();

    /**
     * @brief 通知 GPU 上下文恢复
     */
    void OnContextRestored();

    /**
     * @brief 检查是否需要回退到 CPU 渲染
     */
    bool ShouldFallbackToCPU() const {
        return state_ == GPUContextState::kLost ||
               state_ == GPUContextState::kInvalid ||
               fallback_to_cpu_;
    }

    /**
     * @brief 设置是否强制回退到 CPU
     */
    void SetFallbackToCPU(bool fallback) { fallback_to_cpu_ = fallback; }

    /**
     * @brief 设置上下文丢失回调
     */
    using ContextLostCallback = std::function<void()>;
    void SetContextLostCallback(ContextLostCallback callback) {
        context_lost_callback_ = std::move(callback);
    }

    /**
     * @brief 设置上下文恢复回调
     */
    using ContextRestoredCallback = std::function<void()>;
    void SetContextRestoredCallback(ContextRestoredCallback callback) {
        context_restored_callback_ = std::move(callback);
    }

    /**
     * @brief 尝试恢复 GPU 上下文
     * @return true 如果恢复成功
     */
    bool TryRestoreContext();

    /**
     * @brief 获取上下文丢失次数
     */
    int GetContextLostCount() const { return context_lost_count_; }

    /**
     * @brief 重置统计
     */
    void ResetStatistics() { context_lost_count_ = 0; }

private:
    GPUContextLossHandler() = default;
    ~GPUContextLossHandler() = default;

    GPUContextLossHandler(const GPUContextLossHandler&) = delete;
    GPUContextLossHandler& operator=(const GPUContextLossHandler&) = delete;

    GPUContextState state_ = GPUContextState::kValid;
    bool fallback_to_cpu_ = false;
    int context_lost_count_ = 0;
    ContextLostCallback context_lost_callback_;
    ContextRestoredCallback context_restored_callback_;
};

// ============================================================================
// 便捷宏
// ============================================================================

/**
 * @brief 报告错误并返回恢复策略
 */
#define PROPERTY_TREE_REPORT_ERROR(code, message) \
    PropertyTreeErrorHandler::Instance().ReportError( \
        PropertyTreeError::Make(code, message))

/**
 * @brief 报告错误并返回指定值
 */
#define PROPERTY_TREE_RETURN_ON_ERROR(code, message, return_value) \
    do { \
        auto strategy = PROPERTY_TREE_REPORT_ERROR(code, message); \
        if (strategy == PropertyTreeRecoveryStrategy::kAbort) { \
            return return_value; \
        } \
    } while (0)

/**
 * @brief 检查条件，失败时报告错误
 */
#define PROPERTY_TREE_CHECK(condition, code, message) \
    do { \
        if (!(condition)) { \
            PROPERTY_TREE_REPORT_ERROR(code, message); \
        } \
    } while (0)

/**
 * @brief 检查条件，失败时报告错误并返回
 */
#define PROPERTY_TREE_CHECK_RETURN(condition, code, message, return_value) \
    do { \
        if (!(condition)) { \
            PROPERTY_TREE_RETURN_ON_ERROR(code, message, return_value); \
        } \
    } while (0)

} // namespace mbink
