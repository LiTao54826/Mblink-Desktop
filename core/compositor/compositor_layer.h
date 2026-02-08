/**
 * @file compositor_layer.h
 * @brief 合成层 - 分层合成架构的核心组件
 *
 * CompositorLayer 表示一个独立的合成层，包含：
 * - CPU 位图（用于光栅化）
 * - GPU 纹理（用于合成）
 * - 变换矩阵和透明度（用于动画）
 * - 脏区域跟踪（用于增量更新）
 * - PropertyTreeState（属性树状态引用）
 * - PaintChunk 引用（绘制块）
 *
 * 层提升条件：
 * - will-change: transform/opacity
 * - position: fixed
 * - CSS 动画（transform/opacity）
 * - 可滚动容器
 */

#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>
#include "include/core/SkBitmap.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkMatrix.h"
#include "include/core/SkRect.h"
#include "include/core/SkPoint.h"
#include "core/compositor/property_tree/property_tree_state.h"
#include "core/compositor/property_tree/compositing_reasons.h"
#include "core/compositor/animation/animation_bounds_calculator.h"

// 前向声明 OpenGL 类型
typedef unsigned int GLuint;

namespace lightui {

// 前向声明
class RenderObject;
class PaintChunk;

/**
 * @brief 层提升原因
 */
enum class LayerPromotionReason {
    None,                   // 无（不需要独立层）
    WillChangeTransform,    // will-change: transform
    WillChangeOpacity,      // will-change: opacity
    PositionFixed,          // position: fixed
    TransformAnimation,     // CSS transform 动画
    OpacityAnimation,       // CSS opacity 动画
    ScrollableContent,      // 可滚动内容
    HighZIndex,             // 高 z-index（如下拉菜单、弹出层）
    Explicit,               // 显式请求（调试用）
    RootLayer               // 根层
};

/**
 * @brief 合成层类
 *
 * 每个合成层拥有独立的 CPU 位图和 GPU 纹理，
 * 可以独立更新和变换，实现高效的增量渲染。
 */
class CompositorLayer : public std::enable_shared_from_this<CompositorLayer> {
public:
    /**
     * @brief 构造函数
     * @param id 层唯一标识符
     */
    explicit CompositorLayer(uint32_t id);

    /**
     * @brief 析构函数
     */
    ~CompositorLayer();

    // 禁止拷贝
    CompositorLayer(const CompositorLayer&) = delete;
    CompositorLayer& operator=(const CompositorLayer&) = delete;

    // =========================================================================
    // 基本属性
    // =========================================================================

    /**
     * @brief 获取层 ID
     */
    uint32_t GetId() const { return id_; }

    /**
     * @brief 获取关联的 RenderObject
     */
    RenderObject* GetRenderObject() const { return render_object_; }

    /**
     * @brief 设置关联的 RenderObject
     */
    void SetRenderObject(RenderObject* obj) { render_object_ = obj; }

    /**
     * @brief 获取层提升原因
     */
    LayerPromotionReason GetPromotionReason() const { return promotion_reason_; }

    /**
     * @brief 设置层提升原因
     */
    void SetPromotionReason(LayerPromotionReason reason) { promotion_reason_ = reason; }

    // =========================================================================
    // 边界和变换
    // =========================================================================

    /**
     * @brief 获取层边界（相对于父层）
     */
    const SkRect& GetBounds() const { return bounds_; }

    /**
     * @brief 设置层边界
     * @param bounds 新边界
     * @note 如果边界大小改变，会触发位图重新分配
     */
    void SetBounds(const SkRect& bounds);

    /**
     * @brief 获取变换矩阵
     */
    const SkMatrix& GetTransform() const { return transform_; }

    /**
     * @brief 设置变换矩阵（用于动画）
     * @param transform 新变换矩阵
     * @note 不会触发重新光栅化
     */
    void SetTransform(const SkMatrix& transform) { transform_ = transform; }

    /**
     * @brief 获取透明度
     */
    float GetOpacity() const { return opacity_; }

    /**
     * @brief 设置透明度（用于动画）
     * @param opacity 新透明度 [0.0, 1.0]
     * @note 不会触发重新光栅化
     */
    void SetOpacity(float opacity) { opacity_ = opacity; }

    /**
     * @brief box-shadow 扩展范围结构
     *
     * 分别记录四个方向的阴影扩展距离，相比单一的 shadow_extent 更精确，
     * 可以节省内存（特别是对于有偏移的阴影）。
     */
    struct ShadowExtent {
        float left = 0.0f;    ///< 向左扩展的距离
        float right = 0.0f;   ///< 向右扩展的距离
        float top = 0.0f;     ///< 向上扩展的距离
        float bottom = 0.0f;  ///< 向下扩展的距离

        /**
         * @brief 检查是否有任何扩展
         */
        bool HasExtent() const {
            return left > 0 || right > 0 || top > 0 || bottom > 0;
        }

        /**
         * @brief 比较运算符
         */
        bool operator==(const ShadowExtent& other) const {
            return left == other.left && right == other.right &&
                   top == other.top && bottom == other.bottom;
        }

        bool operator!=(const ShadowExtent& other) const {
            return !(*this == other);
        }
    };

    /**
     * @brief 获取 box-shadow 扩展范围
     * @return shadow extent 结构
     */
    const ShadowExtent& GetShadowExtent() const { return shadow_extent_; }

    /**
     * @brief 设置 box-shadow 扩展范围
     * @param extent shadow extent 结构
     * @note 会触发位图重新分配
     */
    void SetShadowExtent(const ShadowExtent& extent) {
        if (shadow_extent_ != extent) {
            shadow_extent_ = extent;
            bitmap_valid_ = false;  // 触发重新分配
        }
    }

    // =========================================================================
    // CPU 位图管理
    // =========================================================================

    /**
     * @brief 获取 CPU 位图
     */
    SkBitmap& GetBitmap() { return bitmap_; }
    const SkBitmap& GetBitmap() const { return bitmap_; }

    /**
     * @brief 获取用于光栅化的 Canvas
     * @return Canvas 指针，如果位图未初始化则返回 nullptr
     */
    SkCanvas* GetCanvas();

    /**
     * @brief 确保位图已分配
     * @return true 如果位图可用
     */
    bool EnsureBitmap();

    /**
     * @brief 释放位图资源
     */
    void ReleaseBitmap();

    // =========================================================================
    // 脏区域管理
    // =========================================================================

    /**
     * @brief 标记脏区域
     * @param region 需要重绘的区域（层坐标系）
     */
    void MarkDirty(const SkRect& region);

    /**
     * @brief 标记整个层为脏
     */
    void MarkFullDirty();

    /**
     * @brief 清除所有脏区域
     */
    void ClearDirtyRegions();

    /**
     * @brief 检查是否有脏区域
     */
    bool HasDirtyRegions() const { return !dirty_regions_.empty(); }

    /**
     * @brief 获取脏区域列表
     */
    const std::vector<SkIRect>& GetDirtyRegions() const { return dirty_regions_; }

    /**
     * @brief 合并重叠的脏区域
     * @note 减少绘制调用次数
     */
    void MergeDirtyRegions();

    // =========================================================================
    // GPU 纹理管理
    // =========================================================================

    /**
     * @brief 获取 GPU 纹理 ID
     */
    GLuint GetTextureId() const { return texture_id_; }

    /**
     * @brief 检查纹理是否需要更新
     */
    bool IsTextureDirty() const { return !texture_dirty_regions_.empty(); }

    /**
     * @brief 标记纹理脏区域
     * @param region 需要上传的区域
     */
    void MarkTextureDirty(const SkIRect& region);

    /**
     * @brief 上传脏区域到 GPU 纹理
     * @return true 如果上传成功
     */
    bool UploadDirtyRegions();

    /**
     * @brief 创建 GPU 纹理
     * @return true 如果创建成功
     */
    bool CreateTexture();

    /**
     * @brief 销毁 GPU 纹理
     */
    void DestroyTexture();

    /**
     * @brief 检查是否有有效的 GPU 纹理
     */
    bool HasTexture() const { return texture_id_ != 0; }

    // =========================================================================
    // 父子关系
    // =========================================================================

    /**
     * @brief 获取父层
     */
    std::shared_ptr<CompositorLayer> GetParent() const { return parent_.lock(); }

    /**
     * @brief 设置父层
     */
    void SetParent(std::shared_ptr<CompositorLayer> parent) { parent_ = parent; }

    /**
     * @brief 添加子层
     */
    void AddChild(std::shared_ptr<CompositorLayer> child);

    /**
     * @brief 移除子层
     */
    void RemoveChild(CompositorLayer* child);

    /**
     * @brief 移除所有子层
     */
    void RemoveAllChildren();

    /**
     * @brief 获取子层列表
     */
    const std::vector<std::shared_ptr<CompositorLayer>>& GetChildren() const { return children_; }

    // =========================================================================
    // 属性树状态（Property Tree System）
    // =========================================================================

    /**
     * @brief 获取属性树状态
     */
    const PropertyTreeState& GetPropertyTreeState() const { return property_tree_state_; }

    /**
     * @brief 设置属性树状态
     */
    void SetPropertyTreeState(const PropertyTreeState& state) { property_tree_state_ = state; }

    /**
     * @brief 获取合成原因
     */
    CompositingReasons GetCompositingReasons() const { return compositing_reasons_; }

    /**
     * @brief 设置合成原因
     */
    void SetCompositingReasons(CompositingReasons reasons) { compositing_reasons_ = reasons; }

    /**
     * @brief 添加合成原因
     */
    void AddCompositingReason(CompositingReasons reason) { 
        compositing_reasons_ = compositing_reasons_ | reason; 
    }

    // =========================================================================
    // 绘制块引用（Paint Chunks）
    // =========================================================================

    /**
     * @brief 获取关联的绘制块
     */
    const std::vector<const PaintChunk*>& GetPaintChunks() const { return paint_chunks_; }

    /**
     * @brief 设置关联的绘制块
     */
    void SetPaintChunks(std::vector<const PaintChunk*> chunks) { 
        paint_chunks_ = std::move(chunks); 
    }

    /**
     * @brief 添加绘制块
     */
    void AddPaintChunk(const PaintChunk* chunk) { 
        if (chunk) paint_chunks_.push_back(chunk); 
    }

    /**
     * @brief 清除绘制块引用
     */
    void ClearPaintChunks() { paint_chunks_.clear(); }

    /**
     * @brief 检查是否有绘制块
     */
    bool HasPaintChunks() const { return !paint_chunks_.empty(); }

    // =========================================================================
    // 增量光栅化支持
    // =========================================================================

    /**
     * @brief 获取光栅化脏区域（层坐标系）
     */
    const std::vector<SkRect>& GetRasterDirtyRects() const { return raster_dirty_rects_; }

    /**
     * @brief 添加光栅化脏区域
     */
    void AddRasterDirtyRect(const SkRect& rect);

    /**
     * @brief 清除光栅化脏区域
     */
    void ClearRasterDirtyRects() { raster_dirty_rects_.clear(); }

    /**
     * @brief 检查是否需要光栅化
     */
    bool NeedsRasterization() const { return !raster_dirty_rects_.empty() || needs_full_raster_; }

    /**
     * @brief 标记需要完整光栅化
     */
    void MarkNeedsFullRaster() { needs_full_raster_ = true; }

    /**
     * @brief 清除完整光栅化标记
     */
    void ClearNeedsFullRaster() { needs_full_raster_ = false; }

    /**
     * @brief 检查是否需要完整光栅化
     */
    bool NeedsFullRaster() const { return needs_full_raster_; }

    // =========================================================================
    // 滚动支持
    // =========================================================================

    /**
     * @brief 获取滚动偏移
     */
    const SkPoint& GetScrollOffset() const { return scroll_offset_; }

    /**
     * @brief 设置滚动偏移
     * @param offset 新滚动偏移
     * @note 不会触发重新光栅化
     */
    void SetScrollOffset(const SkPoint& offset) { scroll_offset_ = offset; }

    /**
     * @brief 滚动指定距离
     */
    void ScrollBy(float dx, float dy);

    // =========================================================================
    // DPI 缩放支持
    // =========================================================================

    /**
     * @brief 获取 DPI 缩放比
     */
    float GetDpiScale() const { return dpi_scale_; }

    /**
     * @brief 设置 DPI 缩放比
     * @param scale DPI 缩放比（例如 2.0 表示 Retina 显示器）
     * @note 会触发位图重新分配
     */
    void SetDpiScale(float scale);

    // =========================================================================
    // 动画边界支持
    // =========================================================================

    /**
     * @brief 获取动画边界信息
     * @return 动画边界指针，如果没有则返回 nullptr
     */
    const AnimationBounds* GetAnimationBounds() const;

    /**
     * @brief 设置动画边界信息
     * @param bounds 动画边界
     */
    void SetAnimationBounds(const AnimationBounds& bounds);

    /**
     * @brief 清除动画边界信息
     */
    void ClearAnimationBounds();

    /**
     * @brief 检查是否有动画边界
     */
    bool HasAnimationBounds() const;

    // =========================================================================
    // 调试支持
    // =========================================================================

    /**
     * @brief 获取层名称（调试用）
     */
    const std::string& GetDebugName() const { return debug_name_; }

    /**
     * @brief 设置层名称（调试用）
     */
    void SetDebugName(const std::string& name) { debug_name_ = name; }

    /**
     * @brief 获取层提升原因的字符串描述
     */
    static const char* PromotionReasonToString(LayerPromotionReason reason);

    // =========================================================================
    // 增量更新支持
    // =========================================================================

    /**
     * @brief 获取层的唯一标识（用于增量更新时识别层）
     *
     * 与 GetId() 的区别：
     * - layer_identity_ 在层的整个生命周期内不变
     * - 即使层被重新创建，只要是同一个 RenderObject，identity 应该相同
     */
    uint64_t GetLayerIdentity() const { return layer_identity_; }

    /**
     * @brief 设置层的唯一标识
     */
    void SetLayerIdentity(uint64_t identity) { layer_identity_ = identity; }

    /**
     * @brief 检查层是否为 fixed 层
     */
    bool IsFixedLayer() const {
        return promotion_reason_ == LayerPromotionReason::PositionFixed;
    }

    /**
     * @brief 获取层在层树中的深度
     * @return 深度值，根层为 0
     */
    int GetTreeDepth() const;

    /**
     * @brief 重新附加到新的父层
     * @param new_parent 新的父层
     *
     * 操作步骤：
     * 1. 从当前父层移除
     * 2. 添加到新父层
     * 3. 更新 parent_ 引用
     */
    void ReparentTo(std::shared_ptr<CompositorLayer> new_parent);

    /**
     * @brief 按 z-index 插入子层
     * @param child 要插入的子层
     * @param z_index z-index 值
     *
     * 保持子层按 z-index 升序排列
     */
    void InsertChildByZIndex(std::shared_ptr<CompositorLayer> child, int z_index);

    /**
     * @brief 获取关联 RenderObject 的 z-index
     * @return z-index 值，默认为 0
     */
    int GetZIndex() const;

private:
    // 层标识
    uint32_t id_;
    std::string debug_name_;

    // 层的唯一标识，在层的生命周期内不变（用于增量更新）
    uint64_t layer_identity_ = 0;
    static uint64_t next_layer_identity_;

    // 关联的渲染对象
    RenderObject* render_object_ = nullptr;
    LayerPromotionReason promotion_reason_ = LayerPromotionReason::None;

    // 边界和变换
    SkRect bounds_ = SkRect::MakeEmpty();
    SkMatrix transform_ = SkMatrix::I();
    float opacity_ = 1.0f;

    // CPU 位图
    SkBitmap bitmap_;
    std::unique_ptr<SkCanvas> canvas_;
    bool bitmap_valid_ = false;

    // GPU 纹理
    GLuint texture_id_ = 0;
    int texture_width_ = 0;
    int texture_height_ = 0;

    // 脏区域
    std::vector<SkIRect> dirty_regions_;
    std::vector<SkIRect> texture_dirty_regions_;

    // 父子关系
    std::weak_ptr<CompositorLayer> parent_;
    std::vector<std::shared_ptr<CompositorLayer>> children_;

    // 滚动
    SkPoint scroll_offset_ = {0, 0};

    // DPI 缩放
    float dpi_scale_ = 1.0f;

    // 属性树状态
    PropertyTreeState property_tree_state_;
    CompositingReasons compositing_reasons_ = CompositingReasons::kNone;

    // 绘制块引用
    std::vector<const PaintChunk*> paint_chunks_;

    // 增量光栅化
    std::vector<SkRect> raster_dirty_rects_;
    bool needs_full_raster_ = true;

    // 动画边界（用于扩展层边界以容纳动画）
    std::optional<AnimationBounds> animation_bounds_;

    // 🐛 修复：box-shadow 扩展范围（用于扩展 bitmap 以容纳阴影）
    // 优化：使用四个方向独立的扩展值，节省内存并提高精确度
    ShadowExtent shadow_extent_;

    // 静态 ID 生成器
    static uint32_t next_id_;
};

/**
 * @brief 创建新的合成层
 * @return 新层的共享指针
 */
std::shared_ptr<CompositorLayer> CreateCompositorLayer();

} // namespace lightui
