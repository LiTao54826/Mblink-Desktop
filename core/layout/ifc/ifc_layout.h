/**
 * @file ifc_layout.h
 * @brief IFC 布局入口
 *
 * 提供 IFC 布局的统一入口，整合所有 IFC 组件。
 * 用于 LayoutEngine 调用。
 */

#pragma once

#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>
#include "inline_box.h"
#include "line_box.h"
#include "line_breaker.h"
#include "vertical_aligner.h"
#include "../types/traits.h"
#include "../../render/render_object.h"

namespace lightui {

/**
 * @brief IFC 布局结果
 */
struct IFCLayoutResult {
    /** @brief 内容总高度 */
    float total_height = 0.0f;

    /** @brief 内容最大宽度 */
    float max_width = 0.0f;

    /** @brief 行数 */
    size_t line_count = 0;

    /** @brief 是否成功 */
    bool success = false;
};

/**
 * @brief IFC 布局器
 *
 * 整合 IFC 的所有组件，提供简单的布局接口。
 *
 * 使用方式：
 * ```cpp
 * IFCLayout ifc_layout;
 * IFCLayoutResult result = ifc_layout.Layout(container, available_width);
 * float height = result.total_height;
 * ```
 */
class IFCLayout {
public:
    /** @brief 默认构造函数 */
    IFCLayout() = default;

    /**
     * @brief 执行 IFC 布局
     * @param container 容器渲染对象
     * @param available_width 可用宽度
     * @param apply_results 是否应用布局结果到渲染对象（默认为 true）
     * @param content_version 外部传入的内容版本号（0表示使用旧的哈希计算方式）
     * @return 布局结果
     */
    IFCLayoutResult Layout(RenderObject* container, float available_width, bool apply_results = true, uint64_t content_version = 0);

    /**
     * @brief 获取内容高度
     * @return 所有行的总高度
     */
    float GetContentHeight() const { return content_height_; }

    /**
     * @brief 获取内容宽度
     * @return 最宽行的宽度
     */
    float GetContentWidth() const { return content_width_; }

    /**
     * @brief 获取生成的行盒
     * @return 行盒列表
     */
    const std::vector<LineBox>& GetLineBoxes() const { return line_boxes_; }

    /**
     * @brief 检查容器是否包含内联内容
     * @param container 容器渲染对象
     * @return 如果包含内联内容则返回 true
     */
    static bool HasInlineContent(RenderObject* container);

    /**
     * @brief 检查渲染对象是否参与 IFC
     * @param render_obj 渲染对象
     * @return 如果参与 IFC 则返回 true
     */
    static bool IsInlineLevel(RenderObject* render_obj);

    //--------------------------------------------------------------------------
    // Static measurement methods (for Taffy integration)
    //--------------------------------------------------------------------------

    /**
     * @brief 静态文本测量方法 (供 Taffy 调用)
     * @param text 文本内容
     * @param font_size 字体大小
     * @param font_family 字体族
     * @param letter_spacing 字符间距
     * @param word_spacing 词间距
     * @param line_height_multiplier 行高倍数
     * @return TextMeasureResult 包含宽度、高度、ascent 和 descent
     */
    static TextMeasureResult MeasureTextStatic(
        const std::string& text,
        float font_size,
        const std::string& font_family,
        float letter_spacing = 0.0f,
        float word_spacing = 0.0f,
        float line_height_multiplier = 1.2f
    );

    /**
     * @brief 执行 IFC 布局并返回详细结果 (供 Taffy 调用)
     * @param container 容器渲染对象
     * @param available_width 可用宽度
     * @return IFCMeasureResult 包含行盒信息
     */
    IFCMeasureResult LayoutWithResult(RenderObject* container, float available_width);

    /**
     * @brief 测量容器的最小内容宽度（最长单词的宽度）
     * @param container 容器渲染对象
     * @return 最小内容宽度
     */
    float MeasureMinContentWidth(RenderObject* container);

    /**
     * @brief 使容器的布局缓存失效
     * @param container 容器渲染对象
     */
    void InvalidateCache(RenderObject* container);

    /**
     * @brief 清除所有布局缓存
     */
    void ClearCache();

    /**
     * @brief 检查缓存是否有效
     * @param container 容器渲染对象
     * @param available_width 可用宽度
     * @param content_version 外部传入的内容版本号（0表示使用旧的哈希计算方式）
     * @return 如果缓存有效则返回 true
     */
    bool IsCacheValid(RenderObject* container, float available_width, uint64_t content_version = 0) const;

private:
    /** @brief 布局结果 */
    std::vector<LineBox> line_boxes_;
    float content_height_ = 0.0f;
    float content_width_ = 0.0f;

    /** @brief 当前布局的可用宽度（用于 CreateInlineBox） */
    float current_available_width_ = 0.0f;

    /** @brief 组件实例 */
    LineBreaker line_breaker_;
    VerticalAligner vertical_aligner_;

    /** @brief 内部收集的内联盒 */
    std::vector<InlineBox> inline_boxes_;

    /** @brief 布局缓存结构 */
    struct LayoutCache {
        float available_width = 0.0f;
        float content_height = 0.0f;
        float content_width = 0.0f;
        std::vector<LineBox> line_boxes;
        std::vector<InlineBox> inline_boxes;  // Cache inline boxes for ApplyLayoutResults
        uint64_t content_version = 0;  // 内容版本号（从外部传入，不再内部计算哈希）
        bool valid = false;
    };

    /** @brief 容器到缓存的映射 */
    std::unordered_map<RenderObject*, LayoutCache> cache_;

    /** @brief 当前容器的内容版本 */
    size_t GetContentVersion(RenderObject* container) const;

    /**
     * @brief 收集内联内容
     * @param container 容器
     */
    void CollectInlineContent(RenderObject* container);

    /**
     * @brief 从渲染对象创建内联盒
     * @param render_obj 渲染对象
     */
    void CreateInlineBox(RenderObject* render_obj);

    /**
     * @brief 测量文本尺寸
     * @param text 文本内容
     * @param style 计算样式
     * @return {宽度, 高度}
     */
    std::pair<float, float> MeasureText(
        const std::string& text,
        const ComputedStyle& style
    );

    /**
     * @brief 应用布局结果到渲染对象
     * @param container 容器
     * @param container_width 容器的 border-box 宽度（用于计算 padding）
     */
    void ApplyLayoutResults(RenderObject* container, float container_width);
};

} // namespace lightui

