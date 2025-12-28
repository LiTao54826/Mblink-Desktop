/**
 * @file layer.h
 * @brief Layer 类 - 管理单个渲染层级
 * 
 * Layer 系统用于实现 CSS Stacking Context，支持：
 * - 正确的绘制顺序
 * - 正确的 Hit Testing
 * - 滚动隔离
 */

#pragma once

#include "core/render/objects/render_object.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkMatrix.h"
#include <vector>
#include <memory>

namespace lightui {

// 前向声明
struct HitTestResult;

/**
 * @brief Layer 中的元素项
 */
struct LayerItem {
    std::shared_ptr<RenderObject> render_obj;  // 渲染对象
    SkMatrix transform;                         // 收集时的变换矩阵
    int z_index;                                // z-index 值
    
    // 用于 Hit Testing 的边界信息
    float abs_x = 0;      // 绝对 X 坐标
    float abs_y = 0;      // 绝对 Y 坐标
    float width = 0;      // 宽度
    float height = 0;     // 高度
};

/**
 * @brief 渲染层级
 * 
 * 每个 Layer 管理一个 z-index 范围内的元素，提供：
 * - Paint: 按 z-index 顺序绘制
 * - HitTest: 从高 z-index 到低 z-index 测试
 * - HandleWheel: 处理滚动事件
 */
class Layer {
public:
    /**
     * @brief 构造函数
     * @param z_min z-index 最小值（包含）
     * @param z_max z-index 最大值（包含），-1 表示无上限
     */
    Layer(int z_min, int z_max = -1);
    
    ~Layer() = default;
    
    // 禁止拷贝
    Layer(const Layer&) = delete;
    Layer& operator=(const Layer&) = delete;
    
    /**
     * @brief 获取 z-index 范围
     */
    int GetZIndexMin() const { return z_index_min_; }
    int GetZIndexMax() const { return z_index_max_; }
    
    /**
     * @brief 检查 z-index 是否属于此 Layer
     */
    bool ContainsZIndex(int z_index) const;
    
    /**
     * @brief 添加渲染对象
     * @param obj 渲染对象
     * @param transform 收集时的变换矩阵
     * @param z_index z-index 值
     */
    void AddItem(std::shared_ptr<RenderObject> obj, const SkMatrix& transform, int z_index);
    
    /**
     * @brief 清除所有元素（每帧开始时调用）
     */
    void Clear();
    
    /**
     * @brief 检查是否为空
     */
    bool IsEmpty() const { return items_.empty(); }
    
    /**
     * @brief 获取元素数量
     */
    size_t GetItemCount() const { return items_.size(); }
    
    /**
     * @brief 绘制所有元素
     * @param canvas 画布
     */
    void Paint(SkCanvas* canvas);
    
    /**
     * @brief Hit Testing
     * @param x 鼠标 X 坐标（视口坐标）
     * @param y 鼠标 Y 坐标（视口坐标）
     * @param result 输出结果
     * @return true 如果命中了元素
     */
    bool HitTest(float x, float y, HitTestResult& result);
    
    /**
     * @brief 处理滚轮事件
     * @param x 鼠标 X 坐标
     * @param y 鼠标 Y 坐标
     * @param delta_x 水平滚动量
     * @param delta_y 垂直滚动量
     * @return true 如果事件被处理
     */
    bool HandleWheel(float x, float y, float delta_x, float delta_y);

private:
    int z_index_min_;
    int z_index_max_;  // -1 表示无上限
    std::vector<LayerItem> items_;
    bool needs_sort_ = false;  // 是否需要排序
    
    /**
     * @brief 确保元素按 z-index 排序
     */
    void EnsureSorted();
    
    /**
     * @brief 递归 Hit Test 渲染对象
     */
    bool HitTestRenderObject(
        std::shared_ptr<RenderObject> render_obj,
        float x, float y,
        float offset_x, float offset_y,
        HitTestResult& result);
};

} // namespace lightui
