/**
 * @file hit_test_controller.h
 * @brief 统一命中测试控制器
 *
 * 使用 ViewportBounds 缓存进行高效命中测试。
 * 
 * 核心设计：
 * - 使用预计算的视口坐标缓存，O(1) 边界检查
 * - 支持所有 CSS 定位类型（static/relative/absolute/fixed）
 * - 支持 CSS Transform、裁剪、层叠上下文
 * - 提供 DevTools 调试信息
 */

#pragma once

#include <memory>
#include <string>
#include <vector>

namespace mbink {

// 前向声明
class Element;
class RenderObject;
class PaintLayer;
class CompositorLayer;

/**
 * @brief 命中测试请求选项
 */
struct HitTestRequest {
    bool ignore_pointer_events = false;  ///< 忽略 pointer-events: none
    bool for_devtools = false;           ///< 为 DevTools 收集额外信息
    bool test_visibility = true;         ///< 检查 visibility: hidden
    bool test_opacity = true;            ///< 检查 opacity: 0
};

/**
 * @brief 基础命中测试结果（向后兼容）
 */
struct HitTestResult {
    std::shared_ptr<Element> element;           ///< 命中的元素
    std::shared_ptr<RenderObject> render_object; ///< 对应的渲染对象
    float local_x = 0;                          ///< 元素局部 X 坐标
    float local_y = 0;                          ///< 元素局部 Y 坐标

    HitTestResult() : element(nullptr), render_object(nullptr), local_x(0), local_y(0) {}

    bool IsValid() const { return element != nullptr; }
};

/**
 * @brief 命中测试结果（扩展版）
 */
struct HitTestResultEx {
    std::shared_ptr<Element> element;           ///< 命中的元素
    std::shared_ptr<RenderObject> render_object; ///< 对应的渲染对象
    float local_x = 0;                          ///< 元素局部 X 坐标
    float local_y = 0;                          ///< 元素局部 Y 坐标
    float viewport_x = 0;                       ///< 视口 X 坐标
    float viewport_y = 0;                       ///< 视口 Y 坐标

    // DevTools 扩展信息
    int z_index = 0;                            ///< 元素的 z-index
    bool in_stacking_context = false;           ///< 是否在层叠上下文中
    std::string miss_reason;                    ///< 未命中原因（调试用）

    bool IsValid() const { return element != nullptr; }
};

/**
 * @brief 统一命中测试控制器
 * 
 * 使用 ViewportBounds 缓存进行高效命中测试。
 * 替代原有的 HitTesting 和 PaintLayer::HitTest。
 */
class HitTestController {
public:
    HitTestController() = default;
    ~HitTestController() = default;

    /**
     * @brief 执行命中测试
     * @param root_render 渲染树根节点
     * @param viewport_x 视口 X 坐标
     * @param viewport_y 视口 Y 坐标
     * @param request 命中测试选项
     * @return 命中测试结果
     */
    HitTestResultEx HitTest(
        std::shared_ptr<RenderObject> root_render,
        float viewport_x,
        float viewport_y,
        const HitTestRequest& request = HitTestRequest());

    /**
     * @brief 解释为什么点击没有命中某个元素
     * @param render_object 目标渲染对象
     * @param viewport_x 视口 X 坐标
     * @param viewport_y 视口 Y 坐标
     * @return 未命中原因描述
     */
    std::string ExplainMiss(
        std::shared_ptr<RenderObject> render_object,
        float viewport_x,
        float viewport_y);

private:
    /**
     * @brief 测试独立合成层
     */
    bool HitTestCompositorLayers(
        const std::vector<std::shared_ptr<CompositorLayer>>& layers,
        float viewport_x,
        float viewport_y,
        const HitTestRequest& request,
        HitTestResultEx& result);

    /**
     * @brief 测试 PaintLayer 及其子层
     */
    bool HitTestLayer(
        PaintLayer* layer,
        float viewport_x,
        float viewport_y,
        const HitTestRequest& request,
        HitTestResultEx& result);

    /**
     * @brief 测试单个渲染对象及其子元素
     */
    bool HitTestRenderObject(
        RenderObject* render_obj,
        float viewport_x,
        float viewport_y,
        const HitTestRequest& request,
        HitTestResultEx& result,
        bool test_out_of_flow_descendants = true);

    bool HitTestRenderObjectWithViewportOffset(
        RenderObject* render_obj,
        RenderObject* offset_root,
        float viewport_x,
        float viewport_y,
        const HitTestRequest& request,
        HitTestResultEx& result,
        float offset_x,
        float offset_y,
        bool test_out_of_flow_descendants = true);

    bool HitTestStickyTableCell(
        RenderObject* render_obj,
        float viewport_x,
        float viewport_y,
        const HitTestRequest& request,
        HitTestResultEx& result);

    bool IsClippedWithViewportOffset(
        RenderObject* render_obj,
        RenderObject* offset_root,
        float viewport_x,
        float viewport_y,
        float offset_x,
        float offset_y);

    /**
     * @brief 检查元素是否被裁剪
     */
    bool IsClipped(
        RenderObject* render_obj,
        float viewport_x,
        float viewport_y);

    /**
     * @brief 填充 DevTools 调试信息
     */
    void FillDevToolsInfo(
        HitTestResultEx& result,
        RenderObject* render_obj,
        PaintLayer* layer);
};

}  // namespace mbink

