/**
 * @file render_pipeline_v2.h
 * @brief 渲染管线 V2 - 分层合成架构的统一入口
 *
 * RenderPipelineV2 整合所有合成器组件：
 * - LayerTreeBuilder: 构建层树
 * - Rasterizer: CPU 光栅化
 * - Compositor: GPU 合成
 * - AnimationLayerBridge: 动画优化
 * - ScrollLayerManager: 滚动优化
 *
 * 设计原则：
 * - 统一的渲染入口
 * - 自动选择最优渲染路径
 * - 支持增量更新
 */

#pragma once

#include "compositor_layer.h"
#include "layer_tree_builder.h"
#include "rasterizer.h"
#include "compositor.h"
#include "animation_layer_bridge.h"
#include "scroll_layer_manager.h"
#include "core/compositor/property_tree/property_trees.h"
#include "core/compositor/property_tree/property_tree_builder.h"
#include "core/compositor/property_tree/paint_artifact_compositor.h"
#include <memory>

// 前向声明 Skia 类
class SkCanvas;
class SkSurface;

namespace lightui {

// 前向声明
class RenderObject;

/**
 * @brief 渲染管线配置
 */
struct RenderPipelineConfig {
    bool enable_gpu_compositing = true;     // 启用 GPU 合成
    bool enable_layer_promotion = true;     // 启用层提升
    bool enable_incremental_rasterize = true;  // 启用增量光栅化
    bool enable_scroll_optimization = true;    // 启用滚动优化
    bool enable_animation_optimization = true; // 启用动画优化
    bool enable_frame_skip = true;          // 启用帧跳过
    bool show_layer_borders = false;        // 显示层边界（调试）
};

/**
 * @brief 渲染帧统计
 */
struct RenderFrameStats {
    // 时间统计（毫秒）
    double layer_tree_build_time = 0.0;
    double rasterize_time = 0.0;
    double composite_time = 0.0;
    double total_time = 0.0;
    
    // 计数统计
    int layers_built = 0;
    int layers_rasterized = 0;
    int layers_composited = 0;
    int dirty_regions_count = 0;
    
    // 状态
    bool frame_skipped = false;
    bool using_gpu = false;
    
    void Reset() {
        layer_tree_build_time = 0.0;
        rasterize_time = 0.0;
        composite_time = 0.0;
        total_time = 0.0;
        layers_built = 0;
        layers_rasterized = 0;
        layers_composited = 0;
        dirty_regions_count = 0;
        frame_skipped = false;
    }
};

/**
 * @brief 渲染管线 V2
 *
 * 分层合成架构的统一渲染入口。
 */
class RenderPipelineV2 {
public:
    RenderPipelineV2();
    ~RenderPipelineV2();

    // 禁止拷贝
    RenderPipelineV2(const RenderPipelineV2&) = delete;
    RenderPipelineV2& operator=(const RenderPipelineV2&) = delete;

    // =========================================================================
    // 初始化
    // =========================================================================

    /**
     * @brief 初始化渲染管线
     * @param width 视口宽度
     * @param height 视口高度
     * @param config 配置选项
     * @return true 如果初始化成功
     */
    bool Initialize(int width, int height, const RenderPipelineConfig& config = {});

    /**
     * @brief 调整视口大小
     * @param width 新宽度
     * @param height 新高度
     */
    void Resize(int width, int height);

    /**
     * @brief 关闭渲染管线
     */
    void Shutdown();

    /**
     * @brief 检查是否已初始化
     */
    bool IsInitialized() const { return initialized_; }

    // =========================================================================
    // 渲染
    // =========================================================================

    /**
     * @brief 渲染一帧
     * @param root 渲染树根节点
     * @return true 如果渲染成功
     *
     * 完整渲染流程：
     * 1. 构建/更新层树
     * 2. 光栅化脏层
     * 3. 合成到屏幕
     */
    bool Render(RenderObject* root);

    /**
     * @brief 渲染到指定 Canvas（用于测试或离屏渲染）
     * @param root 渲染树根节点
     * @param canvas 目标 Canvas
     * @return true 如果渲染成功
     */
    bool RenderToCanvas(RenderObject* root, SkCanvas* canvas);

    /**
     * @brief 标记需要重新渲染
     */
    void MarkNeedsRender() { needs_render_ = true; }

    /**
     * @brief 使层树无效，需要重建
     * 
     * 当渲染树被重建时调用，确保层树中的 RenderObject 指针不会变成悬空指针
     */
    void InvalidateLayerTree() { 
        needs_rebuild_layer_tree_ = true; 
        needs_render_ = true;
    }

    /**
     * @brief 强制重新光栅化所有层
     * 
     * 在有活动动画时调用，确保层内容被正确更新
     */
    void ForceRasterize() {
        if (root_layer_) {
            root_layer_->MarkFullDirty();
        }
        needs_render_ = true;
    }

    /**
     * @brief 检查是否需要渲染
     */
    bool NeedsRender() const { return needs_render_; }

    // =========================================================================
    // 滚动处理
    // =========================================================================

    /**
     * @brief 处理滚动事件
     * @param container 滚动容器
     * @param delta_x X 方向滚动增量
     * @param delta_y Y 方向滚动增量
     * @return true 如果滚动成功
     *
     * 滚动时只更新层偏移，不重新光栅化
     */
    bool HandleScroll(RenderObject* container, float delta_x, float delta_y);

    /**
     * @brief 滚动到指定位置
     * @param container 滚动容器
     * @param scroll_x 目标 X 滚动位置
     * @param scroll_y 目标 Y 滚动位置
     * @return true 如果滚动成功
     */
    bool ScrollTo(RenderObject* container, float scroll_x, float scroll_y);

    // =========================================================================
    // 动画处理
    // =========================================================================

    /**
     * @brief 开始动画帧
     *
     * 在动画更新前调用，准备批处理
     */
    void BeginAnimationFrame();

    /**
     * @brief 更新动画属性
     * @param object 渲染对象
     * @param property 属性名
     * @param value 属性值
     * @return 更新类型
     */
    AnimationUpdateType UpdateAnimationProperty(RenderObject* object,
                                                 const std::string& property,
                                                 const std::string& value);

    /**
     * @brief 结束动画帧
     * @return true 如果有层属性更新（需要合成）
     */
    bool EndAnimationFrame();

    /**
     * @brief 通知动画开始
     */
    void OnAnimationStart(RenderObject* object,
                          const std::string& animation_name,
                          const std::vector<std::string>& properties);

    /**
     * @brief 通知动画结束
     */
    void OnAnimationEnd(RenderObject* object, const std::string& animation_name);

    // =========================================================================
    // 脏区域管理
    // =========================================================================

    /**
     * @brief 标记渲染对象为脏
     * @param object 渲染对象
     */
    void MarkDirty(RenderObject* object);

    /**
     * @brief 标记区域为脏
     * @param region 脏区域（屏幕坐标）
     */
    void MarkDirtyRegion(const SkRect& region);

    // =========================================================================
    // 配置
    // =========================================================================

    /**
     * @brief 获取当前配置
     */
    const RenderPipelineConfig& GetConfig() const { return config_; }

    /**
     * @brief 更新配置
     */
    void SetConfig(const RenderPipelineConfig& config);

    /**
     * @brief 设置是否显示层边界
     */
    void SetShowLayerBorders(bool show);

    /**
     * @brief 设置 DPI 缩放比
     * @param scale DPI 缩放比
     */
    void SetDpiScale(float scale);

    /**
     * @brief 获取 DPI 缩放比
     */
    float GetDpiScale() const { return dpi_scale_; }

    // =========================================================================
    // 统计
    // =========================================================================

    /**
     * @brief 获取上一帧统计
     */
    const RenderFrameStats& GetLastFrameStats() const { return last_frame_stats_; }

    /**
     * @brief 重置统计
     */
    void ResetStats();

    // =========================================================================
    // 组件访问（用于测试和调试）
    // =========================================================================

    LayerTreeBuilder* GetLayerTreeBuilder() { return layer_tree_builder_.get(); }
    Rasterizer* GetRasterizer() { return rasterizer_.get(); }
    Compositor* GetCompositor() { return compositor_.get(); }
    AnimationLayerBridge* GetAnimationBridge() { return animation_bridge_.get(); }
    ScrollLayerManager* GetScrollManager() { return scroll_manager_.get(); }

    /**
     * @brief 获取层树根节点
     */
    std::shared_ptr<CompositorLayer> GetRootLayer() const { return root_layer_; }

    // =========================================================================
    // 属性树系统
    // =========================================================================

    /**
     * @brief 设置是否使用属性树系统
     */
    void SetUsePropertyTreeSystem(bool use) { use_property_tree_system_ = use; }

    /**
     * @brief 检查是否使用属性树系统
     */
    bool IsUsingPropertyTreeSystem() const { return use_property_tree_system_; }

    /**
     * @brief 获取属性树集合
     */
    PropertyTrees* GetPropertyTrees() { return property_trees_.get(); }

    /**
     * @brief 获取属性树构建器
     */
    PropertyTreeBuilder* GetPropertyTreeBuilder() { return property_tree_builder_.get(); }

    /**
     * @brief 获取绘制产物合成器
     */
    PaintArtifactCompositor* GetPaintArtifactCompositor() { return paint_artifact_compositor_.get(); }

private:
    /**
     * @brief 构建或更新层树
     * @param root 渲染树根节点
     */
    void BuildLayerTree(RenderObject* root);

    /**
     * @brief 光栅化脏层
     */
    void RasterizeDirtyLayers();

    /**
     * @brief 合成层到屏幕
     * @return true 如果合成成功
     */
    bool CompositeLayers();

    /**
     * @brief 合成层到 Canvas
     * @param canvas 目标 Canvas
     * @return true 如果合成成功
     */
    bool CompositeLayersToCanvas(SkCanvas* canvas);

    /**
     * @brief 注册滚动容器和固定元素
     * @param root 渲染树根节点
     */
    void RegisterScrollableElements(RenderObject* root);

    /**
     * @brief 递归注册滚动容器和固定元素
     */
    void RegisterScrollableElementsRecursive(RenderObject* obj);

    /**
     * @brief 更新层树边界（不重建层树）
     * @param layer 层节点
     */
    void UpdateLayerTreeBounds(CompositorLayer* layer);

    /**
     * @brief 递归检查渲染对象是否需要重绘
     * @param obj 渲染对象
     * @return true 如果对象或其子对象需要重绘
     */
    bool CheckRenderObjectNeedsPaint(RenderObject* obj);

    // 状态
    bool initialized_ = false;
    bool needs_render_ = true;
    bool needs_rebuild_layer_tree_ = true;  // 是否需要重建层树
    int viewport_width_ = 0;
    int viewport_height_ = 0;

    // 配置
    RenderPipelineConfig config_;

    // 组件
    std::unique_ptr<LayerTreeBuilder> layer_tree_builder_;
    std::unique_ptr<Rasterizer> rasterizer_;
    std::unique_ptr<Compositor> compositor_;
    std::unique_ptr<AnimationLayerBridge> animation_bridge_;
    std::unique_ptr<ScrollLayerManager> scroll_manager_;

    // 层树
    std::shared_ptr<CompositorLayer> root_layer_;

    // 属性树系统
    bool use_property_tree_system_ = true;
    std::unique_ptr<PropertyTrees> property_trees_;
    std::unique_ptr<PropertyTreeBuilder> property_tree_builder_;
    std::unique_ptr<PaintArtifactCompositor> paint_artifact_compositor_;

    // 统计
    RenderFrameStats last_frame_stats_;
    RenderFrameStats current_frame_stats_;

    // 计时
    double frame_start_time_ = 0.0;

    // DPI 缩放
    float dpi_scale_ = 1.0f;
};

} // namespace lightui
