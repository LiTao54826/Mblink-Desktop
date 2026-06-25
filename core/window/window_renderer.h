/**
 * @file window_renderer.h
 * @brief 窗口渲染器
 * 
 * 从 window.cpp 提取的渲染相关逻辑，负责：
 * - 渲染树构建和管理
 * - 增量渲染和脏区域管理
 * - 动画更新和应用
 * - DevTools 渲染
 */

#pragma once

#include <memory>
#include <vector>
#include <unordered_map>
#include "include/core/SkCanvas.h"
#include "include/core/SkRect.h"

namespace mblink {

// 前向声明
class Window;
class Document;
class RenderObject;
class Node;
class LayoutEngine;
class AnimationTimeline;
class AnimationController;
class AnimationApplicator;
class RenderPipeline;
class RenderTreeSynchronizer;
class FBOManager;

/**
 * @brief 窗口渲染器
 * 
 * 负责窗口的渲染逻辑，包括渲染树管理、增量渲染、动画等
 */
class WindowRenderer {
public:
    /**
     * @brief 构造函数
     * @param window 关联的窗口
     */
    explicit WindowRenderer(Window* window);
    
    /**
     * @brief 析构函数
     */
    ~WindowRenderer();
    
    // 禁止拷贝
    WindowRenderer(const WindowRenderer&) = delete;
    WindowRenderer& operator=(const WindowRenderer&) = delete;
    
    // ========== 主渲染接口 ==========
    
    /**
     * @brief 执行渲染
     * 
     * 自动判断使用全量渲染或增量渲染：
     * - 首次渲染/渲染树失效 → 全量渲染
     * - 只有脏区域 → 增量渲染
     */
    void Render();
    
    /**
     * @brief 渲染 DevTools 面板
     * @param canvas Skia 画布
     * @param width 窗口宽度
     * @param height 窗口高度
     */
    void RenderDevTools(SkCanvas* canvas, float width, float height);
    
    // ========== 渲染树管理 ==========
    
    /**
     * @brief 确保渲染树已构建
     */
    void EnsureRenderTree();
    
    /**
     * @brief 标记渲染树需要重建
     */
    void InvalidateRenderTree();
    
    /**
     * @brief 获取缓存的渲染树
     * @return 渲染树根节点
     */
    std::shared_ptr<RenderObject> GetCachedRenderTree() const { return cached_render_tree_; }
    
    /**
     * @brief 检查渲染树是否有效
     */
    bool IsRenderTreeValid() const { return render_tree_valid_; }
    
    // ========== 脏区域管理 ==========
    
    /**
     * @brief 添加脏区域
     * @param rect 脏区域矩形
     */
    void AddDirtyRect(const SkRect& rect);
    
    /**
     * @brief 获取脏区域列表
     */
    const std::vector<SkRect>& GetDirtyRects() const { return dirty_rects_; }
    
    /**
     * @brief 清空脏区域
     */
    void ClearDirtyRects() { dirty_rects_.clear(); }
    
    /**
     * @brief 从渲染树收集脏区域
     * @param root 渲染树根节点
     */
    void CollectDirtyRectsFromRenderTree(RenderObject* root);
    
    // ========== 增量渲染控制 ==========
    
    /**
     * @brief 设置是否启用增量渲染
     */
    void SetEnableIncrementalRender(bool enable) { enable_incremental_render_ = enable; }
    
    /**
     * @brief 获取是否启用增量渲染
     */
    bool IsIncrementalRenderEnabled() const { return enable_incremental_render_; }
    
    /**
     * @brief 设置是否强制全屏重绘
     */
    void SetForceFullRepaint(bool force) { force_full_repaint_ = force; }
    
    /**
     * @brief 获取是否强制全屏重绘
     */
    bool IsForceFullRepaint() const { return force_full_repaint_; }
    
    // ========== 动画 ==========
    
    /**
     * @brief 更新动画
     * @param current_time 当前时间（秒）
     */
    void UpdateAnimations(double current_time);
    
    /**
     * @brief 应用动画到渲染树
     * @param root 渲染树根节点
     */
    void ApplyAnimationsToRenderTree(RenderObject* root);
    
    /**
     * @brief 检查是否有待启动的动画
     * @param root 渲染树根节点
     */
    bool HasPendingAnimations(RenderObject* root) const;
    
    // ========== 布局 ==========
    
    /**
     * @brief 增量布局脏子树
     * @param render_obj 渲染对象
     * @param parent_width 父元素宽度
     * @param parent_height 父元素高度
     * @return true 表示该节点或其子节点被重新布局
     */
    bool LayoutDirtySubtree(RenderObject* render_obj, float parent_width, float parent_height);
    
    // ========== 辅助方法 ==========
    
    /**
     * @brief 标记 RenderObject 为脏
     * @param dom_node DOM 节点
     * @param render_obj 渲染对象
     */
    void MarkRenderObjectsDirty(Node* dom_node, RenderObject* render_obj);
    
    /**
     * @brief 清除 DOM 节点的脏标记
     * @param node DOM 节点
     */
    void ClearDirtyFlags(Node* node);
    
    /**
     * @brief 清除 RenderObject 的脏标记
     * @param render_obj 渲染对象
     */
    void ClearRenderObjectDirtyFlags(RenderObject* render_obj);
    
    /**
     * @brief 检查是否有需要布局的脏节点
     * @param node DOM 节点
     */
    bool HasDirtyLayoutNodes(Node* node);
    
    /**
     * @brief 保存滚动位置
     * @param render_obj 渲染对象
     * @param scroll_positions 滚动位置映射表
     */
    void SaveScrollPositions(RenderObject* render_obj,
                            std::unordered_map<Node*, std::pair<float, float>>& scroll_positions);
    
    /**
     * @brief 恢复滚动位置
     * @param render_obj 渲染对象
     * @param scroll_positions 滚动位置映射表
     */
    void RestoreScrollPositions(RenderObject* render_obj,
                               const std::unordered_map<Node*, std::pair<float, float>>& scroll_positions);

private:
    Window* window_;  // 关联的窗口（不拥有）
    
    // 渲染树缓存
    std::shared_ptr<RenderObject> cached_render_tree_;
    bool render_tree_valid_ = false;
    
    // 脏区域
    std::vector<SkRect> dirty_rects_;
    SkRect last_dirty_bounds_;
    bool has_dirty_bounds_ = false;
    
    // 增量渲染控制
    bool enable_incremental_render_ = true;
    bool force_full_repaint_ = false;
    
    // FBO 增量渲染
    bool use_fbo_incremental_ = false;
    bool fbo_needs_full_paint_ = true;
    
    // 滚动位置跟踪
    float last_body_scroll_x_ = 0.0f;
    float last_body_scroll_y_ = 0.0f;
};

} // namespace mblink
