/**
 * @file layer_manager.h
 * @brief LayerManager - 管理多个渲染层级
 * 
 * LayerManager 是 Layer 系统的核心，负责：
 * - 管理多个 Layer（Base, Overlay, Modal）
 * - 统一的 Paint、HitTest、HandleWheel 接口
 * - 元素收集和分发到对应 Layer
 */

#pragma once

#include "layer.h"
#include <vector>
#include <memory>

namespace lightui {

/**
 * @brief Layer 级别定义
 */
enum class LayerLevel {
    Base = 0,      // z-index < 100: 普通元素
    Overlay = 1,   // z-index 100-999: dropdown, tooltip, popover
    Modal = 2,     // z-index >= 1000: modal, dialog
    Count = 3
};

/**
 * @brief LayerManager 单例
 * 
 * 使用方式：
 * 1. BeginFrame() - 每帧开始时调用
 * 2. ShouldCollect() + Collect() - 在 Paint 过程中收集高 z-index 元素
 * 3. PaintLayers() - 绘制所有 Layer
 * 4. HitTest() - 从最高 Layer 开始测试
 * 5. HandleWheel() - 从最高 Layer 开始处理滚动
 */
class LayerManager {
public:
    /**
     * @brief 获取单例实例
     */
    static LayerManager& Instance();
    
    /**
     * @brief 开始新的渲染帧
     * 清除所有 Layer 中的元素
     */
    void BeginFrame();
    
    /**
     * @brief 检查是否应该收集该元素到 Layer
     * @param render_obj 渲染对象
     * @return 如果应该收集返回 true
     */
    bool ShouldCollect(const RenderObject* render_obj) const;
    
    /**
     * @brief 收集元素到对应的 Layer
     * @param render_obj 渲染对象
     * @param transform 当前的变换矩阵
     * @param z_index z-index 值
     */
    void Collect(std::shared_ptr<RenderObject> render_obj, const SkMatrix& transform, int z_index);
    
    /**
     * @brief 绘制所有 Layer（按层级顺序）
     * @param canvas 画布
     */
    void PaintLayers(SkCanvas* canvas);
    
    /**
     * @brief Hit Testing（从最高 Layer 开始）
     * @param x 鼠标 X 坐标
     * @param y 鼠标 Y 坐标
     * @param result 输出结果
     * @return true 如果命中了元素
     */
    bool HitTest(float x, float y, HitTestResult& result);
    
    /**
     * @brief 处理滚轮事件（从最高 Layer 开始）
     * @param x 鼠标 X 坐标
     * @param y 鼠标 Y 坐标
     * @param delta_x 水平滚动量
     * @param delta_y 垂直滚动量
     * @return true 如果事件被处理
     */
    bool HandleWheel(float x, float y, float delta_x, float delta_y);
    
    /**
     * @brief 检查是否有非 Base 层的元素
     */
    bool HasOverlays() const;
    
    /**
     * @brief 检查是否正在绘制 Layer
     */
    bool IsPaintingLayers() const { return painting_layers_; }
    
    /**
     * @brief 获取指定级别的 Layer
     */
    Layer* GetLayer(LayerLevel level);
    
    /**
     * @brief 设置 Overlay 层的 z-index 阈值
     */
    void SetOverlayThreshold(int threshold) { overlay_threshold_ = threshold; }
    
    /**
     * @brief 设置 Modal 层的 z-index 阈值
     */
    void SetModalThreshold(int threshold) { modal_threshold_ = threshold; }

private:
    LayerManager();
    ~LayerManager() = default;
    LayerManager(const LayerManager&) = delete;
    LayerManager& operator=(const LayerManager&) = delete;
    
    std::vector<std::unique_ptr<Layer>> layers_;
    int overlay_threshold_ = 100;   // z-index >= 100 进入 Overlay 层
    int modal_threshold_ = 1000;    // z-index >= 1000 进入 Modal 层
    bool painting_layers_ = false;  // 是否正在绘制 Layer
    bool frame_started_ = false;    // 新帧是否开始（用于延迟清空）
    
    /**
     * @brief 根据 z-index 获取对应的 Layer 级别
     */
    LayerLevel GetLayerLevel(int z_index) const;
};

} // namespace lightui
