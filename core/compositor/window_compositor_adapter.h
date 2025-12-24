/**
 * @file window_compositor_adapter.h
 * @brief 窗口合成器适配器 - 将 RenderPipelineV2 集成到 Window 类
 *
 * WindowCompositorAdapter 提供：
 * - 在旧渲染管线和新分层合成架构之间切换
 * - 统一的渲染接口
 * - 滚动和动画事件路由
 *
 * 使用方式：
 * 1. 在 Window 中创建 WindowCompositorAdapter
 * 2. 调用 SetUseLayerCompositing(true) 启用分层合成
 * 3. 在 Render() 中调用 adapter->Render()
 */

#pragma once

#include "render_pipeline_v2.h"
#include <memory>

// 前向声明 Skia 类
class SkCanvas;
class SkSurface;

namespace lightui {

// 前向声明
class RenderObject;
class Document;
class Window;

/**
 * @brief 窗口合成器适配器
 *
 * 将 RenderPipelineV2 集成到 Window 渲染流程。
 */
class WindowCompositorAdapter {
public:
    WindowCompositorAdapter();
    ~WindowCompositorAdapter();

    // 禁止拷贝
    WindowCompositorAdapter(const WindowCompositorAdapter&) = delete;
    WindowCompositorAdapter& operator=(const WindowCompositorAdapter&) = delete;

    // =========================================================================
    // 初始化
    // =========================================================================

    /**
     * @brief 初始化适配器
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
     * @brief 关闭适配器
     */
    void Shutdown();

    /**
     * @brief 检查是否已初始化
     */
    bool IsInitialized() const;

    // =========================================================================
    // 模式切换
    // =========================================================================

    /**
     * @brief 设置是否使用分层合成
     * @param use_layer_compositing true 使用新的分层合成架构
     */
    void SetUseLayerCompositing(bool use_layer_compositing);

    /**
     * @brief 检查是否使用分层合成
     */
    bool IsUsingLayerCompositing() const { return use_layer_compositing_; }

    // =========================================================================
    // 渲染
    // =========================================================================

    /**
     * @brief 渲染一帧
     * @param render_tree 渲染树根节点
     * @param canvas 目标 Canvas
     * @return true 如果渲染成功
     *
     * 如果启用分层合成，使用 RenderPipelineV2
     * 否则返回 false，让 Window 使用旧渲染路径
     */
    bool Render(RenderObject* render_tree, SkCanvas* canvas);

    /**
     * @brief 检查是否需要渲染
     */
    bool NeedsRender() const;

    /**
     * @brief 标记需要渲染
     */
    void MarkNeedsRender();

    /**
     * @brief 强制重新光栅化所有层
     * 
     * 在有活动动画时调用，确保层内容被正确更新
     */
    void ForceRasterize();

    // =========================================================================
    // 滚动处理
    // =========================================================================

    /**
     * @brief 处理滚动事件
     * @param container 滚动容器
     * @param delta_x X 方向滚动增量
     * @param delta_y Y 方向滚动增量
     * @return true 如果滚动成功
     */
    bool HandleScroll(RenderObject* container, float delta_x, float delta_y);

    // =========================================================================
    // 动画处理
    // =========================================================================

    /**
     * @brief 开始动画帧
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
     * @return true 如果有层属性更新
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
     * @param region 脏区域
     */
    void MarkDirtyRegion(const SkRect& region);

    /**
     * @brief 使层树无效，需要重建
     * 
     * 当渲染树被重建时调用，确保层树中的 RenderObject 指针不会变成悬空指针
     */
    void InvalidateLayerTree();

    // =========================================================================
    // 配置
    // =========================================================================

    /**
     * @brief 设置是否显示层边界（调试用）
     */
    void SetShowLayerBorders(bool show);

    /**
     * @brief 设置 DPI 缩放比
     * @param scale DPI 缩放比
     */
    void SetDpiScale(float scale);

    /**
     * @brief 获取渲染管线（用于高级配置）
     */
    RenderPipelineV2* GetPipeline() { return pipeline_.get(); }

    // =========================================================================
    // 统计
    // =========================================================================

    /**
     * @brief 获取上一帧统计
     */
    const RenderFrameStats& GetLastFrameStats() const;

private:
    // 分层合成管线
    std::unique_ptr<RenderPipelineV2> pipeline_;

    // 是否使用分层合成
    bool use_layer_compositing_ = false;

    // 视口尺寸
    int viewport_width_ = 0;
    int viewport_height_ = 0;

    // DPI 缩放
    float dpi_scale_ = 1.0f;
};

} // namespace lightui
