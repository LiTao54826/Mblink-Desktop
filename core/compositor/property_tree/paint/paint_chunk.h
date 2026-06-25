/**
 * @file paint_chunk.h
 * @brief 绘制块
 *
 * 绘制块是具有相同属性树状态的连续绘制指令集合：
 * - 属性树状态
 * - 绘制指令范围
 * - 边界
 * - 光栅化失效区域
 *
 * 参考 Chromium Blink: platform/graphics/paint/paint_chunk.h
 */

#pragma once

#include "core/compositor/property_tree/property_tree_state.h"
#include "core/compositor/property_tree/compositing_reasons.h"
#include "include/core/SkRect.h"
#include <vector>
#include <cstdint>

namespace mblink {

// 前向声明
class RenderObject;

/**
 * @brief 绘制块
 *
 * 具有相同属性树状态的连续绘制指令集合。
 * 用于层化决策和光栅化失效计算。
 */
class PaintChunk {
public:
    PaintChunk() = default;
    ~PaintChunk() = default;

    // 支持移动
    PaintChunk(PaintChunk&&) = default;
    PaintChunk& operator=(PaintChunk&&) = default;
    
    // 支持拷贝
    PaintChunk(const PaintChunk&) = default;
    PaintChunk& operator=(const PaintChunk&) = default;

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
    // 绘制指令范围
    // =========================================================================

    /**
     * @brief 获取开始索引
     */
    size_t GetBeginIndex() const { return begin_index_; }

    /**
     * @brief 获取结束索引
     */
    size_t GetEndIndex() const { return end_index_; }

    /**
     * @brief 设置范围
     */
    void SetRange(size_t begin, size_t end) {
        begin_index_ = begin;
        end_index_ = end;
    }

    /**
     * @brief 获取绘制指令数量
     */
    size_t GetItemCount() const { return end_index_ - begin_index_; }

    /**
     * @brief 是否为空
     */
    bool IsEmpty() const { return begin_index_ >= end_index_; }

    // =========================================================================
    // 边界
    // =========================================================================

    /**
     * @brief 获取边界（在本地坐标系中）
     */
    const SkRect& GetBounds() const { return bounds_; }

    /**
     * @brief 设置边界
     */
    void SetBounds(const SkRect& bounds) { bounds_ = bounds; }

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
    // 合并
    // =========================================================================

    /**
     * @brief 是否可以与另一个块合并
     * @param other 另一个绘制块
     * @return true 如果可以合并
     */
    bool CanMergeWith(const PaintChunk& other) const;

    /**
     * @brief 合并另一个块
     * @param other 另一个绘制块
     */
    void MergeWith(const PaintChunk& other);

    // =========================================================================
    // 光栅化失效
    // =========================================================================

    /**
     * @brief 获取光栅化失效区域
     */
    const std::vector<SkRect>& GetRasterInvalidationRects() const {
        return raster_invalidation_rects_;
    }

    /**
     * @brief 添加光栅化失效区域
     */
    void AddRasterInvalidationRect(const SkRect& rect) {
        raster_invalidation_rects_.push_back(rect);
    }

    /**
     * @brief 清除光栅化失效区域
     */
    void ClearRasterInvalidationRects() {
        raster_invalidation_rects_.clear();
    }

    /**
     * @brief 是否有光栅化失效
     */
    bool HasRasterInvalidation() const {
        return !raster_invalidation_rects_.empty();
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

    /**
     * @brief 是否有合成原因
     */
    bool HasCompositingReasons() const {
        return compositing_reasons_ != CompositingReasons::kNone;
    }

    // =========================================================================
    // 关联对象
    // =========================================================================

    /**
     * @brief 获取关联的 RenderObject（主要用于调试）
     */
    RenderObject* GetRenderObject() const { return render_object_; }

    /**
     * @brief 设置关联的 RenderObject
     */
    void SetRenderObject(RenderObject* obj) { render_object_ = obj; }

    // =========================================================================
    // 标识符
    // =========================================================================

    /**
     * @brief 获取唯一标识符
     */
    uint64_t GetId() const { return id_; }

    /**
     * @brief 分配新的 ID
     */
    void AssignId() { id_ = next_id_++; }

private:
    PropertyTreeState state_;
    size_t begin_index_ = 0;
    size_t end_index_ = 0;
    SkRect bounds_ = SkRect::MakeEmpty();
    std::vector<SkRect> raster_invalidation_rects_;
    CompositingReasons compositing_reasons_ = CompositingReasons::kNone;
    RenderObject* render_object_ = nullptr;
    uint64_t id_ = 0;
    
    static uint64_t next_id_;
};

} // namespace mblink
