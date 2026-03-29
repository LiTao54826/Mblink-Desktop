/**
 * @file compositor.h
 * @brief GPU 合成器 - 将多个层合成到屏幕
 *
 * Compositor 负责：
 * - GPU 初始化（着色器、VAO/VBO）
 * - 层合成（按 z-order 渲染纹理）
 * - CPU 回退合成（GPU 不可用时）
 * - 帧跳过优化（无变化时跳过合成）
 *
 * 设计原则：
 * - GPU 加速合成
 * - 支持变换和透明度动画
 * - 自动回退到 CPU 合成
 */

#pragma once

#include "compositor_layer.h"
#include "include/core/SkBitmap.h"
#include <memory>
#include <vector>

// 前向声明 Skia 类
class SkCanvas;
class SkSurface;

namespace mbink {

/**
 * @brief 合成统计信息
 */
struct CompositeStats {
    int frames_composited = 0;      // 合成的帧数
    int frames_skipped = 0;         // 跳过的帧数（无变化）
    int layers_composited = 0;      // 合成的层数
    int textures_uploaded = 0;      // 上传的纹理数
    double composite_time_ms = 0.0; // 合成耗时（毫秒）
    bool using_gpu = false;         // 是否使用 GPU

    void Reset() {
        frames_composited = 0;
        frames_skipped = 0;
        layers_composited = 0;
        textures_uploaded = 0;
        composite_time_ms = 0.0;
    }
};

/**
 * @brief 合成帧信息（用于性能分析）
 */
struct CompositorFrameInfo {
    double rasterize_time_ms = 0.0;  // 光栅化耗时
    double upload_time_ms = 0.0;     // 纹理上传耗时
    double composite_time_ms = 0.0;  // 合成耗时
    double total_time_ms = 0.0;      // 总耗时
    int layers_rasterized = 0;       // 光栅化的层数
    int layers_composited = 0;       // 合成的层数
    bool frame_skipped = false;      // 是否跳过
};

/**
 * @brief GPU 合成器类
 *
 * 将多个合成层合成到屏幕，支持 GPU 加速和 CPU 回退。
 */
class Compositor {
public:
    Compositor();
    ~Compositor();

    // 禁止拷贝
    Compositor(const Compositor&) = delete;
    Compositor& operator=(const Compositor&) = delete;

    // =========================================================================
    // 初始化
    // =========================================================================

    /**
     * @brief 初始化 GPU 资源
     * @param width 视口宽度
     * @param height 视口高度
     * @return true 如果初始化成功
     */
    bool Initialize(int width, int height);

    /**
     * @brief 调整视口大小
     * @param width 新宽度
     * @param height 新高度
     */
    void Resize(int width, int height);

    /**
     * @brief 释放 GPU 资源
     */
    void Shutdown();

    /**
     * @brief 检查是否已初始化
     */
    bool IsInitialized() const { return initialized_; }

    /**
     * @brief 检查是否使用 GPU
     */
    bool IsUsingGPU() const { return use_gpu_; }

    // =========================================================================
    // 合成
    // =========================================================================

    /**
     * @brief 合成层树到屏幕
     * @param root 层树根节点
     * @return true 如果合成成功
     */
    bool Composite(CompositorLayer* root);

    /**
     * @brief 合成层树到指定 Canvas（CPU 模式）
     * @param root 层树根节点
     * @param canvas 目标 Canvas
     * @return true 如果合成成功
     */
    bool CompositeToCanvas(CompositorLayer* root, SkCanvas* canvas);

    /**
     * @brief 检查是否需要合成（有变化）
     * @param root 层树根节点
     * @return true 如果需要合成
     */
    bool NeedsComposite(CompositorLayer* root) const;

    /**
     * @brief 标记需要重新合成
     */
    void MarkNeedsComposite() { needs_composite_ = true; cache_valid_ = false; }

    // =========================================================================
    // 帧管理
    // =========================================================================

    /**
     * @brief 开始新帧
     */
    void BeginFrame();

    /**
     * @brief 结束当前帧
     * @return 帧信息
     */
    CompositorFrameInfo EndFrame();

    /**
     * @brief 获取上一帧信息
     */
    const CompositorFrameInfo& GetLastFrameInfo() const { return last_frame_info_; }

    // =========================================================================
    // 配置
    // =========================================================================

    /**
     * @brief 设置是否启用 GPU 合成
     * @param enabled 是否启用
     */
    void SetGPUEnabled(bool enabled);

    /**
     * @brief 设置是否启用帧跳过优化
     */
    void SetFrameSkipEnabled(bool enabled) { frame_skip_enabled_ = enabled; }

    /**
     * @brief 检查帧跳过是否启用
     */
    bool IsFrameSkipEnabled() const { return frame_skip_enabled_; }

    /**
     * @brief 设置是否显示层边界（调试用）
     */
    void SetShowLayerBorders(bool show) { show_layer_borders_ = show; }

    /**
     * @brief 检查是否显示层边界
     */
    bool IsShowingLayerBorders() const { return show_layer_borders_; }

    // =========================================================================
    // 统计
    // =========================================================================

    /**
     * @brief 获取合成统计信息
     */
    const CompositeStats& GetStats() const { return stats_; }

    /**
     * @brief 重置统计信息
     */
    void ResetStats() { stats_.Reset(); }

private:
    // =========================================================================
    // GPU 合成
    // =========================================================================

    /**
     * @brief 初始化着色器程序
     * @return true 如果成功
     */
    bool InitializeShaders();

    /**
     * @brief 初始化顶点缓冲
     * @return true 如果成功
     */
    bool InitializeBuffers();

    /**
     * @brief GPU 合成单个层
     * @param layer 层
     * @param parent_transform 父层变换
     */
    void CompositeLayerGPU(CompositorLayer* layer, const SkMatrix& parent_transform);

    /**
     * @brief 渲染纹理四边形
     * @param layer 层
     * @param transform 最终变换矩阵
     */
    void RenderTexturedQuad(CompositorLayer* layer, const SkMatrix& transform);

    // =========================================================================
    // CPU 合成
    // =========================================================================

    /**
     * @brief CPU 合成单个层
     * @param layer 层
     * @param canvas 目标 Canvas
     * @param parent_transform 父层变换
     */
    void CompositeLayerCPU(CompositorLayer* layer, SkCanvas* canvas, const SkMatrix& parent_transform);

    // =========================================================================
    // 辅助方法
    // =========================================================================

    /**
     * @brief 上传所有脏纹理
     * @param root 层树根节点
     * @return 上传的纹理数
     */
    int UploadDirtyTextures(CompositorLayer* root);

    /**
     * @brief 检查层树是否有变化
     * @param root 层树根节点
     * @return true 如果有变化
     */
    bool HasChanges(CompositorLayer* root) const;

    /**
     * @brief 绘制层边界（调试用）
     * @param layer 层
     * @param canvas 目标 Canvas
     */
    void DrawLayerBorder(CompositorLayer* layer, SkCanvas* canvas);

    /**
     * @brief 递归计算层数
     * @param root 层树根节点
     * @return 层数
     */
    int CountLayers(CompositorLayer* root) const;

    // 状态
    bool initialized_ = false;
    bool use_gpu_ = false;
    bool needs_composite_ = true;
    bool frame_skip_enabled_ = true;
    bool show_layer_borders_ = false;

    // 视口
    int viewport_width_ = 0;
    int viewport_height_ = 0;

    // GPU 资源
    unsigned int shader_program_ = 0;
    unsigned int vao_ = 0;
    unsigned int vbo_ = 0;

    // 着色器 uniform 位置
    int uniform_transform_ = -1;
    int uniform_opacity_ = -1;
    int uniform_texture_ = -1;

    // CPU 回退 Surface
    sk_sp<SkSurface> cpu_surface_;

    // 合成缓存（CompositeToCanvas 路径的帧跳过优化）
    // 当没有任何层变化时，直接 blit 缓存到目标 canvas，避免遍历整个层树
    SkBitmap composite_cache_;
    bool cache_valid_ = false;

    // 统计
    CompositeStats stats_;
    CompositorFrameInfo last_frame_info_;
    CompositorFrameInfo current_frame_info_;

    // 帧计时
    double frame_start_time_ = 0.0;
};

} // namespace mbink
