/**
 * @file layer.h
 * @brief 渲染层级系统
 *
 * 功能：
 * - 支持分层渲染
 * - 支持层级合成
 * - 处理 z-index
 * - 优化渲染性能
 */

#pragma once

#include "include/core/SkCanvas.h"
#include "include/core/SkSurface.h"
#include "include/core/SkRect.h"
#include <memory>
#include <vector>
#include <string>

namespace lightui {

// 前向声明
class RenderObject;

/**
 * @brief 层级类型
 */
enum class LayerType {
    NORMAL,      // 普通层级
    TRANSFORM,   // 变换层级（支持 transform）
    OPACITY,     // 透明度层级（支持 opacity）
    CLIP,        // 裁剪层级（支持 clip）
};

/**
 * @brief 渲染层级
 *
 * 表示一个独立的渲染层，可以缓存渲染结果
 */
class Layer {
public:
    /**
     * @brief 构造函数
     * @param type 层级类型
     * @param z_index z-index 值
     */
    explicit Layer(LayerType type = LayerType::NORMAL, int z_index = 0);

    /**
     * @brief 析构函数
     */
    ~Layer() = default;

    // ========== 层级属性 ==========

    /**
     * @brief 获取层级类型
     */
    LayerType GetType() const { return type_; }

    /**
     * @brief 获取 z-index
     */
    int GetZIndex() const { return z_index_; }

    /**
     * @brief 设置 z-index
     */
    void SetZIndex(int z_index) { z_index_ = z_index; }

    /**
     * @brief 获取层级 ID
     */
    const std::string& GetId() const { return id_; }

    /**
     * @brief 设置层级 ID
     */
    void SetId(const std::string& id) { id_ = id; }

    // ========== 层级内容 ==========

    /**
     * @brief 添加渲染对象到层级
     * @param render_object 渲染对象
     */
    void AddRenderObject(std::shared_ptr<RenderObject> render_object);

    /**
     * @brief 移除渲染对象
     * @param render_object 渲染对象
     */
    void RemoveRenderObject(std::shared_ptr<RenderObject> render_object);

    /**
     * @brief 清空所有渲染对象
     */
    void ClearRenderObjects();

    /**
     * @brief 获取所有渲染对象
     */
    const std::vector<std::shared_ptr<RenderObject>>& GetRenderObjects() const {
        return render_objects_;
    }

    // ========== 层级边界 ==========

    /**
     * @brief 获取层级边界
     */
    const SkRect& GetBounds() const { return bounds_; }

    /**
     * @brief 设置层级边界
     */
    void SetBounds(const SkRect& bounds) { bounds_ = bounds; }

    /**
     * @brief 更新层级边界（根据渲染对象计算）
     */
    void UpdateBounds();

    // ========== 层级表面 ==========

    /**
     * @brief 获取层级表面
     */
    SkSurface* GetSurface() const { return surface_.get(); }

    /**
     * @brief 创建层级表面
     * @param width 宽度
     * @param height 高度
     */
    void CreateSurface(int width, int height);

    /**
     * @brief 释放层级表面
     */
    void ReleaseSurface();

    /**
     * @brief 是否有表面
     */
    bool HasSurface() const { return surface_ != nullptr; }

    // ========== 层级状态 ==========

    /**
     * @brief 是否需要重绘
     */
    bool NeedsRepaint() const { return needs_repaint_; }

    /**
     * @brief 标记需要重绘
     */
    void MarkNeedsRepaint() { needs_repaint_ = true; }

    /**
     * @brief 清除重绘标记
     */
    void ClearRepaintFlag() { needs_repaint_ = false; }

    /**
     * @brief 是否可见
     */
    bool IsVisible() const { return visible_; }

    /**
     * @brief 设置可见性
     */
    void SetVisible(bool visible) { visible_ = visible; }

    // ========== 层级属性 ==========

    /**
     * @brief 获取透明度
     */
    float GetOpacity() const { return opacity_; }

    /**
     * @brief 设置透明度
     */
    void SetOpacity(float opacity) {
        opacity_ = opacity;
        MarkNeedsRepaint();
    }

    /**
     * @brief 获取裁剪矩形
     */
    const SkRect& GetClipRect() const { return clip_rect_; }

    /**
     * @brief 设置裁剪矩形
     */
    void SetClipRect(const SkRect& rect) {
        clip_rect_ = rect;
        has_clip_ = true;
        MarkNeedsRepaint();
    }

    /**
     * @brief 清除裁剪
     */
    void ClearClip() {
        has_clip_ = false;
        MarkNeedsRepaint();
    }

    /**
     * @brief 是否有裁剪
     */
    bool HasClip() const { return has_clip_; }

    // ========== 层级渲染 ==========

    /**
     * @brief 绘制层级到画布
     * @param canvas 目标画布
     */
    void Paint(SkCanvas* canvas);

    /**
     * @brief 绘制层级内容到自己的表面
     */
    void PaintToSurface();

private:
    LayerType type_;                                          // 层级类型
    int z_index_;                                             // z-index 值
    std::string id_;                                          // 层级 ID
    std::vector<std::shared_ptr<RenderObject>> render_objects_; // 渲染对象列表
    SkRect bounds_;                                           // 层级边界
    sk_sp<SkSurface> surface_;                                // 层级表面（用于缓存）
    bool needs_repaint_;                                      // 是否需要重绘
    bool visible_;                                            // 是否可见
    float opacity_;                                           // 透明度 (0.0 - 1.0)
    SkRect clip_rect_;                                        // 裁剪矩形
    bool has_clip_;                                           // 是否有裁剪
};

/**
 * @brief 层级管理器
 *
 * 管理所有渲染层级，负责层级排序和合成
 */
class LayerManager {
public:
    /**
     * @brief 构造函数
     */
    LayerManager() = default;

    /**
     * @brief 析构函数
     */
    ~LayerManager() = default;

    // ========== 层级管理 ==========

    /**
     * @brief 创建层级
     * @param type 层级类型
     * @param z_index z-index 值
     * @return 层级指针
     */
    std::shared_ptr<Layer> CreateLayer(LayerType type = LayerType::NORMAL, int z_index = 0);

    /**
     * @brief 添加层级
     * @param layer 层级
     */
    void AddLayer(std::shared_ptr<Layer> layer);

    /**
     * @brief 移除层级
     * @param layer 层级
     */
    void RemoveLayer(std::shared_ptr<Layer> layer);

    /**
     * @brief 根据 ID 查找层级
     * @param id 层级 ID
     * @return 层级指针，未找到返回 nullptr
     */
    std::shared_ptr<Layer> FindLayerById(const std::string& id);

    /**
     * @brief 清空所有层级
     */
    void ClearLayers();

    /**
     * @brief 获取所有层级
     */
    const std::vector<std::shared_ptr<Layer>>& GetLayers() const { return layers_; }

    // ========== 层级排序 ==========

    /**
     * @brief 根据 z-index 排序层级
     */
    void SortLayers();

    // ========== 层级合成 ==========

    /**
     * @brief 合成所有层级到画布
     * @param canvas 目标画布
     */
    void Composite(SkCanvas* canvas);

    /**
     * @brief 合成指定区域的层级
     * @param canvas 目标画布
     * @param region 区域
     */
    void CompositeRegion(SkCanvas* canvas, const SkRect& region);

private:
    std::vector<std::shared_ptr<Layer>> layers_; // 层级列表
};

} // namespace lightui

