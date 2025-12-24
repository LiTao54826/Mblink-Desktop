/**
 * @file compositor_layer.h
 * @brief 合成层 - 分层合成架构的核心组件
 *
 * CompositorLayer 表示一个独立的合成层，包含：
 * - CPU 位图（用于光栅化）
 * - GPU 纹理（用于合成）
 * - 变换矩阵和透明度（用于动画）
 * - 脏区域跟踪（用于增量更新）
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
#include <vector>
#include "include/core/SkBitmap.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkMatrix.h"
#include "include/core/SkRect.h"
#include "include/core/SkPoint.h"

// 前向声明 OpenGL 类型
typedef unsigned int GLuint;

namespace lightui {

// 前向声明
class RenderObject;

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

private:
    // 层标识
    uint32_t id_;
    std::string debug_name_;

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

    // 静态 ID 生成器
    static uint32_t next_id_;
};

/**
 * @brief 创建新的合成层
 * @return 新层的共享指针
 */
std::shared_ptr<CompositorLayer> CreateCompositorLayer();

} // namespace lightui
