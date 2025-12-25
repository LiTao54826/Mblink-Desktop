/**
 * @file paint_artifact.h
 * @brief 绘制产物
 *
 * 绘制产物包含所有绘制指令和绘制块：
 * - DisplayItems 列表
 * - PaintChunks 列表
 * - 变化检测
 *
 * 参考 Chromium Blink: platform/graphics/paint/paint_artifact.h
 */

#pragma once

#include "core/compositor/property_tree/display_item.h"
#include "core/compositor/property_tree/paint_chunk.h"
#include <vector>

namespace lightui {

/**
 * @brief 绘制产物变化信息
 */
struct PaintArtifactChangeInfo {
    std::vector<size_t> added_chunks;      // 新增的块索引
    std::vector<size_t> removed_chunks;    // 移除的块索引（在旧产物中）
    std::vector<size_t> modified_chunks;   // 修改的块索引
    bool property_trees_changed = false;   // 属性树是否变化
    
    /**
     * @brief 是否有任何变化
     */
    bool HasAnyChange() const {
        return !added_chunks.empty() || 
               !removed_chunks.empty() || 
               !modified_chunks.empty() ||
               property_trees_changed;
    }
    
    /**
     * @brief 是否需要完整更新
     */
    bool NeedsFullUpdate() const {
        return property_trees_changed || 
               !added_chunks.empty() || 
               !removed_chunks.empty();
    }
};

/**
 * @brief 绘制产物
 *
 * 包含一帧的所有绘制指令和绘制块。
 * 用于层化和光栅化。
 */
class PaintArtifact {
public:
    PaintArtifact() = default;
    ~PaintArtifact() = default;

    // 支持移动
    PaintArtifact(PaintArtifact&&) = default;
    PaintArtifact& operator=(PaintArtifact&&) = default;
    
    // 禁止拷贝（太大了）
    PaintArtifact(const PaintArtifact&) = delete;
    PaintArtifact& operator=(const PaintArtifact&) = delete;

    // =========================================================================
    // 绘制指令
    // =========================================================================

    /**
     * @brief 获取绘制指令列表（只读）
     */
    const std::vector<DisplayItem>& GetDisplayItems() const { return display_items_; }

    /**
     * @brief 获取绘制指令列表（可修改）
     */
    std::vector<DisplayItem>& GetDisplayItems() { return display_items_; }

    /**
     * @brief 添加绘制指令
     */
    void AppendDisplayItem(DisplayItem item);

    /**
     * @brief 获取绘制指令数量
     */
    size_t GetDisplayItemCount() const { return display_items_.size(); }

    // =========================================================================
    // 绘制块
    // =========================================================================

    /**
     * @brief 获取绘制块列表（只读）
     */
    const std::vector<PaintChunk>& GetPaintChunks() const { return paint_chunks_; }

    /**
     * @brief 获取绘制块列表（可修改）
     */
    std::vector<PaintChunk>& GetPaintChunks() { return paint_chunks_; }

    /**
     * @brief 获取绘制块数量
     */
    size_t GetPaintChunkCount() const { return paint_chunks_.size(); }

    // =========================================================================
    // 块管理
    // =========================================================================

    /**
     * @brief 开始新的绘制块
     * @param state 属性树状态
     */
    void StartNewChunk(const PropertyTreeState& state);

    /**
     * @brief 结束当前绘制块
     */
    void FinishCurrentChunk();

    /**
     * @brief 获取当前块
     */
    PaintChunk* GetCurrentChunk();

    /**
     * @brief 是否有当前块
     */
    bool HasCurrentChunk() const { return has_current_chunk_; }

    // =========================================================================
    // 状态
    // =========================================================================

    /**
     * @brief 清除所有内容
     */
    void Clear();

    /**
     * @brief 是否为空
     */
    bool IsEmpty() const { return display_items_.empty(); }

    // =========================================================================
    // 变化检测
    // =========================================================================

    /**
     * @brief 与上一帧比较，计算变化
     * @param previous 上一帧的绘制产物
     * @return 变化信息
     */
    PaintArtifactChangeInfo ComputeChanges(const PaintArtifact& previous) const;

    // =========================================================================
    // 调试
    // =========================================================================

    /**
     * @brief 获取内存使用量（字节）
     */
    size_t GetMemoryUsage() const;

private:
    std::vector<DisplayItem> display_items_;
    std::vector<PaintChunk> paint_chunks_;
    size_t current_chunk_begin_ = 0;
    bool has_current_chunk_ = false;
};

} // namespace lightui
