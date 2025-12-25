/**
 * @file property_tree_error.cpp
 * @brief 属性树系统错误处理实现
 */

#include "property_tree_error.h"
#include <chrono>
#include <iostream>

namespace lightui {

// ============================================================================
// PropertyTreeErrorHandler 实现
// ============================================================================

PropertyTreeErrorHandler& PropertyTreeErrorHandler::Instance() {
    static PropertyTreeErrorHandler instance;
    return instance;
}

PropertyTreeRecoveryStrategy PropertyTreeErrorHandler::ReportError(
    const PropertyTreeError& error) {
    // 更新最近错误
    last_error_ = error;
    last_error_.timestamp = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());

    // 记录历史
    if (record_history_) {
        error_history_.push_back(last_error_);
        if (error_history_.size() > max_history_size_) {
            error_history_.erase(error_history_.begin());
        }
    }

    // 调用回调
    if (error_callback_) {
        return error_callback_(error);
    }

    // 默认恢复策略
    switch (error.severity) {
        case PropertyTreeErrorSeverity::kInfo:
            return PropertyTreeRecoveryStrategy::kIgnore;
        case PropertyTreeErrorSeverity::kWarning:
            return PropertyTreeRecoveryStrategy::kIgnore;
        case PropertyTreeErrorSeverity::kError:
            return PropertyTreeRecoveryStrategy::kFallback;
        case PropertyTreeErrorSeverity::kFatal:
            return PropertyTreeRecoveryStrategy::kAbort;
        default:
            return PropertyTreeRecoveryStrategy::kIgnore;
    }
}

std::string PropertyTreeErrorHandler::GetErrorCodeDescription(
    PropertyTreeErrorCode code) {
    switch (code) {
        case PropertyTreeErrorCode::kNone:
            return "No error";

        // 节点错误
        case PropertyTreeErrorCode::kInvalidNodeId:
            return "Invalid node ID";
        case PropertyTreeErrorCode::kNodeNotFound:
            return "Node not found";
        case PropertyTreeErrorCode::kNodeAlreadyExists:
            return "Node already exists";
        case PropertyTreeErrorCode::kInvalidParentNode:
            return "Invalid parent node";
        case PropertyTreeErrorCode::kCircularReference:
            return "Circular reference detected";
        case PropertyTreeErrorCode::kOrphanNode:
            return "Orphan node detected";

        // 变换错误
        case PropertyTreeErrorCode::kInvalidTransformMatrix:
            return "Invalid transform matrix";
        case PropertyTreeErrorCode::kSingularMatrix:
            return "Singular matrix (not invertible)";
        case PropertyTreeErrorCode::kTransformOverflow:
            return "Transform overflow";
        case PropertyTreeErrorCode::kInvalidTransformOrigin:
            return "Invalid transform origin";

        // 裁剪错误
        case PropertyTreeErrorCode::kInvalidClipRect:
            return "Invalid clip rectangle";
        case PropertyTreeErrorCode::kInvalidClipPath:
            return "Invalid clip path";
        case PropertyTreeErrorCode::kClipRectOverflow:
            return "Clip rectangle overflow";

        // 效果错误
        case PropertyTreeErrorCode::kInvalidOpacity:
            return "Invalid opacity value";
        case PropertyTreeErrorCode::kInvalidFilter:
            return "Invalid filter";
        case PropertyTreeErrorCode::kInvalidBlendMode:
            return "Invalid blend mode";
        case PropertyTreeErrorCode::kFilterTooComplex:
            return "Filter too complex";

        // 滚动错误
        case PropertyTreeErrorCode::kInvalidScrollOffset:
            return "Invalid scroll offset";
        case PropertyTreeErrorCode::kInvalidContainerSize:
            return "Invalid container size";
        case PropertyTreeErrorCode::kInvalidContentSize:
            return "Invalid content size";
        case PropertyTreeErrorCode::kScrollOutOfBounds:
            return "Scroll out of bounds";

        // 几何映射错误
        case PropertyTreeErrorCode::kMappingFailed:
            return "Geometry mapping failed";
        case PropertyTreeErrorCode::kInvalidSourceState:
            return "Invalid source state";
        case PropertyTreeErrorCode::kInvalidTargetState:
            return "Invalid target state";
        case PropertyTreeErrorCode::kNoCommonAncestor:
            return "No common ancestor found";

        // 层化错误
        case PropertyTreeErrorCode::kLayerizationFailed:
            return "Layerization failed";
        case PropertyTreeErrorCode::kTooManyLayers:
            return "Too many layers";
        case PropertyTreeErrorCode::kInvalidPaintChunk:
            return "Invalid paint chunk";
        case PropertyTreeErrorCode::kChunkMergeFailed:
            return "Paint chunk merge failed";

        // 光栅化错误
        case PropertyTreeErrorCode::kRasterizationFailed:
            return "Rasterization failed";
        case PropertyTreeErrorCode::kInvalidationFailed:
            return "Invalidation computation failed";
        case PropertyTreeErrorCode::kTextureAllocationFailed:
            return "Texture allocation failed";
        case PropertyTreeErrorCode::kTextureUploadFailed:
            return "Texture upload failed";

        // 合成错误
        case PropertyTreeErrorCode::kCompositionFailed:
            return "Composition failed";
        case PropertyTreeErrorCode::kGPUContextLost:
            return "GPU context lost";
        case PropertyTreeErrorCode::kGPUOutOfMemory:
            return "GPU out of memory";
        case PropertyTreeErrorCode::kShaderCompileFailed:
            return "Shader compilation failed";

        // 系统错误
        case PropertyTreeErrorCode::kOutOfMemory:
            return "Out of memory";
        case PropertyTreeErrorCode::kInternalError:
            return "Internal error";
        case PropertyTreeErrorCode::kNotImplemented:
            return "Not implemented";
        case PropertyTreeErrorCode::kInvalidState:
            return "Invalid state";

        default:
            return "Unknown error";
    }
}

std::string PropertyTreeErrorHandler::GetSeverityDescription(
    PropertyTreeErrorSeverity severity) {
    switch (severity) {
        case PropertyTreeErrorSeverity::kInfo:
            return "Info";
        case PropertyTreeErrorSeverity::kWarning:
            return "Warning";
        case PropertyTreeErrorSeverity::kError:
            return "Error";
        case PropertyTreeErrorSeverity::kFatal:
            return "Fatal";
        default:
            return "Unknown";
    }
}

// ============================================================================
// GPUContextLossHandler 实现
// ============================================================================

GPUContextLossHandler& GPUContextLossHandler::Instance() {
    static GPUContextLossHandler instance;
    return instance;
}

void GPUContextLossHandler::OnContextLost() {
    state_ = GPUContextState::kLost;
    context_lost_count_++;

    // 报告错误
    PropertyTreeErrorHandler::Instance().ReportError(
        PropertyTreeError::Make(
            PropertyTreeErrorCode::kGPUContextLost,
            "GPU context lost, falling back to CPU rendering",
            PropertyTreeErrorSeverity::kError));

    // 调用回调
    if (context_lost_callback_) {
        context_lost_callback_();
    }
}

void GPUContextLossHandler::OnContextRestored() {
    state_ = GPUContextState::kValid;

    // 调用回调
    if (context_restored_callback_) {
        context_restored_callback_();
    }
}

bool GPUContextLossHandler::TryRestoreContext() {
    if (state_ != GPUContextState::kLost) {
        return state_ == GPUContextState::kValid;
    }

    state_ = GPUContextState::kRestoring;

    // 尝试恢复 GPU 上下文
    // 这里需要实际的 GPU 上下文恢复逻辑
    // 目前只是模拟

    // 假设恢复成功
    state_ = GPUContextState::kValid;

    if (context_restored_callback_) {
        context_restored_callback_();
    }

    return true;
}

} // namespace lightui
