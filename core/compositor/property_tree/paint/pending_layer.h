/**
 * @file pending_layer.h
 * @brief 待定层
 *
 * 待定层是层化过程中的中间表示：
 * - 包含一组绘制块
 * - 属性树状态
 * - 边界
 * - 合成类型
 *
 * 参考 Chromium Blink: platform/graphics/compositing/pending_layer.h
 */

#pragma once

#include "core/compositor/property_tree/paint/paint_chunk.h"
#include "core/compositor/property_tree/property_tree_state.h"
#include "core/compositor/property_tree/compositing_reasons.h"
#include "include/core/SkRect.h"
#include <vector>

namespace mbink {

/**
 * @brief 合成类型
 */
enum class CompositingType {
    kNone,              // 不需要独立层
    kOverlap,           // 因重叠需要独立层
    kDirectReason,      // 因直接原因需要独立层（will-change、动画等）
    kForeignLayer,      // 外部层（video、canvas 等）
    kScrollbar,         // 滚动条层
};

/**
 * @brief 待定层
 *
 * 层化过程中的中间表示。
 * 收集具有相同合成需求的绘制块。
 */
class PendingLayer {
public:
    PendingLayer() = default;
    ~PendingLayer() = default;

    // 支持移动
    PendingLayer(PendingLayer&&) = default;
    PendingLayer& operator=(PendingLayer&&) = default;
    
    // 支持拷贝
    PendingLayer(const PendingLayer&) = default;
    PendingLayer& operator=(const PendingLayer&) = default;

    // =========================================================================
    // 绘制块
    // =========================================================================

    /**
     * @brief 获取包含的绘制块
     */
    const std::vector<const PaintChunk*>& GetChunks() const { return chunks_; }

    /**
     * @brief 添加绘制块
     */
    void AddChunk(const PaintChunk* chunk);

    /**
     * @brief 获取绘制块数量
     */
    size_t GetChunkCount() const { return chunks_.size(); }

    /**
     * @brief 是否为空
     */
    bool IsEmpty() const { return chunks_.empty(); }

    // =========================================================================
    // 属性树状态
    // =========================================================================

    /**
     * @brief 获取属性树状态
     */
    const PropertyTreeState& GetState() const { return state_; }

    /**
     * @brief 设置属性树状态
     */
    void SetState(const PropertyTreeState& state) { state_ = state; }

    // =========================================================================
    // 边界
    // =========================================================================

    /**
     * @brief 获取边界
     */
    const SkRect& GetBounds() const { return bounds_; }

    /**
     * @brief 设置边界
     */
    void SetBounds(const SkRect& bounds) { bounds_ = bounds; }

    /**
     * @brief 更新边界（根据所有绘制块计算）
     */
    void UpdateBounds();

    /**
     * @brief 合并边界
     */
    void UnionBounds(const SkRect& rect) {
        if (bounds_.isEmpty()) {
            bounds_ = rect;
        } else {
            bounds_.join(rect);
        }
    }

    // =========================================================================
    // 合成类型
    // =========================================================================

    /**
     * @brief 获取合成类型
     */
    CompositingType GetCompositingType() const { return compositing_type_; }

    /**
     * @brief 设置合成类型
     */
    void SetCompositingType(CompositingType type) { compositing_type_ = type; }

    /**
     * @brief 是否需要独立层
     */
    bool RequiresOwnLayer() const {
        return compositing_type_ != CompositingType::kNone;
    }

    // =========================================================================
    // 合成原因
    // =========================================================================

    /**
     * @brief 获取合成原因
     */
    CompositingReasons GetCompositingReasons() const { return compositing_reasons_; }

    /**
     * @brief 设置合成原因
     */
    void SetCompositingReasons(CompositingReasons reasons) {
        compositing_reasons_ = reasons;
    }

    /**
     * @brief 添加合成原因
     */
    void AddCompositingReason(CompositingReasons reason) {
        compositing_reasons_ |= reason;
    }

    // =========================================================================
    // 合并
    // =========================================================================

    /**
     * @brief 是否可以与另一个待定层合并
     * @param other 另一个待定层
     * @return true 如果可以合并
     */
    bool CanMergeWith(const PendingLayer& other) const;

    /**
     * @brief 合并另一个待定层
     * @param other 另一个待定层
     */
    void MergeWith(PendingLayer&& other);

    // =========================================================================
    // 重叠检测
    // =========================================================================

    /**
     * @brief 检查是否与矩形重叠
     * @param rect 矩形
     * @return true 如果重叠
     */
    bool OverlapsWith(const SkRect& rect) const {
        return SkRect::Intersects(bounds_, rect);
    }

    /**
     * @brief 检查是否与另一个待定层重叠
     * @param other 另一个待定层
     * @return true 如果重叠
     */
    bool OverlapsWith(const PendingLayer& other) const {
        return OverlapsWith(other.bounds_);
    }

private:
    std::vector<const PaintChunk*> chunks_;
    PropertyTreeState state_;
    SkRect bounds_ = SkRect::MakeEmpty();
    CompositingType compositing_type_ = CompositingType::kNone;
    CompositingReasons compositing_reasons_ = CompositingReasons::kNone;
};

} // namespace mbink
