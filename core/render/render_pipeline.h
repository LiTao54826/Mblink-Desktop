/**
 * @file render_pipeline.h
 * @brief 渲染管线
 * 
 * 功能：
 * - 管理渲染生命周期的各个阶段
 * - 协调 DOM 同步、样式计算、布局和绘制
 * - 提供清晰的渲染流程
 * 
 * 设计参考：
 * - Chromium Blink 的 DocumentLifecycle
 * - Flutter 的渲染管线
 */

#pragma once

#include <memory>
#include <functional>

// Skia 前向声明（全局命名空间）
class SkCanvas;

namespace lightui {

// 前向声明
class DirtyNodeTracker;
class RenderTreeSynchronizer;
class RenderObject;
class Document;
class NativeLayoutEngine;

/**
 * @brief 渲染生命周期阶段
 */
enum class RenderLifecycle {
    Idle,                    ///< 空闲状态
    DOMModification,         ///< DOM 修改中（收集变化）
    RenderTreeSync,          ///< 渲染树同步
    StyleRecalc,             ///< 样式重算
    Layout,                  ///< 布局计算
    Paint,                   ///< 绘制
};

/**
 * @brief 渲染管线
 * 
 * 管理渲染生命周期，协调各个阶段的执行。
 * 确保渲染流程的正确顺序和状态一致性。
 */
class RenderPipeline {
public:
    RenderPipeline();
    ~RenderPipeline();
    
    // 禁止拷贝
    RenderPipeline(const RenderPipeline&) = delete;
    RenderPipeline& operator=(const RenderPipeline&) = delete;
    
    // ========== 配置 ==========
    
    /**
     * @brief 设置脏节点追踪器
     * @param tracker 脏节点追踪器指针
     */
    void SetDirtyTracker(DirtyNodeTracker* tracker);
    
    /**
     * @brief 设置渲染树根节点
     * @param render_tree 渲染树根节点
     */
    void SetRenderTree(std::shared_ptr<RenderObject> render_tree);
    
    /**
     * @brief 设置布局引擎
     * @param layout_engine 布局引擎指针
     */
    void SetLayoutEngine(NativeLayoutEngine* layout_engine);
    
    /**
     * @brief 设置文档
     * @param doc 文档对象
     */
    void SetDocument(std::shared_ptr<Document> doc);
    
    /**
     * @brief 设置渲染树同步器
     * @param synchronizer 渲染树同步器
     */
    void SetSynchronizer(std::shared_ptr<RenderTreeSynchronizer> synchronizer);
    
    /**
     * @brief 设置视口尺寸
     * @param width 宽度
     * @param height 高度
     */
    void SetViewportSize(float width, float height);
    
    /**
     * @brief 设置绘制回调
     * @param callback 绘制回调函数
     */
    void SetPaintCallback(std::function<void(SkCanvas*)> callback);
    
    // ========== 生命周期 ==========
    
    /**
     * @brief 获取当前生命周期阶段
     * @return 当前阶段
     */
    RenderLifecycle GetLifecycle() const { return lifecycle_; }
    
    /**
     * @brief 标记需要样式重算
     */
    void MarkNeedsStyleRecalc();
    
    /**
     * @brief 标记需要布局
     */
    void MarkNeedsLayout();
    
    /**
     * @brief 标记需要绘制
     */
    void MarkNeedsPaint();
    
    /**
     * @brief 检查是否需要更新
     * @return true 表示需要更新
     */
    bool NeedsUpdate() const;
    
    // ========== 渲染流程 ==========
    
    /**
     * @brief 处理一帧
     * 
     * 执行完整的渲染流程：
     * 1. 渲染树同步（如果有待处理的 DOM 变化）
     * 2. 样式重算（如果需要）
     * 3. 布局计算（如果需要）
     * 4. 绘制（如果需要）
     * 
     * @param canvas Skia 画布（可选，如果为 nullptr 则跳过绘制）
     */
    void ProcessFrame(SkCanvas* canvas = nullptr);
    
    /**
     * @brief 强制完整更新
     * 
     * 强制执行完整的渲染流程，忽略脏标记。
     * 用于窗口 resize 等需要完整重新渲染的场景。
     */
    void ForceFullUpdate();
    
private:
    // ========== 内部方法 ==========
    
    /**
     * @brief 执行渲染树同步阶段
     */
    void DoRenderTreeSync();
    
    /**
     * @brief 执行样式重算阶段
     */
    void DoStyleRecalc();
    
    /**
     * @brief 执行布局阶段
     */
    void DoLayout();
    
    /**
     * @brief 执行绘制阶段
     * @param canvas Skia 画布
     */
    void DoPaint(SkCanvas* canvas);
    
private:
    /// 当前生命周期阶段
    RenderLifecycle lifecycle_ = RenderLifecycle::Idle;
    
    /// 脏节点追踪器
    DirtyNodeTracker* dirty_tracker_ = nullptr;
    
    /// 渲染树同步器
    std::shared_ptr<RenderTreeSynchronizer> synchronizer_;
    
    /// 布局引擎
    NativeLayoutEngine* layout_engine_ = nullptr;
    
    /// 渲染树根节点
    std::shared_ptr<RenderObject> render_tree_;
    
    /// 文档
    std::weak_ptr<Document> document_;
    
    /// 视口尺寸
    float viewport_width_ = 0.0f;
    float viewport_height_ = 0.0f;
    
    /// 绘制回调
    std::function<void(SkCanvas*)> paint_callback_;
    
    /// 脏标记
    bool needs_style_recalc_ = false;
    bool needs_layout_ = false;
    bool needs_paint_ = false;
};

} // namespace lightui
